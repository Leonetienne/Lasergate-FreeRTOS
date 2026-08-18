# Lasergate

Lasergate is a PoE-powered light-barrier device built on the ESP32-S3, meant to watch doorways or other openings and report when something passes through.
Instead of a camera, it uses pairs of modulated lasers and LDRs. No image data, no object detection model, no privacy questions to answer.

This is a hobby project I'm building in my spare time, not a finished product. Firmware, mechanics and electronics are all still in flux.

## Status

**Work in progress.** What exists right now:

- Firmware scaffolding on ESP-IDF/FreeRTOS, targeting the ESP32-S3
- A basic application architecture (state machine, settings storage, HTTP API/web UI, HAL layers for GPIO, ADC, Ethernet, MQTT),
- with unit tests for the hardware abstraction layer
- Mechanical prototypes for the sensor modules
- A first electronics prototype: ESP32-S3 with a PoE HAT, breadboarded laser/LDR module

## How it's meant to work

Each sensor is a laser/LDR pair, called a module. A gate is made up of however many modules you need to cover an opening.

The laser is pulsed with a pseudo-random pattern instead of just being on or off, and the receiving LDR checks the incoming signal against the expected pattern
rather than reading a plain brightness threshold. That should make the thing harder to fool with a flashlight or a well-timed hand wave than a basic light barrier would be. A long, narrow black tube in front of each LDR blocks stray light from the sides so the sensor mostly only sees its own laser.

Modules are meant to calibrate themselves to ambient light rather than needing manual tuning, and the trigger condition
(how many modules need to be blocked at once, and for how long) is meant to be configurable per gate.

## Interfaces

Planned: an HTTP API/webhook and CAN bus for linking multiple units together.

Currently working: a small HTTP-served web UI for basic and advanced settings, backed by a status/settings API. MQTT support is implemented but not yet used by the application.

## Hardware / build photos

| Fifth-iteration laser/LDR module prototype | Glare-shield tube prototype, ESP32 nodemcu (not yet S3) | First PoE board, ESP32-S3 with W5500 |
|:---:|:---:|:---:|
| ![Laser/LDR module prototype wired on a cutting mat](https://raw.githubusercontent.com/Leonetienne/Lasergate-FreeRTOS/master/github-assets/PXL_20260609_192337563.jpg) | ![Glare-shield tube prototype and ESP32 nodemcu breadboard setup](https://raw.githubusercontent.com/Leonetienne/Lasergate-FreeRTOS/master/github-assets/PXL_20260611_215554151.jpg) | ![PoE board with ESP32-S3 and W5500 module](https://raw.githubusercontent.com/Leonetienne/Lasergate-FreeRTOS/master/github-assets/PXL_20260617_220122405.jpg) |

## License

[GPLv3](LICENSE)
