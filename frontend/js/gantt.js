/**
 * Interactive Gantt Chart Renderer for CPU Scheduling Timelines.
 */

function renderAllGanttCharts(results) {
  const container = document.getElementById("ganttChartsContainer");
  if (!container) return;
  container.innerHTML = "";

  results.forEach((res, idx) => {
    const block = document.createElement("div");
    block.className = "gantt-algo-block";

    const header = document.createElement("div");
    header.className = "gantt-algo-header";
    header.innerHTML = `
      <div class="gantt-algo-title">${res.algorithm_name}</div>
      <div class="card-tag">Makespan: ${res.metrics.total_simulation_time.toFixed(1)} ms | CS: ${res.metrics.total_context_switches}</div>
    `;
    block.appendChild(header);

    const timelineWrapper = document.createElement("div");
    timelineWrapper.className = "gantt-timeline-wrapper";

    const track = document.createElement("div");
    track.className = "gantt-track";

    const totalTime = Math.max(1.0, res.metrics.total_simulation_time);

    res.gantt_chart.forEach(seg => {
      const segDuration = seg.end_time - seg.start_time;
      if (segDuration <= 0) return;

      const leftPercent = (seg.start_time / totalTime) * 100.0;
      const widthPercent = (segDuration / totalTime) * 100.0;

      const segDiv = document.createElement("div");
      segDiv.className = "gantt-segment";

      // Classify segment type
      if (seg.is_idle) {
        segDiv.classList.add("type-idle");
        segDiv.textContent = "IDLE";
        segDiv.title = `IDLE: [${seg.start_time.toFixed(1)}ms - ${seg.end_time.toFixed(1)}ms] (${segDuration.toFixed(1)}ms)`;
      } else if (seg.is_context_switch) {
        segDiv.classList.add("type-cs");
        segDiv.textContent = "CS";
        segDiv.title = `Context Switch: [${seg.start_time.toFixed(1)}ms - ${seg.end_time.toFixed(1)}ms] (${segDuration.toFixed(1)}ms)`;
      } else if (seg.queue_type === "RR_FALLBACK" || seg.queue_type === "NORMAL" && res.algorithm_name.includes("Round Robin")) {
        segDiv.classList.add("type-rr");
        segDiv.innerHTML = `<span>${seg.pid}</span><span style="font-size:0.6rem;opacity:0.8">${segDuration.toFixed(1)}</span>`;
        segDiv.title = `${seg.pid} [RR]: ${seg.start_time.toFixed(1)}ms - ${seg.end_time.toFixed(1)}ms (${segDuration.toFixed(1)}ms)`;
      } else {
        segDiv.classList.add("type-sjf");
        segDiv.innerHTML = `<span>${seg.pid}</span><span style="font-size:0.6rem;opacity:0.8">${segDuration.toFixed(1)}</span>`;
        segDiv.title = `${seg.pid} [SJF]: ${seg.start_time.toFixed(1)}ms - ${seg.end_time.toFixed(1)}ms (${segDuration.toFixed(1)}ms)`;
      }

      segDiv.style.left = `${leftPercent}%`;
      segDiv.style.width = `${Math.max(1.2, widthPercent)}%`;

      track.appendChild(segDiv);
    });

    timelineWrapper.appendChild(track);

    // Time Axis Ticks
    const axis = document.createElement("div");
    axis.className = "gantt-time-axis";

    const tickCount = Math.min(10, Math.ceil(totalTime));
    const step = totalTime / tickCount;

    for (let t = 0; t <= totalTime; t += step) {
      const tick = document.createElement("span");
      tick.className = "gantt-time-tick";
      tick.style.left = `${(t / totalTime) * 100.0}%`;
      tick.textContent = t.toFixed(0);
      axis.appendChild(tick);
    }
    // Final tick at end
    const lastTick = document.createElement("span");
    lastTick.className = "gantt-time-tick";
    lastTick.style.left = `100%`;
    lastTick.textContent = totalTime.toFixed(1);
    axis.appendChild(lastTick);

    timelineWrapper.appendChild(axis);
    block.appendChild(timelineWrapper);
    container.appendChild(block);
  });
}
