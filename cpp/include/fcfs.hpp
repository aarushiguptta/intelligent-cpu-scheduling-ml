#ifndef FCFS_HPP
#define FCFS_HPP

#include "scheduler.hpp"

class FCFSScheduler : public BaseScheduler {
public:
    std::string getName() const override { return "FCFS"; }
    SimulationResult schedule(
        std::vector<Process> processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    ) override;
};

#endif // FCFS_HPP
