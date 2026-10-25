#include "fcfs.hpp"
#include <algorithm>

SimulationResult FCFSScheduler::schedule(
    std::vector<Process> processes,
    double /*time_quantum*/,
    double context_switch_overhead,
    double /*aging_factor*/,
    double /*fallback_quantum*/
) {
    SimulationResult result;
    result.algorithm_name = getName();
    if (processes.empty()) return result;

    for (auto& p : processes) p.reset();

    // Sort by arrival time; tie-break by PID
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        if (std::abs(a.arrival_time - b.arrival_time) > 1e-9)
            return a.arrival_time < b.arrival_time;
        return a.pid < b.pid;
    });

    double current_time = 0.0;
    int total_context_switches = 0;
    std::string last_pid = "";

    for (auto& p : processes) {
        // CPU idle period
        if (current_time < p.arrival_time) {
            GanttSegment idle_seg;
            idle_seg.pid = "IDLE";
            idle_seg.start_time = current_time;
            idle_seg.end_time = p.arrival_time;
            idle_seg.is_idle = true;
            idle_seg.queue_type = "IDLE";
            result.gantt_chart.push_back(idle_seg);
            current_time = p.arrival_time;
            last_pid = "";
        }

        // Context switch overhead (if switching from another process)
        if (!last_pid.empty() && last_pid != p.pid && context_switch_overhead > 0.0) {
            GanttSegment cs_seg;
            cs_seg.pid = "CS";
            cs_seg.start_time = current_time;
            cs_seg.end_time = current_time + context_switch_overhead;
            cs_seg.is_context_switch = true;
            cs_seg.queue_type = "CS";
            result.gantt_chart.push_back(cs_seg);
            current_time += context_switch_overhead;
            total_context_switches++;
        }

        // Execute process to completion
        p.start_time = current_time;
        p.has_started = true;
        p.response_time = p.start_time - p.arrival_time;

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
        last_pid = p.pid;
    }

    result.processes = processes;
    result.metrics = computeMetrics(result.processes, result.gantt_chart, current_time, total_context_switches);
    return result;
}
