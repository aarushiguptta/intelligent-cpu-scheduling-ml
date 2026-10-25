/**
 * Intelligent CPU Scheduling Dashboard - Main Client Controller.
 */

// Global Application State
const State = {
  processes: [],
  simulationData: null,
  workloadDescriptions: {},
  modelsMetadata: {},
  currentAlgorithm: "ALL",
  theme: "dark"
};

// Textbook and Scenario Presets
const PRESETS = {
  textbook_sjf: [
    { pid: "P1", arrival_time: 0.0, burst_time: 7.0, category: 1, priority: 2, memory_mb: 512, past_burst_avg: 7.0 },
    { pid: "P2", arrival_time: 2.0, burst_time: 4.0, category: 0, priority: 1, memory_mb: 256, past_burst_avg: 4.0 },
    { pid: "P3", arrival_time: 4.0, burst_time: 1.0, category: 3, priority: 0, memory_mb: 128, past_burst_avg: 1.0 },
    { pid: "P4", arrival_time: 5.0, burst_time: 4.0, category: 1, priority: 2, memory_mb: 512, past_burst_avg: 4.0 }
  ],
  convoy_effect: [
    { pid: "P_Heavy", arrival_time: 0.0, burst_time: 30.0, category: 2, priority: 3, memory_mb: 2048, past_burst_avg: 28.0 },
    { pid: "P_Short1", arrival_time: 1.0, burst_time: 2.0, category: 0, priority: 1, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P_Short2", arrival_time: 2.0, burst_time: 2.0, category: 0, priority: 1, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P_Short3", arrival_time: 3.0, burst_time: 3.0, category: 0, priority: 1, memory_mb: 256, past_burst_avg: 3.0 }
  ],
  starvation_aging: [
    { pid: "P_Big", arrival_time: 0.0, burst_time: 12.0, category: 2, priority: 2, memory_mb: 1024, past_burst_avg: 12.0 },
    { pid: "P_S1", arrival_time: 0.0, burst_time: 2.0, category: 0, priority: 0, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P_S2", arrival_time: 2.0, burst_time: 2.0, category: 0, priority: 0, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P_S3", arrival_time: 4.0, burst_time: 2.0, category: 0, priority: 0, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P_S4", arrival_time: 6.0, burst_time: 2.0, category: 0, priority: 0, memory_mb: 128, past_burst_avg: 2.0 }
  ],
  simultaneous_arrival: [
    { pid: "P1", arrival_time: 0.0, burst_time: 8.0, category: 1, priority: 1, memory_mb: 512, past_burst_avg: 8.0 },
    { pid: "P2", arrival_time: 0.0, burst_time: 4.0, category: 0, priority: 2, memory_mb: 256, past_burst_avg: 4.0 },
    { pid: "P3", arrival_time: 0.0, burst_time: 2.0, category: 3, priority: 0, memory_mb: 128, past_burst_avg: 2.0 },
    { pid: "P4", arrival_time: 0.0, burst_time: 6.0, category: 2, priority: 3, memory_mb: 1024, past_burst_avg: 6.0 }
  ],
  misprediction_test: [
    { pid: "P_Underestimated", arrival_time: 0.0, burst_time: 15.0, category: 0, priority: 0, memory_mb: 128, past_burst_avg: 3.0 },
    { pid: "P_Regular1", arrival_time: 1.0, burst_time: 4.0, category: 1, priority: 1, memory_mb: 512, past_burst_avg: 4.0 },
    { pid: "P_Regular2", arrival_time: 2.0, burst_time: 3.0, category: 0, priority: 1, memory_mb: 256, past_burst_avg: 3.0 }
  ]
};

// Initial setup on DOM ready
document.addEventListener("DOMContentLoaded", () => {
  setupEventListeners();
  loadInitialData();
});

function setupEventListeners() {
  // Theme Toggle
  document.getElementById("themeToggleBtn").addEventListener("click", toggleTheme);

  // Workload Generator
  document.getElementById("generateWorkloadBtn").addEventListener("click", () => {
    const type = document.getElementById("workloadSelect").value;
    const count = parseInt(document.getElementById("workloadCount").value, 10) || 7;
    fetchWorkload(type, count);
  });

  document.getElementById("workloadSelect").addEventListener("change", (e) => {
    updateWorkloadInfoBox(e.target.value);
  });

  // Presets Dropdown
  const presetsBtn = document.getElementById("presetsBtn");
  const presetsMenu = document.getElementById("presetsMenu");
  presetsBtn.addEventListener("click", () => presetsMenu.classList.toggle("hidden"));
  document.addEventListener("click", (e) => {
    if (!presetsBtn.contains(e.target) && !presetsMenu.contains(e.target)) {
      presetsMenu.classList.add("hidden");
    }
  });

  document.querySelectorAll(".dropdown-item").forEach(item => {
    item.addEventListener("click", (e) => {
      const presetKey = e.target.getAttribute("data-preset");
      loadPreset(presetKey);
      presetsMenu.classList.add("hidden");
    });
  });

  // I/O Note Toggle
  document.getElementById("ioNoteToggle").addEventListener("click", () => {
    const content = document.getElementById("ioNoteContent");
    content.classList.toggle("hidden");
  });

  // Table Actions
  document.getElementById("addProcessBtn").addEventListener("click", openAddModal);
  document.getElementById("closeModalBtn").addEventListener("click", closeAddModal);
  document.getElementById("cancelModalBtn").addEventListener("click", closeAddModal);
  document.getElementById("addProcessForm").addEventListener("submit", handleAddProcess);
  document.getElementById("clearProcsBtn").addEventListener("click", clearProcesses);
  document.getElementById("exportJsonBtn").addEventListener("click", exportProcessesJson);

  // Run Simulation Button
  document.getElementById("runSimulationBtn").addEventListener("click", runSimulation);

  // Filter Details Table
  document.getElementById("detailsAlgoFilter").addEventListener("change", (e) => {
    renderDetailsTable(e.target.value);
  });
}

async function loadInitialData() {
  try {
    // 1. Fetch Workload Types
    const wRes = await fetch("/api/workload/types");
    if (wRes.ok) {
      const wData = await wRes.json();
      State.workloadDescriptions = wData.workload_types || {};
      document.getElementById("ioNoteContent").textContent = wData.io_modeling_note || "";
      updateWorkloadInfoBox("mixed");
    }

    // 2. Fetch Models Info
    const mRes = await fetch("/api/models");
    if (mRes.ok) {
      const mData = await mRes.json();
      State.modelsMetadata = mData.evaluation_metrics || {};
    }

    // 3. Default Preset: Silberschatz SJF
    loadPreset("textbook_sjf");

    // Automatically trigger initial simulation to display results on page load
    setTimeout(runSimulation, 400);

  } catch (err) {
    console.error("Initialization error:", err);
    loadPreset("textbook_sjf");
  }
}

function toggleTheme() {
  const body = document.body;
  const isLight = body.classList.toggle("light-theme");
  document.getElementById("themeIcon").textContent = isLight ? "🌙" : "☀️";
  State.theme = isLight ? "light" : "dark";
  if (State.simulationData) {
    renderResults(State.simulationData);
  }
}

function updateWorkloadInfoBox(wType) {
  const desc = State.workloadDescriptions[wType];
  const titleEl = document.getElementById("workloadInfoTitle");
  const textEl = document.getElementById("workloadInfoText");
  if (desc) {
    titleEl.textContent = desc.title;
    textEl.textContent = `${desc.description} | ${desc.io_characteristics}`;
  }
}

async function fetchWorkload(type, count) {
  const loader = document.getElementById("simulationLoader");
  loader.classList.remove("hidden");
  try {
    const res = await fetch(`/api/workload/generate?type=${type}&count=${count}&seed=${Math.floor(Math.random() * 1000)}`);
    if (!res.ok) throw new Error("Failed to generate workload");
    const data = await res.json();
    State.processes = data.processes || [];
    renderProcessTable();
    updateWorkloadInfoBox(type);
  } catch (err) {
    alert("Error generating workload: " + err.message);
  } finally {
    loader.classList.add("hidden");
  }
}

function loadPreset(presetKey) {
  const preset = PRESETS[presetKey];
  if (!preset) return;
  // Deep copy
  State.processes = JSON.parse(JSON.stringify(preset));
  renderProcessTable();
}

function renderProcessTable() {
  const tbody = document.getElementById("processTableBody");
  tbody.innerHTML = "";
  document.getElementById("processCountBadge").textContent = `${State.processes.length} processes`;

  State.processes.forEach((p, idx) => {
    const tr = document.createElement("tr");

    const categoryNames = ["Interactive", "Batch", "Computational", "System"];
    const catLabel = categoryNames[p.process_category ?? p.category ?? 1] || "Batch";

    tr.innerHTML = `
      <td class="font-mono"><strong>${p.pid}</strong></td>
      <td class="font-mono">${Number(p.arrival_time).toFixed(1)}</td>
      <td class="font-mono">${Number(p.burst_time).toFixed(1)}</td>
      <td><span class="badge badge-cpp">${catLabel}</span></td>
      <td>${p.priority_level ?? p.priority ?? 0}</td>
      <td>${p.memory_mb ?? 512} MB</td>
      <td class="font-mono">${Number(p.past_burst_avg ?? p.burst_time).toFixed(1)}</td>
      <td class="font-mono" style="color:var(--cyan);font-weight:700;">
        ${p.predicted_burst_time ? Number(p.predicted_burst_time).toFixed(1) : '<span style="opacity:0.5">-</span>'}
      </td>
      <td>
        <button class="btn btn-icon btn-danger-text" onclick="deleteProcess(${idx})" title="Delete Process">✕</button>
      </td>
    `;
    tbody.appendChild(tr);
  });
}

window.deleteProcess = function(idx) {
  State.processes.splice(idx, 1);
  renderProcessTable();
};

function clearProcesses() {
  State.processes = [];
  renderProcessTable();
}

function exportProcessesJson() {
  const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(State.processes, null, 2));
  const downloadAnchor = document.createElement("a");
  downloadAnchor.setAttribute("href", dataStr);
  downloadAnchor.setAttribute("download", "cpu_scheduling_processes.json");
  document.body.appendChild(downloadAnchor);
  downloadAnchor.click();
  downloadAnchor.remove();
}

