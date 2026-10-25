"""
Unit and Integration Tests for C++ Scheduling Engine Bridge.
"""

import pytest
import os
import sys

BACKEND_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "backend")
if BACKEND_DIR not in sys.path:
    sys.path.append(BACKEND_DIR)

from cpp_bridge import run_cpp_simulation

def test_fcfs_basic_correctness():
    processes = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 4.0},
        {"pid": "P2", "arrival_time": 1.0, "burst_time": 3.0},
        {"pid": "P3", "arrival_time": 2.0, "burst_time": 1.0}
    ]
    res = run_cpp_simulation(processes=processes, algorithm="FCFS")
    assert res["algorithm_name"] == "FCFS"
    metrics = res["metrics"]
    # Hand calculation:
    # P1: start=0, finish=4, wait=0, tat=4
    # P2: start=4, finish=7, wait=3, tat=6
    # P3: start=7, finish=8, wait=5, tat=6
    # AWT = 8/3 = 2.6667, ATT = 16/3 = 5.3333
    assert abs(metrics["average_waiting_time"] - 2.6667) < 0.01
    assert abs(metrics["average_turnaround_time"] - 5.3333) < 0.01
    assert metrics["cpu_utilization"] == 100.0

def test_simultaneous_arrivals():
    # P1, P2, P3 arrive at 0.0
    processes = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 8.0, "predicted_burst_time": 8.0},
        {"pid": "P2", "arrival_time": 0.0, "burst_time": 2.0, "predicted_burst_time": 2.0},
        {"pid": "P3", "arrival_time": 0.0, "burst_time": 4.0, "predicted_burst_time": 4.0}
    ]
    res = run_cpp_simulation(processes=processes, algorithm="ORACLE_SJF")
    # In Oracle SJF, execution sequence must be P2 (2), P3 (4), P1 (8)
    gantt = res["gantt_chart"]
    assert len(gantt) == 3
    assert gantt[0]["pid"] == "P2"
    assert gantt[1]["pid"] == "P3"
    assert gantt[2]["pid"] == "P1"

def test_idle_cpu_handling():
    processes = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 2.0},
        {"pid": "P2", "arrival_time": 6.0, "burst_time": 3.0}
    ]
    res = run_cpp_simulation(processes=processes, algorithm="FCFS")
    metrics = res["metrics"]
    assert metrics["total_simulation_time"] == 9.0
    assert metrics["cpu_busy_time"] == 5.0
    # CPU idle from 2.0 to 6.0
    idle_segs = [g for g in res["gantt_chart"] if g["is_idle"]]
    assert len(idle_segs) == 1
    assert idle_segs[0]["start_time"] == 2.0
    assert idle_segs[0]["end_time"] == 6.0

def test_context_switch_penalty():
    processes = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 4.0},
        {"pid": "P2", "arrival_time": 0.0, "burst_time": 4.0}
    ]
    # Quantum = 2.0, Context switch = 0.5
    res = run_cpp_simulation(
        processes=processes,
        algorithm="ROUND_ROBIN",
        time_quantum=2.0,
        context_switch_overhead=0.5
    )
    metrics = res["metrics"]
    assert metrics["total_context_switches"] >= 3
    assert metrics["total_simulation_time"] > 8.0

def test_ml_hybrid_demotion_on_underestimation():
    processes = [
        {"pid": "P_Under", "arrival_time": 0.0, "burst_time": 10.0, "predicted_burst_time": 2.0},
        {"pid": "P_Normal", "arrival_time": 1.0, "burst_time": 2.0, "predicted_burst_time": 2.0}
    ]
    res = run_cpp_simulation(processes=processes, algorithm="ML_SJF_HYBRID", fallback_quantum=2.0)
    # P_Under must be demoted to RR fallback queue
    procs = res["processes"]
    p_under = next(p for p in procs if p["pid"] == "P_Under")
    assert p_under["demoted_to_rr"] is True

def test_all_six_configurations():
    processes = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 5.0, "predicted_burst_time": 4.5, "estimated_burst_time": 5.0},
        {"pid": "P2", "arrival_time": 1.0, "burst_time": 2.0, "predicted_burst_time": 2.0, "estimated_burst_time": 2.0}
    ]
    res = run_cpp_simulation(processes=processes, algorithm="ALL")
    assert "results" in res
    assert len(res["results"]) == 6
    names = [r["algorithm_name"] for r in res["results"]]
    assert any("FCFS" in n for n in names)
    assert any("Round Robin" in n for n in names)
    assert any("Oracle SJF" in n for n in names)
    assert any("Estimated SJF" in n for n in names)
    assert any("Adaptive" in n for n in names)
    assert any("Hybrid" in n for n in names)
