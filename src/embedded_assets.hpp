#pragma once

namespace EmbeddedAssets {
    inline const char INDEX_HTML[] = R"rwbench_html(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RW-Bench | Modern Disk Speed Benchmarker</title>
  <link rel="stylesheet" href="/style.css">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800&family=JetBrains+Mono:wght@400;500;600&display=swap" rel="stylesheet">
</head>
<body class="dark-theme">
  <div class="app-container">
    <!-- Top Header -->
    <header class="app-header">
      <div class="brand">
        <div class="brand-icon">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M22 12h-4l-3 9L9 3l-3 9H2" />
          </svg>
        </div>
        <div class="brand-text">
          <h1>RW-BENCH <span class="badge-tag">2026 EDITION</span></h1>
          <p class="subtitle">Direct I/O Hardware Storage Benchmark &bull; Windows Low-Level Engine</p>
        </div>
      </div>
      <div class="header-status">
        <div class="status-indicator" id="status-pill">
          <span class="status-dot" id="status-dot"></span>
          <span id="status-text">Engine Ready</span>
        </div>
      </div>
    </header>

    <!-- Main Grid Layout -->
    <main class="dashboard-grid">
      <!-- Left Column: Controls & Drive Selector -->
      <section class="panel control-panel">
        <div class="panel-header">
          <svg class="panel-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor">
            <rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect>
            <rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect>
            <line x1="6" y1="6" x2="6.01" y2="6"></line>
            <line x1="6" y1="18" x2="6.01" y2="18"></line>
          </svg>
          <h2>Storage Configuration</h2>
        </div>

        <div class="control-group">
          <label class="control-label" for="drive-select">Target Storage Drive</label>
          <div class="custom-select-wrapper">
            <select id="drive-select" class="material-select">
              <option value="" disabled selected>Scanning drives...</option>
            </select>
          </div>
          <div class="drive-details-card" id="drive-card">
            <div class="drive-stat-row">
              <span class="label">File System:</span>
              <span class="val badge-fs" id="drive-fs">-</span>
            </div>
            <div class="drive-stat-row">
              <span class="label">Drive Type:</span>
              <span class="val" id="drive-type">-</span>
            </div>
            <div class="drive-stat-row">
              <span class="label">Free / Total:</span>
              <span class="val" id="drive-capacity">-</span>
            </div>
            <div class="storage-bar-track">
              <div class="storage-bar-fill" id="drive-space-fill" style="width: 0%"></div>
            </div>
          </div>
        </div>

        <div class="control-group">
          <label class="control-label">Benchmark Routine</label>
          <div class="mode-grid">
            <label class="mode-card active">
              <input type="radio" name="bench-mode" value="full" checked>
              <div class="mode-content">
                <span class="mode-title">Full Suite</span>
                <span class="mode-desc">Seq Read/Write + Random 4K</span>
              </div>
            </label>
            <label class="mode-card">
              <input type="radio" name="bench-mode" value="seq-write">
              <div class="mode-content">
                <span class="mode-title">Seq Write</span>
                <span class="mode-desc">Direct write throughput</span>
              </div>
            </label>
            <label class="mode-card">
              <input type="radio" name="bench-mode" value="seq-read">
              <div class="mode-content">
                <span class="mode-title">Seq Read</span>
                <span class="mode-desc">Direct read throughput</span>
              </div>
            </label>
            <label class="mode-card">
              <input type="radio" name="bench-mode" value="rnd-4k">
              <div class="mode-content">
                <span class="mode-title">Random 4K</span>
                <span class="mode-desc">IOPS & Latency stress</span>
              </div>
            </label>
          </div>
        </div>

        <div class="control-group">
          <label class="control-label">Test File Size</label>
          <div class="pill-group">
            <button type="button" class="pill-btn" data-size="134217728">128 MB</button>
            <button type="button" class="pill-btn active" data-size="268435456">256 MB</button>
            <button type="button" class="pill-btn" data-size="536870912">512 MB</button>
            <button type="button" class="pill-btn" data-size="1073741824">1 GB</button>
          </div>
        </div>