function openAddModal() {
  document.getElementById("newPid").value = `P${State.processes.length + 1}`;
  document.getElementById("addProcessModal").classList.remove("hidden");
}

function closeAddModal() {
  document.getElementById("addProcessModal").classList.add("hidden");
}

function handleAddProcess(e) {
  e.preventDefault();
  const pid = document.getElementById("newPid").value.trim();
  const arrival = parseFloat(document.getElementById("newArrivalTime").value);
  const burst = parseFloat(document.getElementById("newBurstTime").value);
  const category = parseInt(document.getElementById("newCategory").value, 10);
  const priority = parseInt(document.getElementById("newPriority").value, 10);
  const memory = parseFloat(document.getElementById("newMemory").value);
  const pastBurst = parseFloat(document.getElementById("newPastBurst").value);

  State.processes.push({
    pid: pid,
    arrival_time: arrival,
    burst_time: burst,
    category: category,
    process_category: category,
    priority: priority,
    priority_level: priority,
    memory_mb: memory,
    past_burst_avg: pastBurst
  });

  closeAddModal();
  renderProcessTable();
}

// Simulation Dispatcher
async function runSimulation() {
  if (State.processes.length === 0) {
    alert("Please add at least one process to the pool.");
    return;
  }

  const loader = document.getElementById("simulationLoader");
  const resultsSec = document.getElementById("resultsSection");
  loader.classList.remove("hidden");

  const algorithm = document.getElementById("algorithmSelect").value;
  const quantum = parseFloat(document.getElementById("quantumInput").value) || 2.0;
  const csOverhead = parseFloat(document.getElementById("csOverheadInput").value) || 0.0;
  const agingFactor = parseFloat(document.getElementById("agingFactorInput").value) || 0.15;
  const fallbackQuantum = parseFloat(document.getElementById("fallbackQuantumInput").value) || 2.0;
  const modelName = document.getElementById("mlModelSelect").value;

  const payload = {
    processes: State.processes,
    algorithm: algorithm,
    time_quantum: quantum,
    context_switch_overhead: csOverhead,
    aging_factor: agingFactor,
    fallback_quantum: fallbackQuantum,
    model_name: modelName
  };

  try {
    const res = await fetch("/api/simulate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    if (!res.ok) {
      const errData = await res.json();
      throw new Error(errData.error || "Simulation failed");
    }

    const data = await res.json();
    State.simulationData = data;

    // Update enriched predictions in process table
    if (data.simulation && data.simulation.results && data.simulation.results.length > 0) {
      const firstAlgoProcs = data.simulation.results[0].processes || [];
      firstAlgoProcs.forEach(fp => {
        const matching = State.processes.find(p => p.pid === fp.pid);
        if (matching) matching.predicted_burst_time = fp.predicted_burst_time;
      });
      renderProcessTable();
    }

    renderResults(data);
    resultsSec.scrollIntoView({ behavior: "smooth" });

  } catch (err) {
    alert("Simulation Error: " + err.message);
  } finally {
    loader.classList.add("hidden");
  }
}

