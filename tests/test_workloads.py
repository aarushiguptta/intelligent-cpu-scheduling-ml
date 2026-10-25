"""
Unit Tests for the Five Workload Types and Telemetry Generation.
"""

import pytest
import os
import sys

BACKEND_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "backend")
if BACKEND_DIR not in sys.path:
    sys.path.append(BACKEND_DIR)

from workload_generator import get_workload_sample, WORKLOAD_DESCRIPTIONS, SIMULATION_IO_NOTE

WORKLOAD_TYPES = ["cpu_bound", "io_bound", "mixed", "heavy_tailed", "bursty"]

def test_workload_catalog_completeness():
    for w in WORKLOAD_TYPES:
        assert w in WORKLOAD_DESCRIPTIONS
        info = WORKLOAD_DESCRIPTIONS[w]
        assert "title" in info
        assert "description" in info
        assert "io_characteristics" in info
    assert len(SIMULATION_IO_NOTE) > 50

def test_workload_generation_properties():
    for w in WORKLOAD_TYPES:
        sample = get_workload_sample(workload_type=w, count=15, seed=100)
        assert len(sample) == 15
        
        for p in sample:
            assert p["arrival_time"] >= 0.0
            assert p["burst_time"] > 0.0
            assert p["memory_mb"] > 0.0
            assert p["past_burst_avg"] > 0.0
            assert 0.0 <= p["past_io_rate"] <= 1.0

def test_workload_behavioral_differences():
    # CPU-bound mean burst should be noticeably higher than I/O-bound mean burst
    cpu_sample = get_workload_sample(workload_type="cpu_bound", count=40, seed=42)
    io_sample = get_workload_sample(workload_type="io_bound", count=40, seed=42)
    
    cpu_mean_burst = sum(p["burst_time"] for p in cpu_sample) / len(cpu_sample)
    io_mean_burst = sum(p["burst_time"] for p in io_sample) / len(io_sample)
    
    assert cpu_mean_burst > io_mean_burst * 2.0
    
    # I/O rate of I/O bound should be higher than CPU bound
    io_mean_rate = sum(p["past_io_rate"] for p in io_sample) / len(io_sample)
    cpu_mean_rate = sum(p["past_io_rate"] for p in cpu_sample) / len(cpu_sample)
    assert io_mean_rate > cpu_mean_rate

def test_reproducibility():
    sample1 = get_workload_sample(workload_type="mixed", count=10, seed=99)
    sample2 = get_workload_sample(workload_type="mixed", count=10, seed=99)
    assert sample1 == sample2
