#include "sjf.hpp"
#include <algorithm>
#include <limits>

namespace {
SimulationResult runSJFInternal(
    BaseScheduler* scheduler,
    std::vector<Process>& processes,
    bool use_oracle,
    const std::string& name,
    double context_switch_overhead
) {
    SimulationResult result;
    result.algorithm_name = name;
    if (processes.empty()) return result;

    for (auto& p : processes) p.reset();

    double current_time = 0.0;
    int completed_count = 0;
    int n = static_cast<int>(processes.size());
    std::string last_pid = "";

    while (completed_count < n) {
        // Find arrived processes not completed
        int best_idx = -1;
        double best_metric = std::numeric_limits<double>::infinity();
        double earliest_future_arrival = std::numeric_limits<double>::infinity();

        for (int i = 0; i < n; ++i) {
            if (!processes[i].completed) {
                if (processes[i].arrival_time <= current_time) {
                    double metric = use_oracle ? processes[i].burst_time : processes[i].estimated_burst_time;
                    if (metric < best_metric) {
                        best_metric = metric;
                        best_idx = i;
                    } else if (std::abs(metric - best_metric) < 1e-9) {
                        // Tie-break: arrival time, then PID
                        if (processes[i].arrival_time < processes[best_idx].arrival_time ||
                            (std::abs(processes[i].arrival_time - processes[best_idx].arrival_time) < 1e-9 && processes[i].pid < processes[best_idx].pid)) {
                            best_metric = metric;
                            best_idx = i;
                        }
                    }
                } else {
                    if (processes[i].arrival_time < earliest_future_arrival) {
                        earliest_future_arrival = processes[i].arrival_time;
                    }
                }
            }
        }

        if (best_idx == -1) {
            // CPU is idle until earliest_future_arrival
            if (earliest_future_arrival < std::numeric_limits<double>::infinity()) {
                GanttSegment idle_seg;
                idle_seg.pid = "IDLE";
                idle_seg.start_time = current_time;
                idle_seg.end_time = earliest_future_arrival;
                idle_seg.is_idle = true;
                idle_seg.queue_type = "IDLE";
                result.gantt_chart.push_back(idle_seg);
                current_time = earliest_future_arrival;
                last_pid = "";
            }
            continue;
        }

        auto& p = processes[best_idx];

        // Context switch
        if (!last_pid.empty() && last_pid != p.pid && context_switch_overhead > 0.0) {
            GanttSegment cs_seg;
            cs_seg.pid = "CS";
            cs_seg.start_time = current_time;
            cs_seg.end_time = current_time + context_switch_overhead;
            cs_seg.is_context_switch = true;
            cs_seg.queue_type = "CS";
            result.gantt_chart.push_back(cs_seg);
            current_time += context_switch_overhead;
        }

        p.start_time = current_time;
        p.has_started = true;
        p.response_time = p.start_time - p.arrival_time;

        // Non-preemptive execution for full actual burst time
        GanttSegment exec_seg;
        exec_seg.pid = p.pid;
        exec_seg.start_time = current_time;
        exec_seg.end_time = current_time + p.burst_time;
        exec_seg.is_idle = false;
        exec_seg.is_context_switch = false;
        exec_seg.queue_type = "NORMAL";
        result.gantt_chart.push_back(exec_seg);

        current_time += p.burst_time;
        p.remaining_time = 0.0;
        p.executed_time = p.burst_time;
        p.completion_time = current_time;
        p.completed = true;
        completed_count++;
        last_pid = p.pid;
    }

    result.processes = processes;
    // Call scheduler's computeMetrics helper via subclass
    (void)scheduler;
    return result;
}
} // namespace

SimulationResult OracleSJFScheduler::schedule(
    std::vector<Process> processes,
    double /*time_quantum*/,
    double context_switch_overhead,
    double /*aging_factor*/,
    double /*fallback_quantum*/
) {
    auto res = runSJFInternal(this, processes, true, getName(), context_switch_overhead);
    int total_cs = 0;
    for (const auto& g : res.gantt_chart) {
        if (g.is_context_switch) total_cs++;
    }
    double total_time = res.gantt_chart.empty() ? 0.0 : res.gantt_chart.back().end_time;
    res.metrics = computeMetrics(res.processes, res.gantt_chart, total_time, total_cs);
    return res;
}

SimulationResult EstimatedSJFScheduler::schedule(
    std::vector<Process> processes,
    double /*time_quantum*/,
    double context_switch_overhead,
    double /*aging_factor*/,
    double /*fallback_quantum*/
) {
    auto res = runSJFInternal(this, processes, false, getName(), context_switch_overhead);
    int total_cs = 0;
    for (const auto& g : res.gantt_chart) {
        if (g.is_context_switch) total_cs++;
    }
    double total_time = res.gantt_chart.empty() ? 0.0 : res.gantt_chart.back().end_time;
    res.metrics = computeMetrics(res.processes, res.gantt_chart, total_time, total_cs);
    return res;
}
