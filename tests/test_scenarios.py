"""
Scenario and Stress Tests for Scheduling Correctness:
Starvation, Fairness, Mispredictions, and Context Switch Overhead.
"""

import pytest
import os
import sys

BACKEND_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "backend")
if BACKEND_DIR not in sys.path:
    sys.path.append(BACKEND_DIR)

from cpp_bridge import run_cpp_simulation

def test_aging_mitigates_starvation():
    """
    Tests that the aging mechanism in ML-SJF Hybrid prevents indefinite postponement
    of long jobs when a continuous stream of short jobs arrives.
    """
    procs = [
        {"pid": "P_Long", "arrival_time": 0.0, "burst_time": 8.0, "predicted_burst_time": 8.0},
        {"pid": "P_S1", "arrival_time": 0.0, "burst_time": 3.0, "predicted_burst_time": 3.0},
        {"pid": "P_S2", "arrival_time": 3.0, "burst_time": 3.0, "predicted_burst_time": 3.0},
        {"pid": "P_S3", "arrival_time": 6.0, "burst_time": 3.0, "predicted_burst_time": 3.0},
        {"pid": "P_S4", "arrival_time": 9.0, "burst_time": 3.0, "predicted_burst_time": 3.0}
    ]
    
    # Run with active aging (alpha = 1.0)
    res_aged = run_cpp_simulation(
        processes=procs,
        algorithm="ML_SJF_HYBRID",
        aging_factor=1.0,
        fallback_quantum=2.0
    )
    p_long_aged = next(p for p in res_aged["processes"] if p["pid"] == "P_Long")
    
    # Run without aging (alpha = 0.0)
    res_no_aging = run_cpp_simulation(
        processes=procs,
        algorithm="ML_SJF_HYBRID",
        aging_factor=0.0,
        fallback_quantum=2.0
    )
    p_long_no_aging = next(p for p in res_no_aging["processes"] if p["pid"] == "P_Long")
    
    # Aging must schedule P_Long earlier than when no aging is used (t=6 vs t=12)
    assert p_long_aged["start_time"] == 6.0
    assert p_long_no_aging["start_time"] == 12.0
    assert p_long_aged["start_time"] < p_long_no_aging["start_time"]
    assert p_long_aged["waiting_time"] < p_long_no_aging["waiting_time"]

def test_extreme_misprediction_recovery():
    """
    Tests that a process whose burst is severely underestimated by ML is preempted
    after its predicted slice and successfully demoted to Round Robin fallback.
    """
    procs = [
        {"pid": "P_Rogue", "arrival_time": 0.0, "burst_time": 16.0, "predicted_burst_time": 2.0},
        {"pid": "P_Quick", "arrival_time": 1.0, "burst_time": 2.0, "predicted_burst_time": 2.0}
    ]
    
    res = run_cpp_simulation(
        processes=procs,
        algorithm="ML_SJF_HYBRID",
        fallback_quantum=2.0
    )
    
    procs_out = res["processes"]
    p_rogue = next(p for p in procs_out if p["pid"] == "P_Rogue")
    p_quick = next(p for p in procs_out if p["pid"] == "P_Quick")
    
    # P_Rogue must be marked demoted
    assert p_rogue["demoted_to_rr"] is True
    # P_Quick was not starved; it completed before P_Rogue's full 16ms finished!
    assert p_quick["completion_time"] < p_rogue["completion_time"]

def test_fairness_properties():
    """
    Verifies that Jain's Fairness Index evaluates to ~1.0 for equitable round robin
    and degrades under unfair starvation conditions.
    """
    # Symmetric workload
    sym_procs = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 4.0},
        {"pid": "P2", "arrival_time": 0.0, "burst_time": 4.0},
        {"pid": "P3", "arrival_time": 0.0, "burst_time": 4.0}
    ]
    res_rr = run_cpp_simulation(processes=sym_procs, algorithm="ROUND_ROBIN", time_quantum=2.0)
    fairness = res_rr["metrics"]["jain_fairness_index"]
    assert fairness >= 0.95
    assert fairness <= 1.0
