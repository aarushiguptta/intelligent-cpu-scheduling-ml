#ifndef ADAPTIVE_RR_HPP
#define ADAPTIVE_RR_HPP

#include "scheduler.hpp"

class AdaptiveRRScheduler : public BaseScheduler {
public:
    std::string getName() const override { return "Adaptive Round Robin"; }
    SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) override;
};

#endif // ADAPTIVE_RR_HPP
