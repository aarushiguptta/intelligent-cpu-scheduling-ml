"""
ML Inference Service for CPU Burst Prediction.
Provides prediction with latency profiling and automatic feature imputation for user inputs.
"""

import os
import time
import joblib
import numpy as np
import pandas as pd
from typing import List, Dict, Any, Tuple

from feature_pipeline import FEATURE_COLUMNS

MODEL_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "models")

_LOADED_MODELS: Dict[str, Any] = {}

def get_model(model_name: str = "decision_tree"):
    """Loads and caches the ML model."""
    name = model_name.lower().replace(" ", "_")
    if name not in ["decision_tree", "random_forest", "gradient_boosting"]:
        name = "decision_tree"
        
    if name not in _LOADED_MODELS:
        path = os.path.join(MODEL_DIR, f"{name}.joblib")
        if not os.path.exists(path):
            raise FileNotFoundError(f"Model file not found: {path}. Run train_model.py first.")
        _LOADED_MODELS[name] = joblib.load(path)
    return _LOADED_MODELS[name]

def impute_missing_features(proc: Dict[str, Any]) -> Dict[str, Any]:
    """
    If user manually creates a process without all telemetry features,
    synthesizes realistic pre-scheduling features without data leakage.
    """
    p = proc.copy()
    
    # Priority
    priority = p.get("priority", 0)
    
    # Workload category
    workload_type = p.get("workload_type", "mixed")
    cat_map = {"io_bound": 0, "mixed": 1, "cpu_bound": 2, "bursty": 1, "heavy_tailed": 2}
    cat = cat_map.get(workload_type, 1)
    
    # Pre-scheduling features default values
    if "process_category" not in p:
        p["process_category"] = cat
    if "memory_mb" not in p:
        p["memory_mb"] = 256.0 if cat == 0 else (2048.0 if cat == 2 else 512.0)
    if "priority_level" not in p:
        p["priority_level"] = priority
    if "thread_count" not in p:
        p["thread_count"] = 1 if cat == 0 else (8 if cat == 2 else 2)
    if "past_burst_avg" not in p:
        # If user supplied estimated_burst_time, use that as past average; otherwise use baseline heuristic
        p["past_burst_avg"] = float(p.get("estimated_burst_time", p.get("burst_time", 5.0)))
    if "past_io_rate" not in p:
        p["past_io_rate"] = 0.8 if cat == 0 else (0.05 if cat == 2 else 0.35)
    if "arrival_interval" not in p:
        p["arrival_interval"] = 2.0
    if "queue_length_at_arrival" not in p:
        p["queue_length_at_arrival"] = 1
        
    return p

def predict_burst_times(
    processes: List[Dict[str, Any]],
    model_name: str = "decision_tree"
) -> Tuple[List[Dict[str, Any]], Dict[str, Any]]:
    """
    Predicts CPU burst times for a batch of processes.
    Returns:
    - enriched processes with 'predicted_burst_time'
    - telemetry stats (inference latency, MAE/MAPE if true bursts provided)
    """
    if not processes:
        return [], {"error": "Empty process list"}
        
    model = get_model(model_name)
    
    imputed_procs = [impute_missing_features(p) for p in processes]
    df = pd.DataFrame(imputed_procs)
    
    X = df[FEATURE_COLUMNS].copy()
    
    # Profile inference latency
    t0 = time.perf_counter()
    raw_predictions = model.predict(X)
    total_latency_sec = time.perf_counter() - t0
    
    # Enforce minimum positive execution slice
    predicted_bursts = np.maximum(np.round(raw_predictions, 2), 0.5)
    
    for i, p in enumerate(processes):
        p["predicted_burst_time"] = float(predicted_bursts[i])
        # If estimated_burst_time not present, set exponential smoothing or past avg
        if "estimated_burst_time" not in p or p["estimated_burst_time"] <= 0:
            p["estimated_burst_time"] = float(imputed_procs[i]["past_burst_avg"])
            
    # Calculate empirical prediction error if actual bursts are known
    actuals = [p.get("burst_time", 0.0) for p in processes if "burst_time" in p and p["burst_time"] > 0]
    preds = [p["predicted_burst_time"] for p in processes if "burst_time" in p and p["burst_time"] > 0]
    
    metrics = {
        "model_used": model_name,
        "total_inference_time_ms": round(total_latency_sec * 1000.0, 4),
        "avg_inference_time_us": round((total_latency_sec / len(processes)) * 1_000_000.0, 2),
        "process_count": len(processes)
    }
    
    if len(actuals) == len(processes) and len(actuals) > 0:
        arr_actual = np.array(actuals)
        arr_pred = np.array(preds)
        mae = float(np.mean(np.abs(arr_actual - arr_pred)))
        rmse = float(np.sqrt(np.mean((arr_actual - arr_pred) ** 2)))
        safe_actual = np.where(arr_actual == 0, 1e-6, arr_actual)
        mape = float(np.mean(np.abs((arr_actual - arr_pred) / safe_actual)) * 100.0)
        
        metrics["mae"] = round(mae, 4)
        metrics["rmse"] = round(rmse, 4)
        metrics["mape_percent"] = round(mape, 2)
        
    return processes, metrics

if __name__ == "__main__":
    sample_procs = [
        {"pid": "P1", "arrival_time": 0.0, "burst_time": 10.0, "workload_type": "cpu_bound"},
        {"pid": "P2", "arrival_time": 1.0, "burst_time": 3.0, "workload_type": "io_bound"}
    ]
    enriched, meta = predict_burst_times(sample_procs, model_name="decision_tree")
    print("Enriched processes:", enriched)
    print("Inference metrics:", meta)
