#include "simulator.hpp"
#include "json_helper.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    try {
        std::string json_input;
        bool run_all = false;
        std::string algo = "";

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--all") {
                run_all = true;
            } else if (arg == "--algo" && i + 1 < argc) {
                algo = argv[++i];
            } else if (arg == "--file" && i + 1 < argc) {
                std::ifstream ifs(argv[++i]);
                if (!ifs.is_open()) {
                    std::cerr << "{\"error\": \"Could not open input file\"}\n";
                    return 1;
                }
                std::stringstream buffer;
                buffer << ifs.rdbuf();
                json_input = buffer.str();
            } else if (arg == "--json" && i + 1 < argc) {
                json_input = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: scheduler_engine [--all] [--algo <ALGO>] [--file <path>] [--json <json_string>]\n";
                std::cout << "Algorithms: FCFS, ROUND_ROBIN, ORACLE_SJF, ESTIMATED_SJF, ADAPTIVE_RR, ML_SJF_HYBRID\n";
                return 0;
            }
        }

        if (json_input.empty()) {
            // Read from standard input
            std::stringstream buffer;
            buffer << std::cin.rdbuf();
            json_input = buffer.str();
        }

        if (json_input.empty()) {
            std::cerr << "{\"error\": \"Empty JSON input provided\"}\n";
            return 1;
        }

        SimpleJson::SimConfig config = SimpleJson::parseSimulationConfig(json_input);

        SimulatorEngine engine;

        if (run_all || config.algorithm == "ALL") {
            auto results = engine.runAll(
                config.processes,
                config.time_quantum,
                config.context_switch_overhead,
                config.aging_factor,
                config.fallback_quantum
            );
            std::cout << SimpleJson::multiResultsToJson(results) << std::endl;
        } else {
            std::string target_algo = algo.empty() ? config.algorithm : algo;
            auto result = engine.runAlgorithm(
                target_algo,
                config.processes,
                config.time_quantum,
                config.context_switch_overhead,
                config.aging_factor,
                config.fallback_quantum
            );
            std::cout << SimpleJson::resultToJson(result) << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "{\"error\": \"" << SimpleJson::escapeString(e.what()) << "\"}\n";
        return 1;
    }
}