// Results Renderer
function renderResults(data) {
  const sim = data.simulation;
  const ml = data.ml_telemetry || {};
  let resultsList = [];

  if (sim.results) {
    resultsList = sim.results;
  } else if (sim.algorithm_name) {
    resultsList = [sim];
  }

  if (resultsList.length === 0) return;

  // 1. Metric Summary Cards (Focus on ML-SJF Hybrid or selected)
  renderMetricSummaryCards(resultsList);

  // 2. Side-by-Side Benchmark Table
  renderBenchmarkTable(resultsList);

  // 3. Performance Charts (Chart.js)
  renderPerformanceCharts(resultsList);

  // 4. Gantt Charts
  renderAllGanttCharts(resultsList);

  // 5. ML Telemetry Panel
  renderMlDiagnostics(ml, data.configuration.model_name);

  // 6. Populate Algorithm Filter for Process Details
  populateDetailsFilter(resultsList);
}

function renderMetricSummaryCards(results) {
  const container = document.getElementById("metricsCardsGrid");
  container.innerHTML = "";

  // Highlight ML-SJF Hybrid if available, else first algorithm
  const target = results.find(r => r.algorithm_name.includes("Hybrid")) || results[0];
  const m = target.metrics;

  const cards = [
    { label: "Avg Waiting Time", val: `${m.average_waiting_time.toFixed(2)} ms`, sub: target.algorithm_name },
    { label: "Avg Turnaround", val: `${m.average_turnaround_time.toFixed(2)} ms`, sub: `Makespan: ${m.total_simulation_time.toFixed(1)}ms` },
    { label: "Avg Response Time", val: `${m.average_response_time.toFixed(2)} ms`, sub: "Time to initial CPU slice" },
    { label: "Throughput", val: `${m.throughput.toFixed(3)}`, sub: "Processes / ms" },
    { label: "CPU Utilization", val: `${m.cpu_utilization.toFixed(1)}%`, sub: `Busy: ${m.cpu_busy_time.toFixed(1)}ms` },
    { label: "Jain's Fairness", val: `${m.jain_fairness_index.toFixed(3)}`, sub: "1.0 = Perfect Equity" },
    { label: "Context Switches", val: `${m.total_context_switches}`, sub: `Starved Jobs: ${m.starved_process_count}` }
  ];

  cards.forEach(c => {
    const cardEl = document.createElement("div");
    cardEl.className = "metric-card";
    cardEl.innerHTML = `
      <div class="metric-label">${c.label}</div>
      <div class="metric-value">${c.val}</div>
      <div class="metric-sub">${c.sub}</div>
    `;
    container.appendChild(cardEl);
  });
}

