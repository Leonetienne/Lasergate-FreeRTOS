const App = (() => {
    const STATE_LABELS = {
        NONE: 'Unknown',
        INITIALIZING: 'Initializing',
        USER_ADJUSTING_BEAMS: 'Adjusting emitters',
        CALIBRATION_LDR_THRESH: 'Calibrating LDR threshold',
        CALIBRATION_MODULATION_FREQUENCY: 'Calibrating pulse frequency',
        OBSERVING: 'Armed',
        DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST: 'Running self-test',
        DIAGNOSTIC_GPIO_DISCOVERY: 'GPIO discovery',
        DISARMED: 'Disarmed',
        ALARM: 'Alarm',
        FAULT: 'Fault',
        SHUTTING_DOWN: 'Shutting down',
    };

    // exclusive task states, other states are enterable once one finishes or faults
    const TASK_STATE_PAGES = {
        USER_ADJUSTING_BEAMS: '/emitters',
        CALIBRATION_LDR_THRESH: '/calibrate/ldr',
        CALIBRATION_MODULATION_FREQUENCY: '/calibrate/frequency',
        DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST: '/self-test',
        DIAGNOSTIC_GPIO_DISCOVERY: '/gpio-discovery',
    };

    let latest = null;
    let connected = false;
    const listeners = [];
    let ws = null;
    let reconnectDelay = 1000;

    function notify() {
        if (!latest) return;
        for (const cb of listeners) cb(latest, connected);
    }

    function onUpdate(cb) {
        listeners.push(cb);
        if (latest) cb(latest, connected);
    }

    function connectWs() {
        const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
        ws = new WebSocket(`${proto}//${location.host}/ws`);

        ws.onopen = () => {
            connected = true;
            reconnectDelay = 1000;
            notify();
        };
        ws.onmessage = (event) => {
            latest = JSON.parse(event.data);
            notify();
        };
        // onclose fires after errors too, so reconnect only here
        ws.onclose = () => {
            connected = false;
            notify();
            setTimeout(connectWs, reconnectDelay);
            reconnectDelay = Math.min(reconnectDelay * 2, 15000);
        };
    }

    function renderSidebarActive() {
        const path = location.pathname;
        document.querySelectorAll('.nav-item').forEach(a => {
            a.classList.toggle('active', a.getAttribute('href') === path);
        });
    }

    async function injectPartial(placeholderId, url) {
        const el = document.getElementById(placeholderId);
        if (!el) return;
        const res = await fetch(url);
        el.innerHTML = await res.text();
    }

    async function requestState(stateName) {
        const res = await fetch('/api/state', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `state=${encodeURIComponent(stateName)}`,
        });
        return res.ok;
    }

    async function init() {
        await Promise.all([
            injectPartial('sidebar', '/sidebar.html'),
        ]);
        renderSidebarActive();
        connectWs();
    }

    document.addEventListener('DOMContentLoaded', init);

    return { onUpdate, requestState, STATE_LABELS, TASK_STATE_PAGES };
})();
