"""
Unit Tests for Machine Learning Subsystem and Anti-Leakage Constraints.
"""

import pytest
import os
import sys
import numpy as np
import pandas as pd

ML_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "ml")
if ML_DIR not in sys.path:
    sys.path.append(ML_DIR)

from dataset_generator import generate_workload_processes, generate_multiworkload_dataset
from feature_pipeline import extract_features_and_target, chronological_split, FEATURE_COLUMNS, TARGET_COLUMN
from predict import predict_burst_times, get_model

def test_data_leakage_prevention():
    df = generate_workload_processes(workload_type="mixed", n_samples=50, seed=123)
    X, y = extract_features_and_target(df)
    
    # Assert target is not in feature matrix
    assert TARGET_COLUMN not in X.columns
    for col in FEATURE_COLUMNS:
        assert col in X.columns
        assert col != "burst_time"
        
    assert len(X) == 50
    assert len(y) == 50

def test_chronological_split():
    df = generate_multiworkload_dataset(n_per_workload=50, seed=42)
    train_df, test_df = chronological_split(df, train_ratio=0.8)
    
    assert len(train_df) == int(len(df) * 0.8)
    assert len(test_df) == len(df) - len(train_df)
    
    # Verify chronological integrity: max train arrival <= min test arrival
    max_train_arrival = train_df["arrival_time"].max()
    min_test_arrival = test_df["arrival_time"].min()
    assert max_train_arrival <= min_test_arrival + 1e-5

def test_model_inference_and_metrics():
    procs = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 12.0, "workload_type": "cpu_bound"},
        {"pid": "P2", "arrival_time": 1.0, "burst_time": 3.0, "workload_type": "io_bound"},
        {"pid": "P3", "arrival_time": 2.0, "burst_time": 6.0, "workload_type": "mixed"}
    ]
    
    enriched, meta = predict_burst_times(procs, model_name="decision_tree")
    assert len(enriched) == 3
    for p in enriched:
        assert "predicted_burst_time" in p
        assert p["predicted_burst_time"] > 0.0
        
    assert "mae" in meta
    assert "rmse" in meta
    assert "mape_percent" in meta
    assert "avg_inference_time_us" in meta
    assert meta["avg_inference_time_us"] > 0.0

def test_model_loading_and_fallback():
    # Valid model
    dt_model = get_model("decision_tree")
    assert dt_model is not None
    
    rf_model = get_model("random_forest")
    assert rf_model is not None
    
    # Fallback to decision_tree for unknown name
    fallback = get_model("unknown_model_xyz")
    assert fallback is not None