function renderBenchmarkTable(results) {
  const tbody = document.getElementById("benchmarkTableBody");
  tbody.innerHTML = "";

  // Find min AWT, ATT, ART for highlighting
  const minAwt = Math.min(...results.map(r => r.metrics.average_waiting_time));
  const minAtt = Math.min(...results.map(r => r.metrics.average_turnaround_time));
  const maxFairness = Math.max(...results.map(r => r.metrics.jain_fairness_index));

  results.forEach(res => {
    const m = res.metrics;
    const tr = document.createElement("tr");

    const isAwtBest = Math.abs(m.average_waiting_time - minAwt) < 1e-4;
    const isAttBest = Math.abs(m.average_turnaround_time - minAtt) < 1e-4;
    const isFairBest = Math.abs(m.jain_fairness_index - maxFairness) < 1e-4;

    tr.innerHTML = `
      <td><strong>${res.algorithm_name}</strong></td>
      <td class="font-mono ${isAwtBest ? 'val-best' : ''}">${m.average_waiting_time.toFixed(2)} ms</td>
      <td class="font-mono ${isAttBest ? 'val-best' : ''}">${m.average_turnaround_time.toFixed(2)} ms</td>
      <td class="font-mono">${m.average_response_time.toFixed(2)} ms</td>
      <td class="font-mono">${m.throughput.toFixed(3)}</td>
      <td class="font-mono">${m.cpu_utilization.toFixed(1)}%</td>
      <td class="font-mono ${isFairBest ? 'val-best' : ''}">${m.jain_fairness_index.toFixed(3)}</td>
      <td class="font-mono">${m.max_waiting_time.toFixed(2)} ms</td>
      <td class="font-mono">${m.total_context_switches}</td>
      <td class="font-mono ${m.starved_process_count > 0 ? 'val-worst' : ''}">${m.starved_process_count}</td>
    `;
    tbody.appendChild(tr);
  });
}

