const MODULE_COUNT = 4;
const gateEl = document.getElementById('gate');
const grid = document.getElementById('module-grid');
const statusMsg = document.getElementById('status');
const saveAllBtn = document.getElementById('save-all-btn');

App.gateOnState(gateEl, 'DISARMED', 'Disarmed');

for (let i = 0; i < MODULE_COUNT; i++) {
    const card = document.createElement('div');
    card.className = 'module-card card';
    card.innerHTML = `
        <div class="module-head">
            <h3>Module ${i}</h3>
            <span class="pill" data-badge>-</span>
        </div>
        <form data-index="${i}">
            <div class="field-row">
                <div><label>Laser GPIO</label><input type="number" name="laser_gpio" min="0" max="48"></div>
                <div><label>Status LED GPIO</label><input type="number" name="led_gpio" min="0" max="48"></div>
            </div>
            <div class="field-row">
                <div><label>LDR GPIO</label><input type="number" name="ldr_gpio" min="0" max="48"></div>
                <div><label>LDR threshold</label><input type="number" name="ldr_thresh" min="0"></div>
            </div>
            <label>Pulse frequency (ms)</label>
            <input type="number" name="pulse_freq" min="0">
        </form>
    `;
    grid.appendChild(card);
    // block implicit submit on Enter
    card.querySelector('form').addEventListener('submit', (e) => e.preventDefault());
}

// saved together, one restart for all modules
saveAllBtn.addEventListener('click', async () => {
    statusMsg.className = 'status-msg show';
    statusMsg.textContent = 'Saving…';

    const forms = grid.querySelectorAll('form');
    const results = await Promise.all(Array.from(forms).map((form) =>
        fetch(`/api/modules/${form.dataset.index}`, { method: 'POST', body: new URLSearchParams(new FormData(form)) })
    ));

    const failed = results.findIndex(r => !r.ok);
    if (failed !== -1) {
        const reason = (await results[failed].text()).trim();
        statusMsg.textContent = `Module ${failed}: ${reason || 'save failed'}`;
        return;
    }

    statusMsg.textContent = 'Saved. Restarting…';
    await App.requestState('SHUTTING_DOWN');
    statusMsg.textContent = 'Restarting, reload in a few seconds.';
});

App.onUpdate((data) => {
    data.modules.forEach((m, i) => {
        const form = grid.querySelector(`form[data-index="${i}"]`);
        const badge = form.closest('.module-card').querySelector('[data-badge]');

        badge.textContent = m.configured ? (m.ready ? 'Ready' : 'Configured') : 'Unconfigured';
        badge.className = `pill ${m.configured ? (m.ready ? 'good' : 'accent') : ''}`;

        // keep values being edited
        if (document.activeElement && form.contains(document.activeElement)) return;

        form.laser_gpio.value = m.laser_gpio ?? '';
        form.led_gpio.value = m.led_gpio ?? '';
        form.ldr_gpio.value = m.ldr_gpio ?? '';
        form.ldr_thresh.value = m.ldr_threshold;
        form.pulse_freq.value = m.pulse_frequency_ms;
    });
});