        <div class="action-buttons">
          <button id="btn-start" class="btn btn-primary">
            <svg viewBox="0 0 24 24" fill="currentColor">
              <polygon points="5 3 19 12 5 21 5 3"></polygon>
            </svg>
            <span>Start Live Benchmark</span>
          </button>
          <button id="btn-stop" class="btn btn-secondary" disabled>
            <svg viewBox="0 0 24 24" fill="currentColor">
              <rect x="6" y="6" width="12" height="12"></rect>
            </svg>
            <span>Abort</span>
          </button>
        </div>
      </section>

      <!-- Center & Right: Live Visuals & Metrics -->
      <section class="panel visuals-panel">
        <div class="visuals-top-row">
          <!-- Live Gauge -->
          <div class="gauge-container">
            <svg class="speedometer-svg" viewBox="0 0 200 160">
              <defs>
                <linearGradient id="gaugeGradient" x1="0%" y1="0%" x2="100%" y2="0%">
                  <stop offset="0%" stop-color="#00e5ff" />
                  <stop offset="50%" stop-color="#00e676" />
                  <stop offset="100%" stop-color="#ff007f" />
                </linearGradient>
              </defs>
              <path class="gauge-bg" d="M 20 140 A 80 80 0 0 1 180 140" fill="none" stroke="rgba(255,255,255,0.08)" stroke-width="16" stroke-linecap="round" />
              <path id="gauge-fill" class="gauge-fill" d="M 20 140 A 80 80 0 0 1 180 140" fill="none" stroke="url(#gaugeGradient)" stroke-width="16" stroke-linecap="round" stroke-dasharray="251.32" stroke-dashoffset="251.32" />
            </svg>
            <div class="gauge-readout">
              <div class="live-speed-value" id="val-speed-instant">0.0</div>
              <div class="live-speed-unit">MB / s</div>
              <div class="live-test-phase" id="val-active-phase">READY</div>
            </div>
          </div>

          <!-- Key Metric Cards -->
          <div class="metrics-cards-grid">
            <div class="metric-card cyan-card">
              <div class="metric-title">Instantaneous</div>
              <div class="metric-value" id="metric-instant">0.0 <span class="unit">MB/s</span></div>
            </div>
            <div class="metric-card emerald-card">
              <div class="metric-title">Average Throughput</div>
              <div class="metric-value" id="metric-average">0.0 <span class="unit">MB/s</span></div>
            </div>
            <div class="metric-card purple-card">
              <div class="metric-title">Peak Speed</div>
              <div class="metric-value" id="metric-peak">0.0 <span class="unit">MB/s</span></div>
            </div>
            <div class="metric-card amber-card">
              <div class="metric-title">Current IOPS</div>
              <div class="metric-value" id="metric-iops">0 <span class="unit">IOPS</span></div>
            </div>
          </div>
        </div>

        <!-- Real-Time 60FPS Speed Wave Chart -->
        <div class="chart-container">
          <div class="chart-header">
            <div class="chart-title">Real-Time Throughput Stream (Direct I/O)</div>
            <div class="chart-legend">
              <span class="legend-dot cyan"></span> Instant Speed
              <span class="legend-dot emerald"></span> Average
            </div>
          </div>
          <div class="canvas-wrapper">
            <canvas id="speed-chart"></canvas>
          </div>
          <div class="progress-bar-container">
            <div class="progress-bar-fill" id="main-progress-bar" style="width: 0%"></div>
          </div>
        </div>

        <!-- Benchmark Completed Results Table -->
        <div class="results-table-container">
          <div class="results-header">
            <h3>Completed Benchmark Results</h3>
            <button id="btn-export" class="btn-text">Export Report (JSON)</button>
          </div>
          <table class="results-table">
            <thead>
              <tr>
                <th>Test Routine</th>
                <th>Avg Speed</th>
                <th>Peak Speed</th>
                <th>IOPS</th>
                <th>Avg Latency</th>
                <th>Duration</th>
              </tr>
            </thead>
            <tbody id="results-body">
              <tr>
                <td colspan="6" class="empty-state">No tests run yet. Select a routine and click "Start Live Benchmark".</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>
    </main>

