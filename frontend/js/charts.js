/**
 * Chart.js Visualizations for CPU Scheduling Performance & Machine Learning Diagnostics.
 */

let latencyChartInstance = null;
let fairnessChartInstance = null;
let featureImportanceChartInstance = null;

function destroyCharts() {
  if (latencyChartInstance) { latencyChartInstance.destroy(); latencyChartInstance = null; }
  if (fairnessChartInstance) { fairnessChartInstance.destroy(); fairnessChartInstance = null; }
  if (featureImportanceChartInstance) { featureImportanceChartInstance.destroy(); featureImportanceChartInstance = null; }
}

function renderPerformanceCharts(results) {
  const isLight = document.body.classList.contains("light-theme");
  const textColor = isLight ? "#0f172a" : "#f1f5f9";
  const gridColor = isLight ? "rgba(0,0,0,0.06)" : "rgba(255,255,255,0.06)";

  const labels = results.map(r => {
    // Shorten label for chart axis readability
    if (r.algorithm_name.includes("Oracle")) return "Oracle SJF";
    if (r.algorithm_name.includes("Estimated")) return "Estimated SJF";
    if (r.algorithm_name.includes("Hybrid")) return "ML-SJF Hybrid";
    if (r.algorithm_name.includes("Adaptive")) return "Adaptive RR";
    return r.algorithm_name;
  });

  const awtData = results.map(r => r.metrics.average_waiting_time);
  const attData = results.map(r => r.metrics.average_turnaround_time);
  const artData = results.map(r => r.metrics.average_response_time);

  const ctxLatency = document.getElementById("latencyChart");
  if (ctxLatency) {
    if (latencyChartInstance) latencyChartInstance.destroy();
    latencyChartInstance = new Chart(ctxLatency, {
      type: "bar",
      data: {
        labels: labels,
        datasets: [
          {
            label: "Avg Waiting Time (ms)",
            data: awtData,
            backgroundColor: "rgba(59, 130, 246, 0.75)",
            borderColor: "#3b82f6",
            borderWidth: 1,
            borderRadius: 4
          },
          {
            label: "Avg Turnaround Time (ms)",
            data: attData,
            backgroundColor: "rgba(139, 92, 246, 0.75)",
            borderColor: "#8b5cf6",
            borderWidth: 1,
            borderRadius: 4
          },
          {
            label: "Avg Response Time (ms)",
            data: artData,
            backgroundColor: "rgba(16, 185, 129, 0.75)",
            borderColor: "#10b981",
            borderWidth: 1,
            borderRadius: 4
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { labels: { color: textColor, font: { family: 'Plus Jakarta Sans', weight: '600' } } },
          tooltip: { padding: 10 }
        },
        scales: {
          x: { ticks: { color: textColor }, grid: { color: gridColor } },
          y: { 
            ticks: { color: textColor }, 
            grid: { color: gridColor },
            title: { display: true, text: "Milliseconds", color: textColor }
          }
        }
      }
    });
  }

  const throughputData = results.map(r => r.metrics.throughput);
  const fairnessData = results.map(r => r.metrics.jain_fairness_index);
  const utilData = results.map(r => r.metrics.cpu_utilization);

  const ctxFairness = document.getElementById("fairnessChart");
  if (ctxFairness) {
    if (fairnessChartInstance) fairnessChartInstance.destroy();
    fairnessChartInstance = new Chart(ctxFairness, {
      type: "bar",
      data: {
        labels: labels,
        datasets: [
          {
            label: "Jain's Fairness Index (0-1)",
            data: fairnessData,
            backgroundColor: "rgba(6, 182, 212, 0.75)",
            borderColor: "#06b6d4",
            borderWidth: 1,
            borderRadius: 4,
            yAxisID: "yFairness"
          },
          {
            label: "Throughput (proc/ms)",
            data: throughputData,
            type: "line",
            borderColor: "#f59e0b",
            backgroundColor: "#f59e0b",
            tension: 0.3,
            borderWidth: 3,
            pointRadius: 5,
            yAxisID: "yThroughput"
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { labels: { color: textColor, font: { family: 'Plus Jakarta Sans', weight: '600' } } }
        },
        scales: {
          x: { ticks: { color: textColor }, grid: { color: gridColor } },
          yFairness: {
            type: 'linear',
            position: 'left',
            min: 0,
            max: 1.05,
            ticks: { color: textColor },
            grid: { color: gridColor },
            title: { display: true, text: "Fairness Index", color: textColor }
          },
          yThroughput: {
            type: 'linear',
            position: 'right',
            ticks: { color: textColor },
            grid: { drawOnChartArea: false },
            title: { display: true, text: "Throughput", color: textColor }
          }
        }
      }
    });
  }
}

function renderFeatureImportanceChart(importances) {
  const isLight = document.body.classList.contains("light-theme");
  const textColor = isLight ? "#0f172a" : "#f1f5f9";
  const gridColor = isLight ? "rgba(0,0,0,0.06)" : "rgba(255,255,255,0.06)";

  const ctx = document.getElementById("featureImportanceChart");
  if (!ctx || !importances) return;

  const entries = Object.entries(importances).sort((a, b) => b[1] - a[1]);
  const labels = entries.map(e => e[0].replace(/_/g, " ").replace(/\b\w/g, l => l.toUpperCase()));
  const data = entries.map(e => e[1]);

  if (featureImportanceChartInstance) featureImportanceChartInstance.destroy();
  featureImportanceChartInstance = new Chart(ctx, {
    type: "bar",
    data: {
      labels: labels,
      datasets: [{
        label: "Feature Importance Ratio",
        data: data,
        backgroundColor: "rgba(139, 92, 246, 0.75)",
        borderColor: "#8b5cf6",
        borderWidth: 1,
        borderRadius: 4
      }]
    },
    options: {
      indexAxis: 'y',
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: { display: false }
      },
      scales: {
        x: { ticks: { color: textColor }, grid: { color: gridColor } },
        y: { ticks: { color: textColor }, grid: { color: gridColor } }
      }
    }
  });
}
