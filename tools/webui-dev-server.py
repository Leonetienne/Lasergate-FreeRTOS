#!/usr/bin/env python3
"""
Local preview of main/web/ without an ESP32. Stdlib only.

Serves the static files and fakes the firmware's REST + websocket API (see
HttpServerEsp32.cpp) against an in-memory state. Any state is reachable, so every page can be previewed.

Usage: python3 tools/webui-dev-server.py [port]  (default 8080)
"""

import base64
import hashlib
import http.server
import json
import random
import socketserver
import sys
import threading
import time
from pathlib import Path
from urllib.parse import parse_qs

WEB_DIR = Path(__file__).resolve().parent.parent / "main" / "web"
MODULE_COUNT = 4

ALL_STATES = [
    "INITIALIZING", "USER_ADJUSTING_BEAMS", "CALIBRATION_LDR_THRESH",
    "CALIBRATION_MODULATION_FREQUENCY", "OBSERVING", "DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST",
    "DIAGNOSTIC_GPIO_DISCOVERY", "DISARMED", "ALARM", "FAULT", "SHUTTING_DOWN",
]

# states the real Gate auto-exits back to DISARMED once every module "finishes"
AUTO_EXIT_STATES = {"CALIBRATION_LDR_THRESH", "CALIBRATION_MODULATION_FREQUENCY", "DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST"}
AUTO_EXIT_AFTER_SECONDS = 4

# mirrors GpioDiscovery::getAdc1Pins() / getAdc2Pins() / getFreeOutputPins() for this board
GPIO_ADC1_PINS = [1, 2]
GPIO_ADC2_PINS = [15, 16, 17, 18]
GPIO_OUTPUT_PINS = [21, 38, 39, 40, 41, 42, 47, 48]

PAGE_ROUTES = {
    "/": "index.html",
    "/emitters": "emitters.html",
    "/calibrate/ldr": "calibrate-ldr.html",
    "/calibrate/frequency": "calibrate-freq.html",
    "/modules": "modules.html",
    "/self-test": "self-test.html",
    "/gpio-discovery": "gpio-discovery.html",
    "/config": "config.html",
}

WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def make_module(index, configured):
    if not configured:
        return {
            "index": index, "configured": False, "ready": False,
            "laser_gpio": None, "led_gpio": None, "ldr_gpio": None,
            "ldr_threshold": 0, "pulse_frequency_ms": 0,
            "pulse_batch_acceptable": None, "batch_time_ms": None,
            "self_test_error_count": None, "self_test_finished": None,
        }
    return {
        "index": index, "configured": True, "ready": True,
        "laser_gpio": 4 + index, "led_gpio": 10 + index, "ldr_gpio": 1 + index,
        "ldr_threshold": 1800 + index * 10, "pulse_frequency_ms": 8,
        "pulse_batch_acceptable": True, "batch_time_ms": 256,
        "self_test_error_count": 0, "self_test_finished": True,
    }


class FakeSystem:
    """Holds the dev server's in-memory system state, ticked by a background thread."""

    def __init__(self):
        self.lock = threading.Lock()
        self.state = "DISARMED"
        self.fault_reason = ""
        self.start_time = time.time()
        self.state_entered_at = time.time()
        # module 3 unconfigured, previews that badge state
        self.modules = [make_module(i, configured=(i < 3)) for i in range(MODULE_COUNT)]
        self.gpio_adc = {p: random.randint(500, 3500) for p in GPIO_ADC1_PINS + GPIO_ADC2_PINS}
        self.gpio_levels = {p: False for p in GPIO_OUTPUT_PINS}
        self.ws_clients = []  # list of (connection, lock)

    def set_state(self, name, fault_reason=""):
        with self.lock:
            self.state = name
            self.fault_reason = fault_reason
            self.state_entered_at = time.time()

    def apply_module_form(self, index, form):
        def field(name):
            values = form.get(name)
            return values[0] if values else ""

        def gpio(name):
            v = field(name)
            return int(v) if v.isdigit() else None

        with self.lock:
            m = self.modules[index]
            m["laser_gpio"] = gpio("laser_gpio")
            m["led_gpio"] = gpio("led_gpio")
            m["ldr_gpio"] = gpio("ldr_gpio")
            m["configured"] = m["laser_gpio"] is not None and m["ldr_gpio"] is not None
            m["ready"] = m["configured"]
            m["ldr_threshold"] = int(field("ldr_thresh")) if field("ldr_thresh").isdigit() else 0
            m["pulse_frequency_ms"] = int(field("pulse_freq")) if field("pulse_freq").isdigit() else 0
        self.set_state("SHUTTING_DOWN")  # save restarts, like the firmware
        threading.Timer(1.0, lambda: self.set_state("DISARMED")).start()

    def set_gpio_level(self, pin, high):
        with self.lock:
            if pin in self.gpio_levels:
                self.gpio_levels[pin] = high

    def snapshot_json(self):
        with self.lock:
            gpio_pins = []
            if self.state == "DIAGNOSTIC_GPIO_DISCOVERY":
                gpio_pins = (
                    [{"gpio": p, "role": "adc1" if p in GPIO_ADC1_PINS else "adc2", "adc_raw": v, "level": None}
                     for p, v in self.gpio_adc.items()] +
                    [{"gpio": p, "role": "output", "adc_raw": None, "level": v} for p, v in self.gpio_levels.items()]
                )
            payload = {
                "state": self.state,
                "fault_reason": self.fault_reason,
                "uptime_ms": int((time.time() - self.start_time) * 1000),
                "time": int(time.time()),
                "allowed_transitions": [s for s in ALL_STATES if s != self.state],
                "modules": self.modules,
                "gpio_pins": gpio_pins,
            }
        return json.dumps(payload)

    def tick(self):
        with self.lock:
            state, entered_at = self.state, self.state_entered_at
            if state == "DIAGNOSTIC_GPIO_DISCOVERY":
                for p in self.gpio_adc:
                    self.gpio_adc[p] = max(0, min(4095, self.gpio_adc[p] + random.randint(-200, 200)))
        if state in AUTO_EXIT_STATES and time.time() - entered_at > AUTO_EXIT_AFTER_SECONDS:
            self.set_state("DISARMED")
        if state == "ALARM" and random.random() < 0.02:
            pass  # placeholder, alarms clear only via Stop alarm

    def broadcast(self):
        payload = self.snapshot_json()
        frame = encode_ws_text_frame(payload)
        dead = []
        for conn, send_lock in list(self.ws_clients):
            try:
                with send_lock:
                    conn.sendall(frame)
            except OSError:
                dead.append((conn, send_lock))
        for client in dead:
            if client in self.ws_clients:
                self.ws_clients.remove(client)


