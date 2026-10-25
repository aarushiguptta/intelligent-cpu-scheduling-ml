"""
Training and Evaluation Pipeline for CPU Burst Prediction Models.
Trains Random Forest, Decision Tree, and Gradient Boosting Regressors.
Profiles prediction error and inference overhead separately.
"""

import os
import time
import json
import joblib
import numpy as np
import pandas as pd
from typing import Dict, Any

from sklearn.ensemble import RandomForestRegressor, GradientBoostingRegressor
from sklearn.tree import DecisionTreeRegressor
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score

from dataset_generator import generate_multiworkload_dataset
from feature_pipeline import extract_features_and_target, chronological_split, FEATURE_COLUMNS

MODEL_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "models")
os.makedirs(MODEL_DIR, exist_ok=True)

def calculate_mape(y_true: np.ndarray, y_pred: np.ndarray) -> float:
    """Mean Absolute Percentage Error with protection against zero division."""
    y_true_safe = np.where(y_true == 0, 1e-6, y_true)
    return float(np.mean(np.abs((y_true - y_pred) / y_true_safe)) * 100.0)

def profile_inference_latency(model, X_sample: pd.DataFrame, n_iterations: int = 1000) -> Dict[str, float]:
    """Profiles the real CPU prediction overhead in microseconds."""
    single_row = X_sample.iloc[[0]]
    
    # Warmup
    for _ in range(50):
        _ = model.predict(single_row)
        
    start_time = time.perf_counter()
    for _ in range(n_iterations):
        _ = model.predict(single_row)
    total_time = time.perf_counter() - start_time
    
    per_pred_us = (total_time / n_iterations) * 1_000_000.0
    return {
        "per_prediction_microseconds": round(per_pred_us, 2),
        "per_prediction_milliseconds": round(per_pred_us / 1000.0, 4)
    }

def train_and_evaluate_all():
    print("Generating multi-workload synthetic telemetry dataset...")
    df = generate_multiworkload_dataset(n_per_workload=600, seed=42)
    print(f"Total samples: {len(df)}")
    
    train_df, test_df = chronological_split(df, train_ratio=0.80)
    print(f"Chronological split -> Train: {len(train_df)} samples, Test: {len(test_df)} samples")
    
    X_train, y_train = extract_features_and_target(train_df)
    X_test, y_test = extract_features_and_target(test_df)
    
    models = {
        "random_forest": RandomForestRegressor(n_estimators=100, max_depth=12, random_state=42, n_jobs=-1),
        "decision_tree": DecisionTreeRegressor(max_depth=8, random_state=42),
        "gradient_boosting": GradientBoostingRegressor(n_estimators=100, max_depth=4, learning_rate=0.08, random_state=42)
    }
    
    results: Dict[str, Any] = {}
    
    for name, model in models.items():
        print(f"\nTraining {name}...")
        t0 = time.time()
        model.fit(X_train, y_train)
        train_time = round(time.time() - t0, 4)
        
        y_pred = model.predict(X_test)
        
        # Enforce burst time physical bound (> 0.5)
        y_pred = np.maximum(y_pred, 0.5)
        
        mae = float(mean_absolute_error(y_test, y_pred))
        rmse = float(np.sqrt(mean_squared_error(y_test, y_pred)))
        mape = float(calculate_mape(y_test.values, y_pred))
        r2 = float(r2_score(y_test, y_pred))
        
        latency = profile_inference_latency(model, X_test)
        
        # Feature importances
        importances = {}
        if hasattr(model, "feature_importances_"):
            for feat, imp in zip(FEATURE_COLUMNS, model.feature_importances_):
                importances[feat] = round(float(imp), 4)
        
        model_path = os.path.join(MODEL_DIR, f"{name}.joblib")
        joblib.dump(model, model_path)
        print(f"Saved {name} model to {model_path}")
        
        results[name] = {
            "mae": round(mae, 4),
            "rmse": round(rmse, 4),
            "mape_percent": round(mape, 2),
            "r2_score": round(r2, 4),
            "training_time_sec": train_time,
            "inference_overhead": latency,
            "feature_importances": importances
        }
        
        print(f"Results for {name}:")
        print(f"  MAE: {mae:.4f} ms | RMSE: {rmse:.4f} ms | MAPE: {mape:.2f}% | R2: {r2:.4f}")
        print(f"  Inference Overhead: {latency['per_prediction_microseconds']} µs")
        
    summary_path = os.path.join(MODEL_DIR, "metrics_summary.json")
    with open(summary_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nEvaluation summary saved to {summary_path}")
    return results

if __name__ == "__main__":
    train_and_evaluate_all()
