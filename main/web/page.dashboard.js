const statusEl = document.getElementById('status');
const iconEl = document.getElementById('status-icon');
const headlineEl = document.getElementById('status-headline');
const subEl = document.getElementById('status-sub');
const armBtn = document.getElementById('arm-btn');
const disarmBtn = document.getElementById('disarm-btn');
const stopBtn = document.getElementById('stop-alarm-btn');
const gotoBtn = document.getElementById('status-goto-btn');

const PRESENTATION = {
    DISARMED: { cls: 'disarmed', icon: '/icons/status-disarmed.png', headline: 'Ready', sub: 'Disarmed' },
    OBSERVING: { cls: 'armed', icon: '/icons/status-armed.png', headline: 'Ready', sub: 'Armed' },
    ALARM: { cls: 'alarm', icon: '/icons/status-alarm.png', headline: 'ALARM', sub: 'Intrusion detected' },
};

App.onUpdate((data) => {
    const p = PRESENTATION[data.state] || {
        cls: 'other',
        icon: '/icons/headbar-state.png',
        headline: App.STATE_LABELS[data.state] || data.state,
        sub: 'System busy',
    };

    statusEl.className = `dashboard-status ${p.cls}`;
    iconEl.src = p.icon;
    headlineEl.textContent = p.headline;
    subEl.textContent = data.state === 'FAULT' && data.fault_reason ? data.fault_reason : p.sub;

    armBtn.hidden = data.state !== 'DISARMED';
    armBtn.disabled = !data.modules.some(m => m.ready);
    armBtn.title = armBtn.disabled ? 'No module is ready' : '';
    disarmBtn.hidden = data.state !== 'OBSERVING';
    stopBtn.hidden = data.state !== 'ALARM';

    const busyPage = App.TASK_STATE_PAGES[data.state];
    gotoBtn.hidden = !busyPage;
    if (busyPage) gotoBtn.href = busyPage;
});

armBtn.addEventListener('click', () => App.requestState('OBSERVING'));
disarmBtn.addEventListener('click', () => App.requestState('DISARMED'));
stopBtn.addEventListener('click', () => App.requestState('DISARMED'));