system = FakeSystem()


def ticker_loop():
    while True:
        time.sleep(0.5)
        system.tick()
        system.broadcast()


def encode_ws_text_frame(text):
    payload = text.encode("utf-8")
    length = len(payload)
    if length <= 125:
        header = bytes([0x81, length])
    elif length <= 0xFFFF:
        header = bytes([0x81, 126]) + length.to_bytes(2, "big")
    else:
        header = bytes([0x81, 127]) + length.to_bytes(8, "big")
    return header + payload


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEB_DIR), **kwargs)

    def log_message(self, fmt, *args):
        pass  # silences request logging

    def end_headers(self):
        # files are edited while previewing, so no caching
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def _send_text(self, body, content_type="text/plain", status=200):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _read_form(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length else ""
        return parse_qs(body)

    def do_GET(self):
        if self.path == "/ws":
            self._handle_ws_upgrade()
            return
        if self.path == "/api/state":
            self._send_text(system.snapshot_json(), "application/json")
            return
        if self.path in ("/api/settings", "/api/settings/advanced", "/api/status"):
            self._send_text("device_name=Lasergate (dev preview)\n", "text/plain")
            return
        if self.path in PAGE_ROUTES:
            self.path = "/" + PAGE_ROUTES[self.path]
        super().do_GET()

    def do_HEAD(self):
        if self.path in PAGE_ROUTES:
            self.path = "/" + PAGE_ROUTES[self.path]
        super().do_HEAD()

    def do_POST(self):
        form = self._read_form()

        if self.path == "/api/state":
            requested = form.get("state", [""])[0]
            if requested in ALL_STATES:
                fault_reason = "Simulated fault for preview purposes" if requested == "FAULT" else ""
                system.set_state(requested, fault_reason)
                self._send_text("")
            else:
                self._send_text("unknown state", status=400)
            return

        if self.path.startswith("/api/modules/"):
            try:
                index = int(self.path.rsplit("/", 1)[-1])
                system.apply_module_form(index, form)
                self._send_text("")
            except (ValueError, IndexError):
                self._send_text("bad module index", status=400)
            return

        if self.path in ("/settings", "/settings/advanced"):
            self._send_text("")  # settings are discarded
            return

        if self.path == "/api/gpio-discovery/pin":
            pin = form.get("pin", [""])[0]
            level = form.get("level", [""])[0]
            if pin.isdigit() and level in ("0", "1"):
                system.set_gpio_level(int(pin), level == "1")
                self._send_text("")
            else:
                self._send_text("bad request", status=400)
            return

        self._send_text("not found", status=404)

    def _handle_ws_upgrade(self):
        key = self.headers.get("Sec-WebSocket-Key")
        if not key:
            self.send_error(400, "missing Sec-WebSocket-Key")
            return

        accept = base64.b64encode(hashlib.sha1((key + WS_MAGIC).encode()).digest()).decode()
        self.send_response(101, "Switching Protocols")
        self.send_header("Upgrade", "websocket")
        self.send_header("Connection", "Upgrade")
        self.send_header("Sec-WebSocket-Accept", accept)
        self.end_headers()

        send_lock = threading.Lock()
        client = (self.connection, send_lock)
        system.ws_clients.append(client)
        with send_lock:
            self.connection.sendall(encode_ws_text_frame(system.snapshot_json()))

        # push only, client frames are drained and discarded until it disconnects
        try:
            while self.connection.recv(1024):
                pass
        except OSError:
            pass
        finally:
            if client in system.ws_clients:
                system.ws_clients.remove(client)


class ThreadingHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

    threading.Thread(target=ticker_loop, daemon=True).start()

    server = ThreadingHTTPServer(("localhost", port), Handler)
    print(f"Lasergate web UI preview: http://localhost:{port}")
    print("Mock backend for UI previews")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