    <!-- Footer -->
    <footer class="app-footer">
      <span>RW-BENCH v1.0.0 &bull; MilkyCloud Technologies</span>
      <span class="footer-note">Unbuffered Direct I/O guarantees physical storage performance measurements</span>
    </footer>
  </div>

  <script src="/app.js"></script>
</body>
</html>
)rwbench_html";
    inline const char STYLE_CSS[] = R"rwbench_css(/* ==========================================================================
   2026 Modern Material 3 Dark Theme - RW-Bench
   ========================================================================== */

:root {
  --bg-base: #090c10;
  --bg-surface: #101520;
  --bg-surface-elevated: #161c2b;
  --bg-surface-card: #1c2336;
  
  --border-subtle: rgba(255, 255, 255, 0.08);
  --border-focus: #00e5ff;
  
  --text-primary: #f0f4fc;
  --text-secondary: #9aa8c2;
  --text-tertiary: #5e6c84;
  
  --neon-cyan: #00e5ff;
  --neon-cyan-glow: rgba(0, 229, 255, 0.25);
  --neon-emerald: #00e676;
  --neon-emerald-glow: rgba(0, 230, 118, 0.25);
  --neon-purple: #b388ff;
  --neon-purple-glow: rgba(179, 136, 255, 0.25);
  --neon-amber: #ffd600;
  --neon-amber-glow: rgba(255, 214, 0, 0.25);
  --neon-rose: #ff1744;
  
  --font-sans: 'Outfit', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  --font-mono: 'JetBrains Mono', monospace;
  
  --radius-sm: 8px;
  --radius-md: 14px;
  --radius-lg: 20px;
}

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body.dark-theme {
  background-color: var(--bg-base);
  color: var(--text-primary);
  font-family: var(--font-sans);
  min-height: 100vh;
  overflow-x: hidden;
  background-image: 
    radial-gradient(circle at 15% 15%, rgba(0, 229, 255, 0.04) 0%, transparent 40%),
    radial-gradient(circle at 85% 85%, rgba(179, 136, 255, 0.04) 0%, transparent 40%);
  -webkit-font-smoothing: antialiased;
}

.app-container {
  max-width: 1380px;
  margin: 0 auto;
  padding: 24px 32px;
  display: flex;
  flex-direction: column;
  gap: 24px;
}

/* ================= Header ================= */
.app-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 24px;
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  backdrop-filter: blur(20px);
}

.brand {
  display: flex;
  align-items: center;
  gap: 16px;
}

.brand-icon {
  width: 48px;
  height: 48px;
  background: linear-gradient(135deg, var(--neon-cyan), var(--neon-emerald));
  border-radius: var(--radius-md);
  display: flex;
  align-items: center;
  justify-content: center;
  color: #000;
  box-shadow: 0 0 20px var(--neon-cyan-glow);
}

.brand-icon svg {
  width: 28px;
  height: 28px;
}

.brand-text h1 {
  font-size: 24px;
  font-weight: 800;
  letter-spacing: -0.5px;
  display: flex;
  align-items: center;
  gap: 10px;
}

.badge-tag {
  font-size: 11px;
  font-weight: 700;
  padding: 3px 8px;
  background: rgba(0, 229, 255, 0.12);
  color: var(--neon-cyan);
  border: 1px solid rgba(0, 229, 255, 0.3);
  border-radius: 20px;
  letter-spacing: 0.5px;
}

.subtitle {
  font-size: 13px;
  color: var(--text-secondary);
  margin-top: 2px;
}

.status-indicator {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 16px;
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: 30px;
  font-size: 13px;
  font-weight: 600;
}

.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: var(--neon-emerald);
  box-shadow: 0 0 10px var(--neon-emerald-glow);
  transition: all 0.3s ease;
}

.status-dot.running {
  background: var(--neon-cyan);
  box-shadow: 0 0 12px var(--neon-cyan);
  animation: pulse-dot 1.2s infinite alternate;
}

.status-dot.error {
  background: var(--neon-rose);
  box-shadow: 0 0 12px var(--neon-rose);
}

