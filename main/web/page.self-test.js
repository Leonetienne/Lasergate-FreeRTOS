const gateEl = document.getElementById('gate');
const resultsBody = document.getElementById('results-body');

App.gateOnState(
    gateEl,
    'DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST',
    '<span class="spinner"></span> Running self-test'
);

App.onUpdate((data) => {
    resultsBody.innerHTML = data.modules.map((m, i) => {
        if (!m.configured) {
            return `<tr><td>Module ${i}</td><td><span class="pill">Unconfigured</span></td><td>-</td></tr>`;
        }
        const status = m.self_test_finished
            ? '<span class="pill good">Finished</span>'
            : '<span class="pill accent">Running</span>';
        const errors = m.self_test_error_count ?? '-';
        return `<tr><td>Module ${i}</td><td>${status}</td><td>${errors}</td></tr>`;
    }).join('');
});
