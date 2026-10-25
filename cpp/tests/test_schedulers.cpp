#include "simulator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>

#define ASSERT_NEAR(a, b, eps) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            std::cerr << "Assertion failed: " #a " (" << (a) << ") ~= " #b " (" << (b) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

void test_fcfs_textbook() {
    std::cout << "[RUNNING] test_fcfs_textbook..." << std::endl;
    SimulatorEngine engine;
    std::vector<Process> procs = {
        {"P1", 0.0, 4.0, 4.0, 4.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 1.0, 3.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P3", 2.0, 1.0, 1.0, 1.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("FCFS", procs);
    // P1: Start=0, End=4, WT=0, TAT=4
    // P2: Start=4, End=7, WT=3, TAT=6
    // P3: Start=7, End=8, WT=5, TAT=6
    // AWT = (0 + 3 + 5) / 3 = 2.6667
    // ATAT = (4 + 6 + 6) / 3 = 5.3333
    ASSERT_NEAR(res.metrics.average_waiting_time, 8.0 / 3.0, 0.01);
    ASSERT_NEAR(res.metrics.average_turnaround_time, 16.0 / 3.0, 0.01);
    ASSERT_NEAR(res.metrics.cpu_utilization, 100.0, 0.01);
    std::cout << "  -> Passed!" << std::endl;
}

void test_simultaneous_arrivals() {
    std::cout << "[RUNNING] test_simultaneous_arrivals..." << std::endl;
    SimulatorEngine engine;
    // P3 (2), P1 (6), P2 (4) arrive simultaneously at t=0
    std::vector<Process> procs = {
        {"P3", 0.0, 2.0, 2.0, 2.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P1", 0.0, 6.0, 6.0, 6.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 0.0, 4.0, 4.0, 4.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    // Oracle SJF execution order in Gantt chart must be: P3 (2), then P2 (4), then P1 (6)
    auto sjf_res = engine.runAlgorithm("ORACLE_SJF", procs);
    ASSERT_TRUE(sjf_res.gantt_chart.size() == 3);
    ASSERT_TRUE(sjf_res.gantt_chart[0].pid == "P3");
    ASSERT_TRUE(sjf_res.gantt_chart[1].pid == "P2");
    ASSERT_TRUE(sjf_res.gantt_chart[2].pid == "P1");

    auto get_p = [&](const std::string& pid) -> Process {
        for (const auto& p : sjf_res.processes) {
            if (p.pid == pid) return p;
        }
        throw std::runtime_error("PID not found");
    };

    // P3 CT=2; P2 CT=6; P1 CT=12
    ASSERT_NEAR(get_p("P3").completion_time, 2.0, 0.001);
    ASSERT_NEAR(get_p("P2").completion_time, 6.0, 0.001);
    ASSERT_NEAR(get_p("P1").completion_time, 12.0, 0.001);
    std::cout << "  -> Passed!" << std::endl;
}

void test_idle_cpu_periods() {
    std::cout << "[RUNNING] test_idle_cpu_periods..." << std::endl;
    SimulatorEngine engine;
    // P1 runs from 0 to 4. P2 arrives at 10. Gap of 6 time units idle.
    std::vector<Process> procs = {
        {"P1", 0.0, 4.0, 4.0, 4.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 10.0, 5.0, 5.0, 5.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("FCFS", procs);
    ASSERT_NEAR(res.metrics.total_simulation_time, 15.0, 0.001);
    ASSERT_NEAR(res.metrics.cpu_busy_time, 9.0, 0.001);
    ASSERT_NEAR(res.metrics.cpu_utilization, (9.0 / 15.0) * 100.0, 0.01);
    
    bool found_idle = false;
    for (const auto& g : res.gantt_chart) {
        if (g.is_idle) {
            found_idle = true;
            ASSERT_NEAR(g.start_time, 4.0, 0.001);
            ASSERT_NEAR(g.end_time, 10.0, 0.001);
        }
    }
    ASSERT_TRUE(found_idle);
    std::cout << "  -> Passed!" << std::endl;
}

void test_context_switch_overhead() {
    std::cout << "[RUNNING] test_context_switch_overhead..." << std::endl;
    SimulatorEngine engine;
    std::vector<Process> procs = {
        {"P1", 0.0, 4.0, 4.0, 4.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 0.0, 4.0, 4.0, 4.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("ROUND_ROBIN", procs, 2.0, 0.5);
    ASSERT_TRUE(res.metrics.total_context_switches >= 3);
    ASSERT_TRUE(res.metrics.total_simulation_time > 8.0);
    std::cout << "  -> Total CS count: " << res.metrics.total_context_switches 
              << ", Total makespan: " << res.metrics.total_simulation_time << " -> Passed!" << std::endl;
}

void test_oracle_vs_estimated_sjf() {
    std::cout << "[RUNNING] test_oracle_vs_estimated_sjf..." << std::endl;
    SimulatorEngine engine;
    // P1: Actual=10, Estimated=2
    // P2: Actual=3, Estimated=8
    std::vector<Process> procs = {
        {"P1", 0.0, 10.0, 2.0, 2.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 0.0, 3.0, 8.0, 8.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto oracle = engine.runAlgorithm("ORACLE_SJF", procs);
    auto estimated = engine.runAlgorithm("ESTIMATED_SJF", procs);

    // Oracle picks P2 first (actual 3 < 10)
    ASSERT_TRUE(oracle.gantt_chart[0].pid == "P2");
    // Estimated picks P1 first (estimated 2 < 8)
    ASSERT_TRUE(estimated.gantt_chart[0].pid == "P1");
    // Oracle average waiting time should be lower than estimated because it is the optimal non-preemptive SJF
    ASSERT_TRUE(oracle.metrics.average_waiting_time < estimated.metrics.average_waiting_time);
    std::cout << "  -> Oracle AWT: " << oracle.metrics.average_waiting_time 
              << " vs Estimated AWT: " << estimated.metrics.average_waiting_time << " -> Passed!" << std::endl;
}

void test_ml_hybrid_underestimation_and_demotion() {
    std::cout << "[RUNNING] test_ml_hybrid_underestimation_and_demotion..." << std::endl;
    SimulatorEngine engine;
    // P_rogue: Actual = 12.0, Predicted = 3.0 (underestimated by ML!)
    // P_normal: Arrives at t=2.0, Actual = 2.0, Predicted = 2.0
    std::vector<Process> procs = {
        {"P_rogue", 0.0, 12.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P_normal", 2.0, 2.0, 2.0, 2.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("ML_SJF_HYBRID", procs, 2.0, 0.0, 0.1, 2.0);
    
    // Find P_rogue in results
    bool demoted = false;
    for (const auto& p : res.processes) {
        if (p.pid == "P_rogue") {
            demoted = p.demoted_to_rr;
        }
    }
    ASSERT_TRUE(demoted);

    // Verify Gantt chart segments: P_rogue ran under SJF_QUEUE, was demoted, P_normal got CPU, then P_rogue ran under RR_FALLBACK
    bool found_rr_fallback = false;
    for (const auto& g : res.gantt_chart) {
        if (g.pid == "P_rogue" && g.queue_type == "RR_FALLBACK") {
            found_rr_fallback = true;
        }
    }
    ASSERT_TRUE(found_rr_fallback);
    std::cout << "  -> Verified underestimation preemption and demotion to RR fallback! -> Passed!" << std::endl;
}

void test_ml_hybrid_aging_prevents_starvation() {
    std::cout << "[RUNNING] test_ml_hybrid_aging_prevents_starvation..." << std::endl;
    SimulatorEngine engine;
    // P_big: arrives at 0, burst = 8.0
    // Stream of 4 short jobs (burst = 3.0) arriving at t=0, 3, 6, 9
    auto make_procs = []() -> std::vector<Process> {
        return {
            {"P_big", 0.0, 8.0, 8.0, 8.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
            {"P_s1", 0.0, 3.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
            {"P_s2", 3.0, 3.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
            {"P_s3", 6.0, 3.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
            {"P_s4", 9.0, 3.0, 3.0, 3.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
        };
    };

    // Run with aging factor 1.0 (at t=6, P_big's aged score becomes 8 - 1.0*6 = 2.0 < 3.0, preempting P_s3!)
    auto res_aged = engine.runAlgorithm("ML_SJF_HYBRID", make_procs(), 2.0, 0.0, 1.0, 2.0);
    double start_aged = -1.0;
    for (const auto& p : res_aged.processes) {
        if (p.pid == "P_big") start_aged = p.start_time;
    }

    // Run without aging (aging = 0.0, P_big has to wait for all short jobs to complete)
    auto res_no_aging = engine.runAlgorithm("ML_SJF_HYBRID", make_procs(), 2.0, 0.0, 0.0, 2.0);
    double start_no_aging = -1.0;
    for (const auto& p : res_no_aging.processes) {
        if (p.pid == "P_big") start_no_aging = p.start_time;
    }

    ASSERT_NEAR(start_aged, 6.0, 0.001);
    ASSERT_NEAR(start_no_aging, 12.0, 0.001);
    ASSERT_TRUE(start_aged < start_no_aging);
    std::cout << "  -> P_big scheduled at t=" << start_aged << " (with aging) vs t=" 
              << start_no_aging << " (without aging) -> Aging effectively prevents starvation!" << std::endl;
}

void test_adaptive_rr() {
    std::cout << "[RUNNING] test_adaptive_rr..." << std::endl;
    SimulatorEngine engine;
    std::vector<Process> procs = {
        {"P1", 0.0, 10.0, 10.0, 10.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 0.0, 20.0, 20.0, 20.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P3", 0.0, 30.0, 30.0, 30.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("ADAPTIVE_RR", procs, 2.0);
    ASSERT_TRUE(res.processes.size() == 3);
    for (const auto& p : res.processes) {
        ASSERT_TRUE(p.completed);
    }
    std::cout << "  -> Adaptive RR completed successfully with ATT: " << res.metrics.average_turnaround_time << std::endl;
}

void test_fairness_and_metrics() {
    std::cout << "[RUNNING] test_fairness_and_metrics..." << std::endl;
    SimulatorEngine engine;
    std::vector<Process> procs = {
        {"P1", 0.0, 5.0, 5.0, 5.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0},
        {"P2", 0.0, 5.0, 5.0, 5.0, 0, "mixed", 0, 0, 0, 0, 0, 0, 0, false, false, false, 0, 0, 0}
    };
    auto res = engine.runAlgorithm("ROUND_ROBIN", procs, 2.0);
    ASSERT_TRUE(res.metrics.jain_fairness_index >= 0.95);
    std::cout << "  -> Jain's Fairness Index: " << res.metrics.jain_fairness_index << " -> Passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " RUNNING C++ SCHEDULING ENGINE TESTS    " << std::endl;
    std::cout << "========================================" << std::endl;

    test_fcfs_textbook();
    test_simultaneous_arrivals();
    test_idle_cpu_periods();
    test_context_switch_overhead();
    test_oracle_vs_estimated_sjf();
    test_ml_hybrid_underestimation_and_demotion();
    test_ml_hybrid_aging_prevents_starvation();
    test_adaptive_rr();
    test_fairness_and_metrics();

    std::cout << "========================================" << std::endl;
    std::cout << " ALL C++ UNIT TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