@keyframes pulse-dot {
  from { transform: scale(0.9); opacity: 0.7; }
  to { transform: scale(1.2); opacity: 1; }
}

/* ================= Dashboard Grid ================= */
.dashboard-grid {
  display: grid;
  grid-template-columns: 380px 1fr;
  gap: 24px;
}

.panel {
  background: var(--bg-surface);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 20px;
  backdrop-filter: blur(12px);
}

.panel-header {
  display: flex;
  align-items: center;
  gap: 12px;
  border-bottom: 1px solid var(--border-subtle);
  padding-bottom: 14px;
}

.panel-icon {
  width: 22px;
  height: 22px;
  color: var(--neon-cyan);
}

.panel-header h2 {
  font-size: 18px;
  font-weight: 700;
}

/* Controls */
.control-group {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.control-label {
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.8px;
  color: var(--text-secondary);
}

.material-select {
  width: 100%;
  padding: 12px 16px;
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  color: var(--text-primary);
  font-family: var(--font-sans);
  font-size: 14px;
  font-weight: 500;
  outline: none;
  cursor: pointer;
  transition: all 0.2s ease;
}

.material-select:focus {
  border-color: var(--neon-cyan);
  box-shadow: 0 0 0 2px var(--neon-cyan-glow);
}

.drive-details-card {
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  font-size: 13px;
}

.drive-stat-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.drive-stat-row .label {
  color: var(--text-secondary);
}

.drive-stat-row .val {
  font-weight: 600;
  font-family: var(--font-mono);
}

.badge-fs {
  background: rgba(0, 230, 118, 0.12);
  color: var(--neon-emerald);
  padding: 2px 8px;
  border-radius: 6px;
  font-size: 12px;
}

.storage-bar-track {
  width: 100%;
  height: 6px;
  background: rgba(255, 255, 255, 0.08);
  border-radius: 4px;
  overflow: hidden;
  margin-top: 4px;
}

.storage-bar-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--neon-cyan), var(--neon-emerald));
  transition: width 0.4s ease;
}

/* Routine Mode Grid */
.mode-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.mode-card {
  position: relative;
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 12px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.mode-card input {
  position: absolute;
  opacity: 0;
}

.mode-card.active, .mode-card:has(input:checked) {
  border-color: var(--neon-cyan);
  background: rgba(0, 229, 255, 0.08);
  box-shadow: 0 0 14px rgba(0, 229, 255, 0.15);
}

.mode-title {
  display: block;
  font-size: 14px;
  font-weight: 700;
}

.mode-desc {
  display: block;
  font-size: 11px;
  color: var(--text-secondary);
  margin-top: 2px;
}

/* Size Pills */
.pill-group {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 8px;
}

.pill-btn {
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-sm);
  color: var(--text-secondary);
  padding: 10px 4px;
  font-family: var(--font-sans);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
}

.pill-btn.active {
  background: var(--neon-cyan);
  color: #000;
  border-color: var(--neon-cyan);
  box-shadow: 0 0 12px var(--neon-cyan-glow);
}

/* Action Buttons */
.action-buttons {
  display: flex;
  gap: 12px;
  margin-top: 8px;
}

.btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 14px 20px;
  border-radius: var(--radius-md);
  font-family: var(--font-sans);
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
  border: none;
  transition: all 0.2s ease;
}

.btn svg {
  width: 18px;
  height: 18px;
}

.btn-primary {
  background: linear-gradient(135deg, var(--neon-cyan), #00b0ff);
  color: #000;
  box-shadow: 0 4px 18px var(--neon-cyan-glow);
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 24px rgba(0, 229, 255, 0.4);
}

.btn-secondary {
  background: var(--bg-surface-elevated);
  color: var(--neon-rose);
  border: 1px solid rgba(255, 23, 68, 0.3);
}

.btn-secondary:hover:not(:disabled) {
  background: rgba(255, 23, 68, 0.12);
}

.btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
  transform: none !important;
}

