/* ==========================================================================
   RW-Bench 2026 - Client Application Logic
   ========================================================================== */

(function () {
  'use strict';

  // Elements
  const driveSelect = document.getElementById('drive-select');
  const driveFs = document.getElementById('drive-fs');
  const driveType = document.getElementById('drive-type');
  const driveCapacity = document.getElementById('drive-capacity');
  const driveSpaceFill = document.getElementById('drive-space-fill');

  const btnStart = document.getElementById('btn-start');
  const btnStop = document.getElementById('btn-stop');
  const btnExport = document.getElementById('btn-export');

  const statusDot = document.getElementById('status-dot');
  const statusText = document.getElementById('status-text');

  const valSpeedInstant = document.getElementById('val-speed-instant');
  const valActivePhase = document.getElementById('val-active-phase');
  const gaugeFill = document.getElementById('gauge-fill');

  const metricInstant = document.getElementById('metric-instant');
  const metricAverage = document.getElementById('metric-average');
  const metricPeak = document.getElementById('metric-peak');
  const metricIops = document.getElementById('metric-iops');

  const progressBar = document.getElementById('main-progress-bar');
  const resultsBody = document.getElementById('results-body');
  const canvas = document.getElementById('speed-chart');
  const ctx = canvas.getContext('2d');

  // State
  let drivesData = [];
  let selectedDrive = null;
  let testSizeBytes = 268435456; // 256 MB default
  let isBenchmarking = false;
  let pollInterval = null;
  let eventSource = null;
  let chartSamples = []; // {t, inst, avg}
  let maxObservedSpeed = 100.0;
  let completedResults = [];

  // ================= Drive Enumeration =================
  async function loadDrives() {
    try {
      const res = await fetch('/api/drives');
      if (!res.ok) throw new Error('Drive API error');
      drivesData = await res.json();

      driveSelect.innerHTML = '';
      if (drivesData.length === 0) {
        driveSelect.innerHTML = '<option value="" disabled>No writable storage found</option>';
        return;
      }

      drivesData.forEach((d, idx) => {
        const opt = document.createElement('option');
        opt.value = d.path;
        const totalGB = (d.totalBytes / (1024 ** 3)).toFixed(1);
        const freeGB = (d.freeBytes / (1024 ** 3)).toFixed(1);
        const label = d.volumeName ? `"${d.volumeName}"` : 'Local Disk';
        opt.textContent = `${d.path} ${label} (${d.fileSystem}) - ${freeGB} GB free of ${totalGB} GB`;
        if (idx === 0) opt.selected = true;
        driveSelect.appendChild(opt);
      });

      updateDriveCard(drivesData[0]);
    } catch (err) {
      console.error('Failed to load drives:', err);
      statusText.textContent = 'Drive Scan Error';
      statusDot.className = 'status-dot error';
    }
  }

  function updateDriveCard(drive) {
    if (!drive) return;
    selectedDrive = drive;
    driveFs.textContent = drive.fileSystem || 'Unknown';
    driveType.textContent = drive.driveType || 'Storage Device';

    const freeGB = (drive.freeBytes / (1024 ** 3)).toFixed(1);
    const totalGB = (drive.totalBytes / (1024 ** 3)).toFixed(1);
    driveCapacity.textContent = `${freeGB} GB / ${totalGB} GB`;

    const usedBytes = drive.totalBytes - drive.freeBytes;
    const usedPct = drive.totalBytes > 0 ? Math.min(100, Math.max(0, (usedBytes / drive.totalBytes) * 100)) : 0;
    driveSpaceFill.style.width = `${usedPct.toFixed(1)}%`;
  }

  driveSelect.addEventListener('change', (e) => {
    const d = drivesData.find((x) => x.path === e.target.value);
    if (d) updateDriveCard(d);
  });

  // Size pills
  document.querySelectorAll('.pill-btn').forEach((btn) => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.pill-btn').forEach((b) => b.classList.remove('active'));
      btn.classList.add('active');
      testSizeBytes = parseInt(btn.dataset.size, 10);
    });
  });

  // Mode radio selection cards
  document.querySelectorAll('.mode-card input').forEach((radio) => {
    radio.addEventListener('change', () => {
      document.querySelectorAll('.mode-card').forEach((c) => c.classList.remove('active'));
      radio.closest('.mode-card').classList.add('active');
    });
  });

  // ================= Canvas Chart Setup =================
  function resizeCanvas() {
    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);
  }

  window.addEventListener('resize', () => {
    resizeCanvas();
    drawChart();
  });
  resizeCanvas();

  function drawChart() {
    const width = canvas.getBoundingClientRect().width;
    const height = canvas.getBoundingClientRect().height;

    ctx.clearRect(0, 0, width, height);

    // Draw background grid lines
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
    ctx.lineWidth = 1;

    const gridLines = 4;
    for (let i = 0; i <= gridLines; i++) {
      const y = (height / gridLines) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();

      // Speed labels
      const speedVal = maxObservedSpeed - (maxObservedSpeed / gridLines) * i;
      ctx.fillStyle = 'rgba(255, 255, 255, 0.25)';
      ctx.font = '10px JetBrains Mono';
      ctx.fillText(`${speedVal.toFixed(0)} MB/s`, 8, y - 4);
    }

    if (chartSamples.length < 2) return;

    // Scale dynamically
    const currentMax = Math.max(...chartSamples.map((s) => s.inst), ...chartSamples.map((s) => s.avg), 50);
    maxObservedSpeed = Math.max(maxObservedSpeed * 0.95, currentMax * 1.15);

    // Compute coordinates
    const stepX = width / (chartSamples.length - 1);

    // Fill gradient for Instant Speed
    const gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, 'rgba(0, 229, 255, 0.35)');
    gradient.addColorStop(1, 'rgba(0, 229, 255, 0.0)');

    ctx.beginPath();
    ctx.moveTo(0, height);
    for (let i = 0; i < chartSamples.length; i++) {
      const x = i * stepX;
      const y = height - (chartSamples[i].inst / maxObservedSpeed) * height;
      ctx.lineTo(x, y);
    }
    ctx.lineTo(width, height);
    ctx.closePath();
    ctx.fillStyle = gradient;
    ctx.fill();

    // Stroke Instant Speed (Cyan)
    ctx.beginPath();
    for (let i = 0; i < chartSamples.length; i++) {
      const x = i * stepX;
      const y = height - (chartSamples[i].inst / maxObservedSpeed) * height;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = '#00e5ff';
    ctx.lineWidth = 2.5;
    ctx.stroke();

    // Stroke Average Speed (Emerald dashed)
    ctx.beginPath();
    ctx.setLineDash([4, 4]);
    for (let i = 0; i < chartSamples.length; i++) {
      const x = i * stepX;
      const y = height - (chartSamples[i].avg / maxObservedSpeed) * height;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = '#00e676';
    ctx.lineWidth = 2;
    ctx.stroke();
    ctx.setLineDash([]);

    // Glowing head point
    const lastSample = chartSamples[chartSamples.length - 1];
    const headX = (chartSamples.length - 1) * stepX;
    const headY = height - (lastSample.inst / maxObservedSpeed) * height;

    ctx.beginPath();
    ctx.arc(headX, headY, 5, 0, Math.PI * 2);
    ctx.fillStyle = '#00e5ff';
    ctx.shadowColor = '#00e5ff';
    ctx.shadowBlur = 12;
    ctx.fill();
    ctx.shadowBlur = 0;
  }

  // ================= Speedometer Gauge =================
  function updateGauge(speedMBps) {
    const totalArc = 251.32; // stroke-dasharray for r=80 semi-circle
    const maxGaugeSpeed = Math.max(500, maxObservedSpeed);
    const fraction = Math.min(1.0, Math.max(0.0, speedMBps / maxGaugeSpeed));
    const offset = totalArc * (1.0 - fraction);
    gaugeFill.style.strokeDashoffset = offset.toFixed(2);
  }

  // ================= Live Status Updates =================
  function applyStatus(data) {
    isBenchmarking = data.isRunning;

    if (data.isRunning) {
      statusDot.className = 'status-dot running';
      statusText.textContent = `Running: ${data.activeTest}`;
      btnStart.disabled = true;
      btnStop.disabled = false;
      valActivePhase.textContent = data.activeTest.toUpperCase();
    } else {
      btnStart.disabled = false;
      btnStop.disabled = true;
      if (data.phase === 'Completed') {
        statusDot.className = 'status-dot';
        statusText.textContent = 'Benchmark Finished';
        valActivePhase.textContent = 'FINISHED';
      } else if (data.phase === 'Aborted') {
        statusDot.className = 'status-dot';
        statusText.textContent = 'Benchmark Aborted';
        valActivePhase.textContent = 'ABORTED';
      } else if (data.phase === 'Error') {
        statusDot.className = 'status-dot error';
        statusText.textContent = data.errorMessage || 'Error Occurred';
        valActivePhase.textContent = 'ERROR';
      }
    }

    // Digital readouts
    valSpeedInstant.textContent = data.instantMBps.toFixed(1);
    metricInstant.innerHTML = `${data.instantMBps.toFixed(1)} <span class="unit">MB/s</span>`;
    metricAverage.innerHTML = `${data.averageMBps.toFixed(1)} <span class="unit">MB/s</span>`;
    metricPeak.innerHTML = `${data.peakMBps.toFixed(1)} <span class="unit">MB/s</span>`;
    metricIops.innerHTML = `${data.currentIOPS.toLocaleString()} <span class="unit">IOPS</span>`;

    updateGauge(data.instantMBps);
    progressBar.style.width = `${data.progress.toFixed(1)}%`;

    // History for chart
    if (data.history && data.history.length > 0) {
      chartSamples = data.history;
      requestAnimationFrame(drawChart);
    }

    // Results table
    if (data.completedResults && data.completedResults.length > 0) {
      completedResults = data.completedResults;
      renderResultsTable(completedResults);
    }
  }

  function renderResultsTable(results) {
    resultsBody.innerHTML = '';
    results.forEach((r) => {
      const row = document.createElement('tr');
      row.innerHTML = `
        <td style="font-weight: 700; color: #00e5ff;">${r.testName}</td>
        <td style="color: #00e676;">${r.avgMBps.toFixed(1)} MB/s</td>
        <td>${r.peakMBps.toFixed(1)} MB/s</td>
        <td>${r.avgIOPS.toLocaleString()}</td>
        <td>${r.avgLatencyMs.toFixed(3)} ms</td>
        <td>${r.durationSec.toFixed(2)} s</td>
      `;
      resultsBody.appendChild(row);
    });
  }

  // ================= Benchmark Control =================
  async function startBenchmark() {
    if (!selectedDrive) {
      alert('Please select a storage drive first.');
      return;
    }

    const modeRadio = document.querySelector('input[name="bench-mode"]:checked');
    const mode = modeRadio ? modeRadio.value : 'full';

    chartSamples = [];
    maxObservedSpeed = 100.0;
    progressBar.style.width = '0%';
    drawChart();

    try {
      const res = await fetch('/api/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          drive: selectedDrive.path,
          mode: mode,
          size: testSizeBytes,
          blockSize: 1048576, // 1 MB
        }),
      });

      if (!res.ok) throw new Error('Start failed');
      startPolling();
    } catch (err) {
      console.error('Failed to start benchmark:', err);
      alert('Could not start benchmark: ' + err.message);
    }
  }

  async function stopBenchmark() {
    try {
      await fetch('/api/stop', { method: 'POST' });
    } catch (err) {
      console.error('Failed to abort:', err);
    }
  }

  function startPolling() {
    if (pollInterval) clearInterval(pollInterval);
    pollInterval = setInterval(async () => {
      try {
        const res = await fetch('/api/status');
        if (res.ok) {
          const status = await res.json();
          applyStatus(status);
          if (!status.isRunning) {
            clearInterval(pollInterval);
            pollInterval = null;
          }
        }
      } catch (e) {
        console.warn('Poll error:', e);
      }
    }, 90);
  }

  btnStart.addEventListener('click', startBenchmark);
  btnStop.addEventListener('click', stopBenchmark);

  // Export report to JSON
  btnExport.addEventListener('click', () => {
    if (completedResults.length === 0) {
      alert('No benchmark results to export yet.');
      return;
    }
    const report = {
      generator: 'RW-Bench 2026 Edition',
      timestamp: new Date().toISOString(),
      drive: selectedDrive,
      testSizeMegabytes: (testSizeBytes / (1024 * 1024)).toFixed(0),
      results: completedResults,
    };
    const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `rwbench-report-${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  });

  // Initial load
  loadDrives();
})();
