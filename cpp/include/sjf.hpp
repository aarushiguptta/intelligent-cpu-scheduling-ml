#ifndef SJF_HPP
#define SJF_HPP

#include "scheduler.hpp"

class OracleSJFScheduler : public BaseScheduler {
public:
    std::string getName() const override { return "Oracle SJF (Actual Burst)"; }
    SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) override;
};

class EstimatedSJFScheduler : public BaseScheduler {
public:
    std::string getName() const override { return "Estimated SJF (Historical/EMA)"; }
    SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) override;
};

#endif // SJF_HPP