/* ================= Visuals Panel ================= */
.visuals-panel {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.visuals-top-row {
  display: grid;
  grid-template-columns: 260px 1fr;
  gap: 20px;
  align-items: center;
}

/* Gauge */
.gauge-container {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  padding: 20px;
}

.speedometer-svg {
  width: 200px;
  height: 140px;
}

.gauge-fill {
  transition: stroke-dashoffset 0.1s linear;
}

.gauge-readout {
  position: absolute;
  bottom: 24px;
  text-align: center;
}

.live-speed-value {
  font-size: 36px;
  font-weight: 800;
  font-family: var(--font-mono);
  color: var(--neon-cyan);
  line-height: 1;
  text-shadow: 0 0 16px var(--neon-cyan-glow);
}

.live-speed-unit {
  font-size: 12px;
  font-weight: 600;
  color: var(--text-secondary);
  margin-top: 4px;
}

.live-test-phase {
  font-size: 11px;
  font-weight: 700;
  color: var(--neon-emerald);
  letter-spacing: 1px;
  margin-top: 6px;
}

/* Metrics Cards Grid */
.metrics-cards-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.metric-card {
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-md);
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  position: relative;
  overflow: hidden;
}

.metric-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 4px;
  height: 100%;
}

.cyan-card::before { background: var(--neon-cyan); }
.emerald-card::before { background: var(--neon-emerald); }
.purple-card::before { background: var(--neon-purple); }
.amber-card::before { background: var(--neon-amber); }

.metric-title {
  font-size: 12px;
  font-weight: 600;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.metric-value {
  font-size: 22px;
  font-weight: 800;
  font-family: var(--font-mono);
  color: var(--text-primary);
}

.metric-value .unit {
  font-size: 13px;
  color: var(--text-secondary);
  font-weight: 500;
}

/* Real-Time Wave Chart */
.chart-container {
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.chart-title {
  font-size: 14px;
  font-weight: 700;
}

.chart-legend {
  display: flex;
  align-items: center;
  gap: 14px;
  font-size: 12px;
  color: var(--text-secondary);
}

.legend-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
  margin-right: 4px;
}

.legend-dot.cyan { background: var(--neon-cyan); box-shadow: 0 0 6px var(--neon-cyan); }
.legend-dot.emerald { background: var(--neon-emerald); box-shadow: 0 0 6px var(--neon-emerald); }

.canvas-wrapper {
  width: 100%;
  height: 220px;
  position: relative;
}

canvas#speed-chart {
  width: 100%;
  height: 100%;
  display: block;
}

.progress-bar-container {
  width: 100%;
  height: 6px;
  background: rgba(255, 255, 255, 0.08);
  border-radius: 3px;
  overflow: hidden;
}

.progress-bar-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--neon-cyan), var(--neon-emerald));
  transition: width 0.2s ease;
}

/* Results Table */
.results-table-container {
  background: var(--bg-surface-elevated);
  border: 1px solid var(--border-subtle);
  border-radius: var(--radius-lg);
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.results-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.results-header h3 {
  font-size: 16px;
  font-weight: 700;
}

.btn-text {
  background: none;
  border: none;
  color: var(--neon-cyan);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  padding: 4px 8px;
  border-radius: 4px;
  transition: background 0.2s;
}

.btn-text:hover {
  background: rgba(0, 229, 255, 0.1);
}

.results-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.results-table th {
  text-align: left;
  padding: 10px 14px;
  color: var(--text-secondary);
  font-weight: 600;
  border-bottom: 1px solid var(--border-subtle);
}

.results-table td {
  padding: 12px 14px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.04);
  font-family: var(--font-mono);
}

.results-table tr:last-child td {
  border-bottom: none;
}

.empty-state {
  text-align: center;
  color: var(--text-tertiary);
  padding: 24px !important;
  font-family: var(--font-sans) !important;
}

/* Footer */
.app-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 20px;
  font-size: 12px;
  color: var(--text-tertiary);
  border-top: 1px solid var(--border-subtle);
}

/* Responsive adjustments */
@media (max-width: 1080px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
  }
}
)rwbench_css";
    inline const char APP_JS[] = R"rwbench_js(/* ==========================================================================
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
)rwbench_js";
}
