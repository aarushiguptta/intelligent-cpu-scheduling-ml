"""
Flask API Blueprint and Route Handlers for the CPU Scheduling Platform.
"""

import os
import json
from flask import Blueprint, request, jsonify, send_from_directory, current_app

from cpp_bridge import run_cpp_simulation
from workload_generator import (
    get_workload_sample,
    WORKLOAD_DESCRIPTIONS,
    SIMULATION_IO_NOTE
)

import sys
ML_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "ml")
if ML_DIR not in sys.path:
    sys.path.append(ML_DIR)

from predict import predict_burst_times

api_bp = Blueprint("api", __name__, url_prefix="/api")

@api_bp.route("/health", methods=["GET"])
def health():
    return jsonify({
        "status": "healthy",
        "service": "Intelligent CPU Scheduling Simulator",
        "components": {
            "cpp_engine": "operational",
            "ml_subsystem": "operational"
        }
    })

@api_bp.route("/workload/types", methods=["GET"])
def workload_types():
    return jsonify({
        "workload_types": WORKLOAD_DESCRIPTIONS,
        "io_modeling_note": SIMULATION_IO_NOTE
    })

@api_bp.route("/workload/generate", methods=["GET"])
def generate_workload():
    w_type = request.args.get("type", "mixed")
    count = int(request.args.get("count", 8))
    seed = int(request.args.get("seed", 42))
    
    # Enforce safe bounds
    count = max(2, min(100, count))
    processes = get_workload_sample(workload_type=w_type, count=count, seed=seed)
    
    return jsonify({
        "workload_type": w_type,
        "count": len(processes),
        "processes": processes,
        "description": WORKLOAD_DESCRIPTIONS.get(w_type, {})
    })

@api_bp.route("/models", methods=["GET"])
def get_models_info():
    metrics_path = os.path.join(ML_DIR, "models", "metrics_summary.json")
    if os.path.exists(metrics_path):
        with open(metrics_path, "r") as f:
            summary = json.load(f)
    else:
        summary = {"error": "Models not yet trained"}
        
    return jsonify({
        "available_models": ["decision_tree", "random_forest", "gradient_boosting"],
        "default_model": "decision_tree",
        "evaluation_metrics": summary
    })

@api_bp.route("/predict", methods=["POST"])
def predict():
    data = request.get_json(force=True) or {}
    processes = data.get("processes", [])
    model_name = data.get("model_name", "decision_tree")
    
    if not processes:
        return jsonify({"error": "No processes provided"}), 400
        
    enriched, meta = predict_burst_times(processes, model_name=model_name)
    return jsonify({
        "processes": enriched,
        "ml_metadata": meta
    })

@api_bp.route("/simulate", methods=["POST"])
def simulate():
    data = request.get_json(force=True) or {}
    processes = data.get("processes", [])
    
    if not processes:
        return jsonify({"error": "Process list is required"}), 400
        
    algorithm = data.get("algorithm", "ALL")
    time_quantum = float(data.get("time_quantum", 2.0))
    context_switch_overhead = float(data.get("context_switch_overhead", 0.0))
    aging_factor = float(data.get("aging_factor", 0.1))
    fallback_quantum = float(data.get("fallback_quantum", 2.0))
    model_name = data.get("model_name", "decision_tree")
    
    # Step 1: Run ML Inference to compute predicted_burst_time and profile overhead
    enriched_procs, ml_meta = predict_burst_times(processes, model_name=model_name)
    
    # Step 2: Run C++ scheduling core
    try:
        sim_output = run_cpp_simulation(
            processes=enriched_procs,
            algorithm=algorithm,
            time_quantum=time_quantum,
            context_switch_overhead=context_switch_overhead,
            aging_factor=aging_factor,
            fallback_quantum=fallback_quantum
        )
    except Exception as e:
        return jsonify({"error": str(e)}), 500
        
    # Return composite response
    response = {
        "status": "success",
        "configuration": {
            "algorithm": algorithm,
            "time_quantum": time_quantum,
            "context_switch_overhead": context_switch_overhead,
            "aging_factor": aging_factor,
            "fallback_quantum": fallback_quantum,
            "model_name": model_name
        },
        "ml_telemetry": ml_meta,
        "simulation": sim_output
    }
    return jsonify(response)
