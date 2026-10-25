#include "round_robin.hpp"
#include <deque>
#include <algorithm>
#include <limits>

SimulationResult RoundRobinScheduler::schedule(
    std::vector<Process> processes,
    double time_quantum,
    double context_switch_overhead,
    double /*aging_factor*/,
    double /*fallback_quantum*/
) {
    SimulationResult result;
    result.algorithm_name = getName();
    if (processes.empty()) return result;

    if (time_quantum <= 0.0) time_quantum = 2.0;

    for (auto& p : processes) p.reset();

    int n = static_cast<int>(processes.size());
    // Track whether a process has been admitted to the ready queue
    std::vector<bool> admitted(n, false);
    std::deque<int> ready_queue;

    double current_time = 0.0;
    int completed_count = 0;
    int total_cs = 0;
    std::string last_pid = "";

    // Helper: admit all processes that have arrived by `current_time`
    auto admit_arrived = [&](double t) {
        // Collect unadmitted arrived processes and sort by arrival time, then PID
        std::vector<int> newly_arrived;
        for (int i = 0; i < n; ++i) {
            if (!admitted[i] && processes[i].arrival_time <= t + 1e-9) {
                newly_arrived.push_back(i);
            }
        }
        std::sort(newly_arrived.begin(), newly_arrived.end(), [&](int a, int b) {
            if (std::abs(processes[a].arrival_time - processes[b].arrival_time) > 1e-9)
                return processes[a].arrival_time < processes[b].arrival_time;
            return processes[a].pid < processes[b].pid;
        });
        for (int idx : newly_arrived) {
            ready_queue.push_back(idx);
            admitted[idx] = true;
        }
    };

    admit_arrived(current_time);

    while (completed_count < n) {
        if (ready_queue.empty()) {
            // Find earliest arrival among non-admitted
            double next_arrival = std::numeric_limits<double>::infinity();
            for (int i = 0; i < n; ++i) {
                if (!admitted[i] && processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }
            if (next_arrival < std::numeric_limits<double>::infinity()) {
                GanttSegment idle_seg;
                idle_seg.pid = "IDLE";
                idle_seg.start_time = current_time;
                idle_seg.end_time = next_arrival;
                idle_seg.is_idle = true;
                idle_seg.queue_type = "IDLE";
                result.gantt_chart.push_back(idle_seg);
                current_time = next_arrival;
                last_pid = "";
                admit_arrived(current_time);
            } else {
                break;
            }
        }

        if (ready_queue.empty()) break;

        int curr_idx = ready_queue.front();
        ready_queue.pop_front();
        auto& p = processes[curr_idx];

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
            total_cs++;
            p.context_switch_count++;
            // Check if any new process arrived during context switch
            admit_arrived(current_time);
        }

        if (!p.has_started) {
            p.has_started = true;
            p.start_time = current_time;
            p.response_time = p.start_time - p.arrival_time;
        }

        double exec_slice = std::min(time_quantum, p.remaining_time);

        GanttSegment exec_seg;
        exec_seg.pid = p.pid;
        exec_seg.start_time = current_time;
        exec_seg.end_time = current_time + exec_slice;
        exec_seg.is_idle = false;
        exec_seg.is_context_switch = false;
        exec_seg.queue_type = "NORMAL";
        result.gantt_chart.push_back(exec_seg);

        current_time += exec_slice;
        p.remaining_time -= exec_slice;
        p.executed_time += exec_slice;
        last_pid = p.pid;

        // Admit any processes that arrived during this execution slice
        admit_arrived(current_time);

        if (p.remaining_time <= 1e-9) {
            p.remaining_time = 0.0;
            p.completed = true;
            p.completion_time = current_time;
            completed_count++;
        } else {
            // Re-queue at the back of ready queue
            ready_queue.push_back(curr_idx);
        }
    }

    result.processes = processes;
    result.metrics = computeMetrics(result.processes, result.gantt_chart, current_time, total_cs);
    return result;
}
