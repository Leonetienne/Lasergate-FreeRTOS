const gateEl = document.getElementById('gate');

App.gateOnState(
    gateEl,
    'USER_ADJUSTING_BEAMS',
    '<span class="spinner"></span> Adjusting emitters'
);

document.getElementById('done-btn').addEventListener('click', () => App.requestState('DISARMED'));
