#ifndef ML_HYBRID_HPP
#define ML_HYBRID_HPP

#include "scheduler.hpp"

class MLSJFHybridScheduler : public BaseScheduler {
public:
    std::string getName() const override { return "ML-SJF Hybrid (Aging + Demotion)"; }
    SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) override;
};

#endif // ML_HYBRID_HPP
