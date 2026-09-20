const gateEl = document.getElementById('gate');
const boardEl = document.getElementById('board');

App.gateOnState(
    gateEl,
    'DIAGNOSTIC_GPIO_DISCOVERY',
    'Click a pin to drive it high or low.'
);

document.getElementById('done-btn').addEventListener('click', () => App.requestState('DISARMED'));

// Waveshare ESP32-S3-ETH pinout rotated 90deg ccw from the silkscreen, usb-c on the left.
// layout derives from these rows and BOARD_*
const TOP_ROW = [33, 34, 'GND', 35, 36, 37, 38, 'GND', 39, 40, 41, 42, 'GND', 45, 46, 47, 48, 'GND', 19, 20];
const BOTTOM_ROW = [43, 44, 'GND', 0, 1, 2, 3, 'GND', 15, 'RUN', 18, 16, 'GND', 17, 21, '3V3', '3V3_EN', 'GND', 'VSYS', 'VBUS'];

// mirrors BoardReservedPins (isReserved/isInputOnly) and GpioDiscovery::getAdc1Pins()/getAdc2Pins().
// the remaining pins are getFreeOutputPins()
const GREYED_OUT_PINS = new Set([
    0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 19, 20,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 43, 44, 45, 46,
]);
const ADC1_PINS = new Set([1, 2]);
const ADC2_PINS = new Set([15, 16, 17, 18]);

const BOARD_W = 960;
const BOARD_H = 320;
const PIN_X0 = 130;
const PIN_DX = 38;
const PIN_R = 10;
const TOP_Y = 52;
const BOTTOM_Y = 286;

const DIGITAL_LOW = 'rgb(45,120,235)';
const DIGITAL_HIGH = 'rgb(225,60,60)';

function lerp(a, b, t) {
    return a + (b - a) * t;
}

function adcColor(t) {
    t = Math.max(0, Math.min(1, t));
    const dim = [74, 62, 20];
    const bright = [255, 214, 48];
    const [r, g, b] = dim.map((c, i) => Math.round(lerp(c, bright[i], t)));
    return `rgb(${r},${g},${b})`;
}

function pinDotId(gpio) {
    return `pin-${gpio}`;
}

function renderRow(row, y, labelBelow) {
    let svg = '';

    row.forEach((entry, i) => {
        const x = PIN_X0 + i * PIN_DX;
        const labelY = labelBelow ? y + 24 : y - 16;

        if (typeof entry !== 'number') {
            svg += `<circle cx="${x}" cy="${y}" r="3" class="pin-dot-passive"></circle>`;
            svg += `<text x="${x}" y="${labelY}" class="pin-label pin-label-passive" text-anchor="middle">${entry}</text>`;
            return;
        }

        const greyedOut = GREYED_OUT_PINS.has(entry);
        const isAdc = ADC1_PINS.has(entry) || ADC2_PINS.has(entry);
        const role = greyedOut ? 'reserved' : isAdc ? 'adc' : 'output';

        svg += `<circle id="${pinDotId(entry)}" cx="${x}" cy="${y}" r="${PIN_R}" class="pin-dot pin-dot-${role}" data-gpio="${entry}"></circle>`;
        svg += `<text x="${x}" y="${labelY}" class="pin-label" text-anchor="middle">IO${entry}</text>`;

        if (isAdc) {
            const valueY = labelBelow ? y + 38 : y - 30;
            svg += `<text id="${pinDotId(entry)}-value" x="${x}" y="${valueY}" class="pin-value" text-anchor="middle">-</text>`;
        }
    });

    return svg;
}

function buildBoardSvg() {
    return `<svg viewBox="0 0 ${BOARD_W} ${BOARD_H}" class="board-svg">
        <rect x="60" y="20" width="840" height="280" rx="14" class="board-body"></rect>
        <rect x="0" y="130" width="70" height="60" rx="8" class="board-usb"></rect>
        <text x="35" y="115" class="board-label" text-anchor="middle">USB-C</text>
        <rect x="430" y="120" width="100" height="80" rx="4" class="board-chip"></rect>
        <text x="480" y="165" class="board-chip-label" text-anchor="middle">ESP32-S3</text>
        <rect x="790" y="130" width="60" height="60" rx="4" class="board-sd"></rect>
        <text x="820" y="205" class="board-label" text-anchor="middle">SD</text>
        <text x="480" y="313" class="board-caption" text-anchor="middle">Waveshare ESP32-S3-ETH pinout</text>
        ${renderRow(TOP_ROW, TOP_Y, true)}
        ${renderRow(BOTTOM_ROW, BOTTOM_Y, false)}
    </svg>`;
}

boardEl.innerHTML = buildBoardSvg();

boardEl.querySelectorAll('.pin-dot-output').forEach((dot) => {
    dot.addEventListener('click', () => {
        setPinLevel(dot.dataset.gpio, !dot.classList.contains('is-high'));
    });
});

async function setPinLevel(pin, high) {
    await fetch('/api/gpio-discovery/pin', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `pin=${encodeURIComponent(pin)}&level=${high ? 1 : 0}`,
    });
}

function renderPins(pins) {
    for (const p of pins) {
        const dot = document.getElementById(pinDotId(p.gpio));
        if (!dot) continue;

        if (p.role === 'adc1' || p.role === 'adc2') {
            dot.style.fill = adcColor(p.adc_raw / 4095);
            const valueEl = document.getElementById(`${pinDotId(p.gpio)}-value`);
            if (valueEl) valueEl.textContent = p.adc_raw;
        } else {
            dot.classList.toggle('is-high', p.level);
            dot.style.fill = p.level ? DIGITAL_HIGH : DIGITAL_LOW;
        }
    }
}

App.onUpdate((data) => {
    if (data.state === 'DIAGNOSTIC_GPIO_DISCOVERY') {
        renderPins(data.gpio_pins);
    }
});