function renderMlDiagnostics(ml, modelName) {
  document.getElementById("mlModelNameVal").textContent = (modelName || "Decision Tree").toUpperCase();
  document.getElementById("mlMaeVal").textContent = ml.mae !== undefined ? `${ml.mae.toFixed(2)} ms` : "N/A";
  document.getElementById("mlRmseVal").textContent = ml.rmse !== undefined ? `${ml.rmse.toFixed(2)} ms` : "N/A";
  document.getElementById("mlMapeVal").textContent = ml.mape_percent !== undefined ? `${ml.mape_percent.toFixed(1)}%` : "N/A";
  
  const overheadUs = ml.avg_inference_time_us || 0;
  document.getElementById("mlOverheadVal").textContent = `${overheadUs.toFixed(1)} µs (${(overheadUs/1000).toFixed(4)} ms)`;
  document.getElementById("mlTotalTimeVal").textContent = `${(ml.total_inference_time_ms || 0).toFixed(2)} ms`;

  // Render Feature Importance Chart
  const modelMeta = State.modelsMetadata[modelName] || State.modelsMetadata["decision_tree"] || {};
  if (modelMeta.feature_importances) {
    renderFeatureImportanceChart(modelMeta.feature_importances);
  }

  // Academic assessment dynamic insight
  const calloutEl = document.getElementById("academicAssessmentText");
  if (ml.mape_percent !== undefined) {
    if (ml.mape_percent < 15.0) {
      calloutEl.innerHTML = `<strong>High Prediction Precision:</strong> The ML model achieved an error rate of <strong>${ml.mape_percent.toFixed(1)}%</strong>. This enables the hybrid scheduler to approximate Oracle SJF waiting times closely while maintaining fairness via the aging factor (α).`;
    } else {
      calloutEl.innerHTML = `<strong>Moderate Estimation Variance (${ml.mape_percent.toFixed(1)}% MAPE):</strong> Due to prediction variance, processes exceeding their estimated burst are safely demoted to the Round Robin fallback queue, preventing convoy starvation. Measured results illustrate how aging and demotion absorb real-world ML errors.`;
    }
  }
}

function populateDetailsFilter(results) {
  const filter = document.getElementById("detailsAlgoFilter");
  filter.innerHTML = "";
  results.forEach((r, idx) => {
    const opt = document.createElement("option");
    opt.value = idx;
    opt.textContent = r.algorithm_name;
    if (r.algorithm_name.includes("Hybrid")) opt.selected = true;
    filter.appendChild(opt);
  });
  renderDetailsTable(filter.value);
}

function renderDetailsTable(algoIdx) {
  const tbody = document.getElementById("detailsTableBody");
  tbody.innerHTML = "";
  if (!State.simulationData) return;

  const sim = State.simulationData.simulation;
  const results = sim.results || [sim];
  const target = results[algoIdx] || results[0];
  if (!target || !target.processes) return;

  target.processes.forEach(p => {
    const tr = document.createElement("tr");
    tr.innerHTML = `
      <td class="font-mono"><strong>${p.pid}</strong></td>
      <td class="font-mono">${p.arrival_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.burst_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.predicted_burst_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.start_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.completion_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.waiting_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.turnaround_time.toFixed(1)} ms</td>
      <td class="font-mono">${p.response_time.toFixed(1)} ms</td>
      <td>
        ${p.demoted_to_rr ? '<span class="badge badge-status" style="background:rgba(239,68,68,0.15);color:#f87171;border:1px solid rgba(239,68,68,0.3)">Demoted to RR</span>' : '<span style="color:var(--text-dim)">No</span>'}
      </td>
    `;
    tbody.appendChild(tr);
  });
}
