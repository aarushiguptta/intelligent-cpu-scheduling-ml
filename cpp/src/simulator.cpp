#include "simulator.hpp"
#include <stdexcept>
#include <algorithm>

SimulatorEngine::SimulatorEngine() {
    schedulers_["FCFS"] = std::make_shared<FCFSScheduler>();
    schedulers_["ROUND_ROBIN"] = std::make_shared<RoundRobinScheduler>();
    schedulers_["ORACLE_SJF"] = std::make_shared<OracleSJFScheduler>();
    schedulers_["ESTIMATED_SJF"] = std::make_shared<EstimatedSJFScheduler>();
    schedulers_["ADAPTIVE_RR"] = std::make_shared<AdaptiveRRScheduler>();
    schedulers_["ML_SJF_HYBRID"] = std::make_shared<MLSJFHybridScheduler>();
}

SimulationResult SimulatorEngine::runAlgorithm(
    const std::string& algo_name,
    const std::vector<Process>& processes,
    double time_quantum,
    double context_switch_overhead,
    double aging_factor,
    double fallback_quantum
) {
    std::string key = algo_name;
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    if (key == "RR") key = "ROUND_ROBIN";
    if (key == "SJF" || key == "SJF_ORACLE") key = "ORACLE_SJF";
    if (key == "SJF_ESTIMATED") key = "ESTIMATED_SJF";
    if (key == "ADAPTIVE" || key == "ARR") key = "ADAPTIVE_RR";
    if (key == "ML" || key == "HYBRID" || key == "ML_HYBRID") key = "ML_SJF_HYBRID";

    auto it = schedulers_.find(key);
    if (it != schedulers_.end()) {
        return it->second->schedule(processes, time_quantum, context_switch_overhead, aging_factor, fallback_quantum);
    }
    throw std::runtime_error("Unknown scheduling algorithm: " + algo_name);
}

std::vector<SimulationResult> SimulatorEngine::runAll(
    const std::vector<Process>& processes,
    double time_quantum,
    double context_switch_overhead,
    double aging_factor,
    double fallback_quantum
) {
    std::vector<SimulationResult> results;
    // Standard execution order for the 6 configurations
    const std::vector<std::string> ordered_keys = {
        "FCFS",
        "ROUND_ROBIN",
        "ORACLE_SJF",
        "ESTIMATED_SJF",
        "ADAPTIVE_RR",
        "ML_SJF_HYBRID"
    };

    for (const auto& key : ordered_keys) {
        results.push_back(runAlgorithm(key, processes, time_quantum, context_switch_overhead, aging_factor, fallback_quantum));
    }
    return results;
}
