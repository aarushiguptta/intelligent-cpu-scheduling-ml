#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include "process.hpp"
#include "fcfs.hpp"
#include "sjf.hpp"
#include "round_robin.hpp"
#include "adaptive_rr.hpp"
#include "ml_hybrid.hpp"
#include <memory>
#include <vector>
#include <map>

class SimulatorEngine {
public:
    SimulatorEngine();
    
    SimulationResult runAlgorithm(
        const std::string& algo_name,
        const std::vector<Process>& processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    );

    std::vector<SimulationResult> runAll(
        const std::vector<Process>& processes,
        double time_quantum = 2.0,
        double context_switch_overhead = 0.0,
        double aging_factor = 0.1,
        double fallback_quantum = 2.0
    );

private:
    std::map<std::string, std::shared_ptr<BaseScheduler>> schedulers_;
};

#endif // SIMULATOR_HPP
