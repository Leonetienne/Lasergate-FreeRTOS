const gateEl = document.getElementById('gate');
const statusMsg = document.getElementById('status');
const settingsForm = document.getElementById('settings-form');
const advancedForm = document.getElementById('advanced-form');

App.gateOnState(gateEl, 'DISARMED', 'Disarmed');

function parseKeyValues(text) {
    const values = {};
    text.trim().split('\n').filter(Boolean).forEach(line => {
        const i = line.indexOf('=');
        values[line.slice(0, i)] = line.slice(i + 1);
    });
    return values;
}

async function loadSettings() {
    const values = parseKeyValues(await (await fetch('/api/settings')).text());
    settingsForm.device_name.value = values.device_name || '';
    settingsForm.mqtt_uri.value = values.mqtt_uri || '';
    settingsForm.mqtt_user.value = values.mqtt_user || '';
    settingsForm.mqtt_pass.placeholder = values.mqtt_password_set === '1' ? 'Blank keeps current' : '';
    settingsForm.node_id.value = values.node_id || '';
}

async function loadAdvanced() {
    const values = parseKeyValues(await (await fetch('/api/settings/advanced')).text());
    advancedForm.ethernet_led_gpio.value = values.ethernet_led_gpio || '';
    advancedForm.mqtt_led_gpio.value = values.mqtt_led_gpio || '';
    advancedForm.enable_conn_leds.checked = values.conn_leds_enabled !== '0';
}

function showSaving() {
    statusMsg.className = 'status-msg show';
    statusMsg.textContent = 'Saving & restarting…';
}

settingsForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    showSaving();
    await fetch('/settings', { method: 'POST', body: new URLSearchParams(new FormData(settingsForm)) });
    statusMsg.textContent = 'Saved. Restarting, reload in a few seconds.';
});

advancedForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    showSaving();
    const res = await fetch('/settings/advanced', { method: 'POST', body: new URLSearchParams(new FormData(advancedForm)) });
    if (!res.ok) {
        statusMsg.textContent = (await res.text()).trim() || 'Save failed';
        return;
    }
    statusMsg.textContent = 'Saved. Restarting, reload in a few seconds.';
});

const resetBtn = document.getElementById('reset-btn');
const resetStatusMsg = document.getElementById('reset-status');

App.onUpdate((data) => {
    const allowed = data.state === 'DISARMED' || data.state === 'FAULT';
    resetBtn.disabled = !allowed;
    resetBtn.title = allowed ? '' : `Not available from ${App.STATE_LABELS[data.state]}`;
});

resetBtn.addEventListener('click', async () => {
    if (!confirm('Erase all settings and restart?')) return;
    resetStatusMsg.className = 'status-msg show';
    resetStatusMsg.textContent = 'Resetting & restarting…';
    const res = await fetch('/api/settings/reset', { method: 'POST' });
    if (!res.ok) {
        resetStatusMsg.textContent = (await res.text()).trim() || 'Reset failed';
        return;
    }
    resetStatusMsg.textContent = 'Settings reset. Restarting, reload in a few seconds.';
});

loadSettings();
loadAdvanced();
