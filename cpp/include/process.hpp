#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <string>
#include <vector>

struct Process {
    std::string pid;
    double arrival_time = 0.0;
    double burst_time = 0.0;             // Actual burst time
    double predicted_burst_time = 0.0;   // Predicted by ML
    double estimated_burst_time = 0.0;   // Estimated (e.g. historical/EMA)
    int priority = 0;                    // Static priority or nice level (optional)
    std::string workload_type = "mixed"; // CPU-bound, I/O-bound, mixed, etc.

    // Dynamic execution state
    double remaining_time = 0.0;
    double executed_time = 0.0;
    double start_time = -1.0;
    double completion_time = 0.0;
    double waiting_time = 0.0;
    double turnaround_time = 0.0;
    double response_time = -1.0;
    bool has_started = false;
    bool completed = false;

    // ML Hybrid specific attributes
    bool demoted_to_rr = false;
    double last_ready_time = 0.0;        // Tracked for aging
    double current_wait = 0.0;           // Cumulative wait time
    int context_switch_count = 0;

    void reset() {
        remaining_time = burst_time;
        executed_time = 0.0;
        start_time = -1.0;
        completion_time = 0.0;
        waiting_time = 0.0;
        turnaround_time = 0.0;
        response_time = -1.0;
        has_started = false;
        completed = false;
        demoted_to_rr = false;
        last_ready_time = arrival_time;
        current_wait = 0.0;
        context_switch_count = 0;
    }
};

struct GanttSegment {
    std::string pid;
    double start_time = 0.0;
    double end_time = 0.0;
    bool is_idle = false;
    bool is_context_switch = false;
    std::string queue_type = "NORMAL"; // SJF_QUEUE, RR_FALLBACK, IDLE, CS
};

struct SchedulerMetrics {
    double average_waiting_time = 0.0;
    double average_turnaround_time = 0.0;
    double average_response_time = 0.0;
    double throughput = 0.0;
    double total_simulation_time = 0.0;
    double cpu_busy_time = 0.0;
    double cpu_utilization = 0.0;
    int total_context_switches = 0;
    double jain_fairness_index = 0.0;
    double max_waiting_time = 0.0;
    int starved_process_count = 0;
};

struct SimulationResult {
    std::string algorithm_name;
    SchedulerMetrics metrics;
    std::vector<Process> processes;
    std::vector<GanttSegment> gantt_chart;
};

#endif // PROCESS_HPP
