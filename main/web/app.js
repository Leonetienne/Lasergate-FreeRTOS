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
    let heartbeatTimer = null;
    const HEARTBEAT_INTERVAL_MS = 5000;

    function notify() {
        if (!latest) return;
        for (const cb of listeners) cb(latest, connected);
        renderHeadbar();
        renderFaultBanner();
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
            // server evicts the least recently used socket, a push-only ws would always be first
            heartbeatTimer = setInterval(() => ws.send('ping'), HEARTBEAT_INTERVAL_MS);
            notify();
        };
        ws.onmessage = (event) => {
            latest = JSON.parse(event.data);
            notify();
        };
        // onclose fires after errors too, so reconnect only here
        ws.onclose = () => {
            connected = false;
            clearInterval(heartbeatTimer);
            notify();
            setTimeout(connectWs, reconnectDelay);
            reconnectDelay = Math.min(reconnectDelay * 2, 15000);
        };
    }

    function formatUptime(ms) {
        const totalSeconds = Math.floor(ms / 1000);
        const h = Math.floor(totalSeconds / 3600);
        const m = Math.floor((totalSeconds % 3600) / 60);
        const s = totalSeconds % 60;
        return `${h}h ${String(m).padStart(2, '0')}m ${String(s).padStart(2, '0')}s`;
    }

    function renderHeadbar() {
        const bar = document.getElementById('headbar');
        if (!bar || !latest) return;

        bar.querySelector('.hb-uptime').textContent = formatUptime(latest.uptime_ms);
        bar.querySelector('.hb-state').textContent = STATE_LABELS[latest.state] || latest.state;

        const configured = latest.modules.filter(m => m.configured).length;
        bar.querySelector('.hb-modules').textContent = `${configured} / ${latest.modules.length}`;

        const dot = bar.querySelector('.conn-dot');
        dot.classList.toggle('live', connected);
        dot.title = connected ? 'Live' : 'Reconnecting…';
    }

    // FAULT persists until a reboot
    function renderFaultBanner() {
        const headbar = document.getElementById('headbar');
        if (!headbar || !latest) return;

        let banner = document.getElementById('fault-banner');
        if (latest.state !== 'FAULT') {
            if (banner) banner.hidden = true;
            return;
        }

        if (!banner) {
            banner = document.createElement('div');
            banner.id = 'fault-banner';
            banner.className = 'fault-banner';
            banner.innerHTML = `
                <div class="fault-text">
                    <strong>System fault</strong>
                    <span>Reboot to recover. Settings can be reset under <a href="/config">Configuration</a>.</span>
                    <span class="fault-reason"></span>
                </div>
                <button class="danger" type="button">Reboot</button>
            `;
            banner.querySelector('button').addEventListener('click', async (e) => {
                const button = e.currentTarget;
                button.disabled = true;
                button.textContent = 'Rebooting…';
                if (!(await requestState('SHUTTING_DOWN'))) {
                    button.disabled = false;
                    button.textContent = 'Reboot';
                }
            });
            headbar.insertAdjacentElement('afterend', banner);
        }

        banner.hidden = false;
        banner.querySelector('.fault-reason').textContent = latest.fault_reason
            ? `Reason: ${latest.fault_reason}`
            : 'No reason recorded';
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

    // gated-content is live in targetState. elsewhere it links the running task's page,
    // or offers an enter button while the transition is allowed
    function gateOnState(el, targetState, waitingText) {
        const gate = el.querySelector('.state-gate');
        const button = gate.querySelector('button');
        const label = gate.querySelector('.label');

        onUpdate((data) => {
            const active = data.state === targetState;
            el.dataset.active = String(active);

            if (active) {
                label.innerHTML = waitingText;
                button.hidden = true;
                return;
            }

            const busyPage = TASK_STATE_PAGES[data.state];
            if (busyPage && data.state !== targetState) {
                label.innerHTML = `System busy: <strong>${STATE_LABELS[data.state]}</strong>. ` +
                    `<a href="${busyPage}">View progress</a>`;
                button.hidden = true;
                return;
            }

            label.innerHTML = `State: <strong>${STATE_LABELS[data.state]}</strong>`;
            button.hidden = false;
            button.disabled = !data.allowed_transitions.includes(targetState);
            button.title = button.disabled ? `Not available from ${STATE_LABELS[data.state]}` : '';
        });

        button.addEventListener('click', () => requestState(targetState));
    }

    async function init() {
        await Promise.all([
            injectPartial('sidebar', '/sidebar.html'),
            injectPartial('headbar', '/headbar.html'),
        ]);
        renderSidebarActive();
        connectWs();
    }

    document.addEventListener('DOMContentLoaded', init);

    return { onUpdate, requestState, gateOnState, STATE_LABELS, TASK_STATE_PAGES, formatUptime };
})();
