#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "process.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

class BaseScheduler {
public:
    virtual ~BaseScheduler() = default;
    virtual std::string getName() const = 0;
    virtual SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) = 0;

protected:
    SchedulerMetrics computeMetrics(
        std::vector<Process>& processes,
        const std::vector<GanttSegment>& gantt,
        double current_time,
        int total_cs
    ) {
        SchedulerMetrics m;
        if (processes.empty()) return m;

        m.total_simulation_time = current_time;
        m.total_context_switches = total_cs;

        double sum_wt = 0.0;
        double sum_tat = 0.0;
        double sum_rt = 0.0;
        double max_wt = 0.0;
        int starved_count = 0;

        // Vector for Jain's fairness index on normalized turnaround (BT / TAT)
        std::vector<double> norm_eff;
        norm_eff.reserve(processes.size());

        for (auto& p : processes) {
            p.turnaround_time = p.completion_time - p.arrival_time;
            p.waiting_time = p.turnaround_time - p.burst_time;
            if (p.waiting_time < 0.0) p.waiting_time = 0.0; // avoid floating point epsilon
            if (p.response_time < 0.0) p.response_time = p.start_time - p.arrival_time;

            sum_wt += p.waiting_time;
            sum_tat += p.turnaround_time;
            sum_rt += p.response_time;

            if (p.waiting_time > max_wt) {
                max_wt = p.waiting_time;
            }

            // Starvation heuristic: waiting time significantly exceeding burst time (e.g. > 4x and > 10 time units)
            if (p.burst_time > 0 && p.waiting_time > 4.0 * p.burst_time && p.waiting_time > 10.0) {
                starved_count++;
            }

            double eff = (p.turnaround_time > 0.0) ? (p.burst_time / p.turnaround_time) : 1.0;
            norm_eff.push_back(eff);
        }

        m.average_waiting_time = sum_wt / processes.size();
        m.average_turnaround_time = sum_tat / processes.size();
        m.average_response_time = sum_rt / processes.size();
        m.max_waiting_time = max_wt;
        m.starved_process_count = starved_count;

        m.throughput = (current_time > 0.0) ? (processes.size() / current_time) : 0.0;

        // Calculate CPU busy time from Gantt chart
        double busy_time = 0.0;
        for (const auto& seg : gantt) {
            if (!seg.is_idle && !seg.is_context_switch) {
                busy_time += (seg.end_time - seg.start_time);
            }
        }
        m.cpu_busy_time = busy_time;
        m.cpu_utilization = (current_time > 0.0) ? (busy_time / current_time * 100.0) : 0.0;

        // Jain's Fairness Index: (sum(x_i))^2 / (n * sum(x_i^2))
        double sum_x = 0.0;
        double sum_sq_x = 0.0;
        for (double val : norm_eff) {
            sum_x += val;
            sum_sq_x += (val * val);
        }
        if (processes.size() > 0 && sum_sq_x > 0.0) {
            m.jain_fairness_index = (sum_x * sum_x) / (processes.size() * sum_sq_x);
        } else {
            m.jain_fairness_index = 1.0;
        }

        return m;
    }
};

#endif // SCHEDULER_HPP
