#ifndef JSON_HELPER_HPP
#define JSON_HELPER_HPP

#include "process.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <map>
#include <stdexcept>
#include <cmath>

namespace SimpleJson {

inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

inline std::string escapeString(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

struct SimConfig {
    std::string algorithm = "FCFS";
    double time_quantum = 2.0;
    double context_switch_overhead = 0.0;
    double aging_factor = 0.1;
    double fallback_quantum = 2.0;
    std::vector<Process> processes;
};

// Simple tokenizer-based parser for our simulation input JSON
inline SimConfig parseSimulationConfig(const std::string& json_str) {
    SimConfig config;
    // We look for keys: "algorithm", "time_quantum", "context_switch_overhead", "aging_factor", "processes"
    // Let's implement a robust recursive-descent or stateful parser for our structured schema
    size_t pos = 0;
    auto skip_whitespace = [&](size_t& p) {
        while (p < json_str.size() && (std::isspace(json_str[p]) || json_str[p] == ',')) p++;
    };

    auto read_string = [&](size_t& p) -> std::string {
        skip_whitespace(p);
        if (p >= json_str.size() || json_str[p] != '"') return "";
        p++; // skip quote
        std::string res;
        while (p < json_str.size()) {
            if (json_str[p] == '\\' && p + 1 < json_str.size()) {
                res += json_str[p + 1];
                p += 2;
            } else if (json_str[p] == '"') {
                p++;
                break;
            } else {
                res += json_str[p++];
            }
        }
        return res;
    };

    auto read_number = [&](size_t& p) -> double {
        skip_whitespace(p);
        size_t start = p;
        if (p < json_str.size() && (json_str[p] == '-' || json_str[p] == '+')) p++;
        while (p < json_str.size() && (std::isdigit(json_str[p]) || json_str[p] == '.' || json_str[p] == 'e' || json_str[p] == 'E')) {
            p++;
        }
        if (start == p) return 0.0;
        try {
            return std::stod(json_str.substr(start, p - start));
        } catch (...) {
            return 0.0;
        }
    };

    while (pos < json_str.size()) {
        skip_whitespace(pos);
        if (pos >= json_str.size()) break;
        if (json_str[pos] == '{' || json_str[pos] == '}' || json_str[pos] == ':') {
            pos++;
            continue;
        }
        if (json_str[pos] == '"') {
            std::string key = read_string(pos);
            skip_whitespace(pos);
            if (pos < json_str.size() && json_str[pos] == ':') pos++;
            skip_whitespace(pos);

            if (key == "algorithm") {
                config.algorithm = read_string(pos);
            } else if (key == "time_quantum" || key == "quantum") {
                config.time_quantum = read_number(pos);
            } else if (key == "context_switch_overhead" || key == "context_switch") {
                config.context_switch_overhead = read_number(pos);
            } else if (key == "aging_factor") {
                config.aging_factor = read_number(pos);
            } else if (key == "fallback_quantum") {
                config.fallback_quantum = read_number(pos);
            } else if (key == "processes") {
                // Parse process array
                skip_whitespace(pos);
                if (pos < json_str.size() && json_str[pos] == '[') {
                    pos++;
                    while (pos < json_str.size()) {
                        skip_whitespace(pos);
                        if (pos >= json_str.size() || json_str[pos] == ']') {
                            if (pos < json_str.size()) pos++;
                            break;
                        }
                        if (json_str[pos] == '{') {
                            pos++;
                            Process proc;
                            while (pos < json_str.size()) {
                                skip_whitespace(pos);
                                if (pos >= json_str.size() || json_str[pos] == '}') {
                                    if (pos < json_str.size()) pos++;
                                    break;
                                }
                                if (json_str[pos] == '"') {
                                    std::string pkey = read_string(pos);
                                    skip_whitespace(pos);
                                    if (pos < json_str.size() && json_str[pos] == ':') pos++;
                                    skip_whitespace(pos);
                                    if (pkey == "pid") {
                                        proc.pid = read_string(pos);
                                    } else if (pkey == "arrival_time") {
                                        proc.arrival_time = read_number(pos);
                                    } else if (pkey == "burst_time") {
                                        proc.burst_time = read_number(pos);
                                    } else if (pkey == "predicted_burst_time" || pkey == "predicted_burst") {
                                        proc.predicted_burst_time = read_number(pos);
                                    } else if (pkey == "estimated_burst_time" || pkey == "estimated_burst") {
                                        proc.estimated_burst_time = read_number(pos);
                                    } else if (pkey == "priority") {
                                        proc.priority = (int)read_number(pos);
                                    } else if (pkey == "workload_type") {
                                        proc.workload_type = read_string(pos);
                                    } else {
                                        // Skip unknown value
                                        if (json_str[pos] == '"') read_string(pos);
                                        else read_number(pos);
                                    }
                                } else {
                                    pos++;
                                }
                            }
                            proc.remaining_time = proc.burst_time;
                            if (proc.predicted_burst_time <= 0.0) {
                                proc.predicted_burst_time = proc.burst_time;
                            }
                            if (proc.estimated_burst_time <= 0.0) {
                                proc.estimated_burst_time = proc.burst_time;
                            }
                            config.processes.push_back(proc);
                        } else {
                            pos++;
                        }
                    }
                }
            } else {
                // Unknown top-level key: advance
                pos++;
            }
        } else {
            pos++;
        }
    }
    return config;
}

inline std::string resultToJson(const SimulationResult& res) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "{\n";
    ss << "  \"algorithm_name\": \"" << escapeString(res.algorithm_name) << "\",\n";
    
    // Metrics
    ss << "  \"metrics\": {\n";
    ss << "    \"average_waiting_time\": " << res.metrics.average_waiting_time << ",\n";
    ss << "    \"average_turnaround_time\": " << res.metrics.average_turnaround_time << ",\n";
    ss << "    \"average_response_time\": " << res.metrics.average_response_time << ",\n";
    ss << "    \"throughput\": " << res.metrics.throughput << ",\n";
    ss << "    \"total_simulation_time\": " << res.metrics.total_simulation_time << ",\n";
    ss << "    \"cpu_busy_time\": " << res.metrics.cpu_busy_time << ",\n";
    ss << "    \"cpu_utilization\": " << res.metrics.cpu_utilization << ",\n";
    ss << "    \"total_context_switches\": " << res.metrics.total_context_switches << ",\n";
    ss << "    \"jain_fairness_index\": " << res.metrics.jain_fairness_index << ",\n";
    ss << "    \"max_waiting_time\": " << res.metrics.max_waiting_time << ",\n";
    ss << "    \"starved_process_count\": " << res.metrics.starved_process_count << "\n";
    ss << "  },\n";

    // Processes
    ss << "  \"processes\": [\n";
    for (size_t i = 0; i < res.processes.size(); ++i) {
        const auto& p = res.processes[i];
        ss << "    {\n";
        ss << "      \"pid\": \"" << escapeString(p.pid) << "\",\n";
        ss << "      \"arrival_time\": " << p.arrival_time << ",\n";
        ss << "      \"burst_time\": " << p.burst_time << ",\n";
        ss << "      \"predicted_burst_time\": " << p.predicted_burst_time << ",\n";
        ss << "      \"estimated_burst_time\": " << p.estimated_burst_time << ",\n";
        ss << "      \"start_time\": " << p.start_time << ",\n";
        ss << "      \"completion_time\": " << p.completion_time << ",\n";
        ss << "      \"waiting_time\": " << p.waiting_time << ",\n";
        ss << "      \"turnaround_time\": " << p.turnaround_time << ",\n";
        ss << "      \"response_time\": " << p.response_time << ",\n";
        ss << "      \"demoted_to_rr\": " << (p.demoted_to_rr ? "true" : "false") << ",\n";
        ss << "      \"context_switch_count\": " << p.context_switch_count << "\n";
        ss << "    }" << (i + 1 < res.processes.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Gantt Chart
    ss << "  \"gantt_chart\": [\n";
    for (size_t i = 0; i < res.gantt_chart.size(); ++i) {
        const auto& g = res.gantt_chart[i];
        ss << "    {\n";
        ss << "      \"pid\": \"" << escapeString(g.pid) << "\",\n";
        ss << "      \"start_time\": " << g.start_time << ",\n";
        ss << "      \"end_time\": " << g.end_time << ",\n";
        ss << "      \"is_idle\": " << (g.is_idle ? "true" : "false") << ",\n";
        ss << "      \"is_context_switch\": " << (g.is_context_switch ? "true" : "false") << ",\n";
        ss << "      \"queue_type\": \"" << escapeString(g.queue_type) << "\"\n";
        ss << "    }" << (i + 1 < res.gantt_chart.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

inline std::string multiResultsToJson(const std::vector<SimulationResult>& results) {
    std::ostringstream ss;
    ss << "{\n  \"results\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        ss << resultToJson(results[i]);
        if (i + 1 < results.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n}";
    return ss.str();
}

} // namespace SimpleJson

#endif // JSON_HELPER_HPP
