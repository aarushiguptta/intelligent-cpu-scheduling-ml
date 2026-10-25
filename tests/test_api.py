"""
Integration and API Endpoint Tests for the Flask Backend.
"""

import pytest
import os
import sys
import json

BACKEND_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "backend")
if BACKEND_DIR not in sys.path:
    sys.path.append(BACKEND_DIR)

from app import create_app

@pytest.fixture
def client():
    app = create_app()
    app.config["TESTING"] = True
    with app.test_client() as client:
        yield client

def test_health_endpoint(client):
    res = client.get("/api/health")
    assert res.status_code == 200
    data = res.get_json()
    assert data["status"] == "healthy"
    assert "cpp_engine" in data["components"]

def test_workload_types_endpoint(client):
    res = client.get("/api/workload/types")
    assert res.status_code == 200
    data = res.get_json()
    assert "workload_types" in data
    assert "cpu_bound" in data["workload_types"]

def test_workload_generate_endpoint(client):
    res = client.get("/api/workload/generate?type=io_bound&count=5&seed=42")
    assert res.status_code == 200
    data = res.get_json()
    assert data["count"] == 5
    assert len(data["processes"]) == 5

def test_models_endpoint(client):
    res = client.get("/api/models")
    assert res.status_code == 200
    data = res.get_json()
    assert "available_models" in data
    assert "decision_tree" in data["available_models"]

def test_predict_endpoint(client):
    payload = {
        "processes": [
            {"pid": "P1", "arrival_time": 0.0, "burst_time": 6.0, "workload_type": "mixed"}
        ],
        "model_name": "decision_tree"
    }
    res = client.post("/api/predict", data=json.dumps(payload), content_type="application/json")
    assert res.status_code == 200
    data = res.get_json()
    assert len(data["processes"]) == 1
    assert "predicted_burst_time" in data["processes"][0]

def test_simulate_endpoint(client):
    payload = {
        "algorithm": "ALL",
        "time_quantum": 2.0,
        "context_switch_overhead": 0.1,
        "aging_factor": 0.2,
        "fallback_quantum": 2.0,
        "model_name": "decision_tree",
        "processes": [
            {"pid": "P1", "arrival_time": 0.0, "burst_time": 6.0},
            {"pid": "P2", "arrival_time": 1.0, "burst_time": 3.0}
        ]
    }
    res = client.post("/api/simulate", data=json.dumps(payload), content_type="application/json")
    assert res.status_code == 200
    data = res.get_json()
    assert data["status"] == "success"
    assert "ml_telemetry" in data
    assert "results" in data["simulation"]
    assert len(data["simulation"]["results"]) == 6

def test_simulate_empty_error(client):
    res = client.post("/api/simulate", data=json.dumps({"processes": []}), content_type="application/json")
    assert res.status_code == 400
