"""
Subprocess Bridge connecting Python/Flask with the compiled C++ CPU Scheduling Engine.
"""

import os
import subprocess
import json
from typing import Dict, Any, List

# Locate the compiled C++ binary
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CPP_BINARY = os.path.join(PROJECT_ROOT, "cpp", "scheduler_engine")

def ensure_cpp_binary_built():
    """Checks if the C++ binary exists; if not, builds it via make."""
    if not os.path.exists(CPP_BINARY):
        print(f"C++ binary not found at {CPP_BINARY}. Compiling...")
        cpp_dir = os.path.join(PROJECT_ROOT, "cpp")
        res = subprocess.run(["make"], cwd=cpp_dir, capture_output=True, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"Failed to build C++ scheduler engine: {res.stderr}")
        print("C++ scheduler engine built successfully.")

def run_cpp_simulation(
    processes: List[Dict[str, Any]],
    algorithm: str = "ALL",
    time_quantum: float = 2.0,
    context_switch_overhead: float = 0.0,
    aging_factor: float = 0.1,
    fallback_quantum: float = 2.0
) -> Dict[str, Any]:
    """
    Executes the C++ scheduling simulation via JSON IPC.
    """
    ensure_cpp_binary_built()
    
    config = {
        "algorithm": algorithm,
        "time_quantum": float(time_quantum),
        "context_switch_overhead": float(context_switch_overhead),
        "aging_factor": float(aging_factor),
        "fallback_quantum": float(fallback_quantum),
        "processes": processes
    }
    
    json_payload = json.dumps(config)
    
    cmd = [CPP_BINARY]
    if algorithm == "ALL":
        cmd.append("--all")
    else:
        cmd.extend(["--algo", algorithm])
        
    proc = subprocess.run(
        cmd,
        input=json_payload,
        capture_output=True,
        text=True,
        timeout=15
    )
    
    if proc.returncode != 0:
        raise RuntimeError(f"C++ scheduler failed with code {proc.returncode}: {proc.stderr}")
        
    try:
        output_data = json.loads(proc.stdout)
        return output_data
    except json.JSONDecodeError as e:
        raise RuntimeError(f"Failed to decode C++ JSON output: {e}\nRaw output: {proc.stdout}")
