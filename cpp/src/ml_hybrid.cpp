#include "ml_hybrid.hpp"
#include <deque>
#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>

SimulationResult MLSJFHybridScheduler::schedule(
    std::vector<Process> processes,
    double /*time_quantum*/,
    double context_switch_overhead,
    double aging_factor,
    double fallback_quantum
) {
    SimulationResult result;
    result.algorithm_name = getName();
    if (processes.empty()) return result;

    if (fallback_quantum <= 0.0) fallback_quantum = 2.0;
    if (aging_factor < 0.0) aging_factor = 0.0;

    for (auto& p : processes) p.reset();

    int n = static_cast<int>(processes.size());
    std::vector<bool> admitted(n, false);

    // Primary speculative queue (indices of active processes that have not exceeded predicted burst)
    std::vector<int> sjf_queue;
    // Secondary fallback Round Robin queue (indices of demoted processes)
    std::deque<int> fallback_rr_queue;

    double current_time = 0.0;
    int completed_count = 0;
    int total_cs = 0;
    std::string last_pid = "";

    auto admit_arrived = [&](double t) {
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
            sjf_queue.push_back(idx);
            admitted[idx] = true;
        }
    };

    admit_arrived(current_time);

    while (completed_count < n) {
        if (sjf_queue.empty() && fallback_rr_queue.empty()) {
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

        if (sjf_queue.empty() && fallback_rr_queue.empty()) break;

        // Decision logic:
        // Prefer SJF queue if it has eligible processes.
        // However, if fallback RR queue has processes waiting too long, prevent starve-out of demoted processes.
        bool pick_from_sjf = !sjf_queue.empty();

        int chosen_idx = -1;
        std::string chosen_queue_type = "SJF_QUEUE";
        double slice = 0.0;

        if (pick_from_sjf) {
            // Find process with lowest effective score = predicted_burst - aging_factor * wait_time
            int best_pos = -1;
            double best_score = std::numeric_limits<double>::infinity();

            for (size_t i = 0; i < sjf_queue.size(); ++i) {
                int p_idx = sjf_queue[i];
                double wait_time = current_time - processes[p_idx].arrival_time - processes[p_idx].executed_time;
                if (wait_time < 0.0) wait_time = 0.0;
                double effective_score = processes[p_idx].predicted_burst_time - (aging_factor * wait_time);

                if (effective_score < best_score) {
                    best_score = effective_score;
                    best_pos = static_cast<int>(i);
                } else if (std::abs(effective_score - best_score) < 1e-9) {
                    // Tie-breaker: earlier arrival, then PID
                    if (processes[p_idx].arrival_time < processes[sjf_queue[best_pos]].arrival_time ||
                        (std::abs(processes[p_idx].arrival_time - processes[sjf_queue[best_pos]].arrival_time) < 1e-9 &&
                         processes[p_idx].pid < processes[sjf_queue[best_pos]].pid)) {
                        best_score = effective_score;
                        best_pos = static_cast<int>(i);
                    }
                }
            }

            chosen_idx = sjf_queue[best_pos];
            sjf_queue.erase(sjf_queue.begin() + best_pos);
            chosen_queue_type = "SJF_QUEUE";

            // Speculative budget: process is allowed to run up to predicted_burst_time
            double remaining_predicted = processes[chosen_idx].predicted_burst_time - processes[chosen_idx].executed_time;
            if (remaining_predicted <= 1e-9) {
                // Already reached or exceeded prediction; demote immediately
                processes[chosen_idx].demoted_to_rr = true;
                fallback_rr_queue.push_back(chosen_idx);
                continue;
            }

            // Slice is minimum of remaining actual burst and remaining predicted budget
            slice = std::min(processes[chosen_idx].remaining_time, remaining_predicted);
        } else {
            // Schedule from Fallback RR Queue
            chosen_idx = fallback_rr_queue.front();
            fallback_rr_queue.pop_front();
            chosen_queue_type = "RR_FALLBACK";
            slice = std::min(fallback_quantum, processes[chosen_idx].remaining_time);
        }

        auto& p = processes[chosen_idx];

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
            admit_arrived(current_time);
        }

        if (!p.has_started) {
            p.has_started = true;
            p.start_time = current_time;
            p.response_time = p.start_time - p.arrival_time;
        }

        GanttSegment exec_seg;
        exec_seg.pid = p.pid;
        exec_seg.start_time = current_time;
        exec_seg.end_time = current_time + slice;
        exec_seg.is_idle = false;
        exec_seg.is_context_switch = false;
        exec_seg.queue_type = chosen_queue_type;
        result.gantt_chart.push_back(exec_seg);

        current_time += slice;
        p.remaining_time -= slice;
        p.executed_time += slice;
        last_pid = p.pid;

        // Admit newly arrived processes
        admit_arrived(current_time);

        if (p.remaining_time <= 1e-9) {
            p.remaining_time = 0.0;
            p.completed = true;
            p.completion_time = current_time;
            completed_count++;
        } else {
            // Check if process has exceeded predicted burst time
            if (p.executed_time >= p.predicted_burst_time - 1e-9) {
                // Demote to fallback round-robin
                p.demoted_to_rr = true;
                fallback_rr_queue.push_back(chosen_idx);
            } else {
                // Still has predicted budget remaining, return to SJF queue
                sjf_queue.push_back(chosen_idx);
            }
        }
    }

    result.processes = processes;
    result.metrics = computeMetrics(result.processes, result.gantt_chart, current_time, total_cs);
    return result;
}
