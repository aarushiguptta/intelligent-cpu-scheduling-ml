"""
Workload generation bridge for frontend API.
Supports 5 workload types and explains simulation modeling of I/O blocking.
"""

import sys
import os
from typing import List, Dict, Any

# Add ML directory to path
ML_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "ml")
if ML_DIR not in sys.path:
    sys.path.append(ML_DIR)

from dataset_generator import generate_workload_processes

WORKLOAD_DESCRIPTIONS = {
    "cpu_bound": {
        "title": "CPU-Bound Workload",
        "description": "Characterized by sustained computational bursts (mean 28ms) with minimal I/O activity. High memory footprints and multiple worker threads. Models scientific simulations, neural network inference, and cryptographic rendering.",
        "io_characteristics": "Rare I/O blocks (I/O rate < 10%). Process holds CPU until time quantum or complete calculation."
    },
    "io_bound": {
        "title": "I/O-Bound Workload",
        "description": "Frequent, rapid CPU bursts (mean 1-5ms) punctuated by frequent I/O waits. Low memory footprint and single-threaded event loops. Models web servers, UI event handlers, and database connection pools.",
        "io_characteristics": "Frequent I/O yielding (I/O rate > 70%). Tasks release CPU quickly to wait on network or disk packets."
    },
    "mixed": {
        "title": "Mixed Workload",
        "description": "A balanced distribution reflecting standard multi-user desktop and general-purpose server environments: 35% interactive tasks, 30% background batch jobs, 25% computational threads, and 10% system daemons.",
        "io_characteristics": "Realistic blend of short interactive bursts and extended batch processing."
    },
    "heavy_tailed": {
        "title": "Heavy-Tailed Workload (Pareto)",
        "description": "Power-law Pareto distribution ($x_m = 2.0, \\alpha = 1.4$). 80% of tasks are ultra-short (1-6ms), while 20% are massive outlier jobs (up to 120ms). Standard in large-scale Unix cluster environments.",
        "io_characteristics": "Extreme variance in execution demands. Highlights the risk of convoy effects and starvation in traditional FCFS and SJF schedulers."
    },
    "bursty": {
        "title": "Bursty Workload (Poisson Spikes)",
        "description": "Simulates non-stationary traffic with sudden surges of process arrivals (clustered inter-arrival times < 0.5s), interspersed with quiet periods. Tests scheduling reactivity under acute queue contention.",
        "io_characteristics": "Sudden arrival spikes create ready queue surges, testing aging and time-slice allocation."
    }
}

SIMULATION_IO_NOTE = (
    "Simulation Modeling Note: In this CPU scheduling simulator, individual processes represent "
    "discrete CPU burst activations (the period between process dispatch and its next I/O blocking call). "
    "I/O-bound tasks are modeled via short CPU burst intervals and rapid yields, while CPU-bound tasks "
    "hold the processor for lengthy computation. Full asynchronous multi-phase CPU-I/O-CPU cycling "
    "is projected onto arrival events when the process returns from I/O wait."
)

def get_workload_sample(
    workload_type: str = "mixed",
    count: int = 8,
    seed: int = 42
) -> List[Dict[str, Any]]:
    """Generates a clean list of process dicts for simulation."""
    if workload_type not in WORKLOAD_DESCRIPTIONS:
        workload_type = "mixed"
        
    df = generate_workload_processes(workload_type=workload_type, n_samples=count, seed=seed)
    
    # Format for scheduler input
    processes = []
    for _, row in df.iterrows():
        processes.append({
            "pid": row["pid"],
            "arrival_time": float(row["arrival_time"]),
            "burst_time": float(row["burst_time"]),
            "workload_type": str(row["workload_type"]),
            "process_category": int(row["process_category"]),
            "memory_mb": float(row["memory_mb"]),
            "priority_level": int(row["priority_level"]),
            "thread_count": int(row["thread_count"]),
            "past_burst_avg": float(row["past_burst_avg"]),
            "past_io_rate": float(row["past_io_rate"]),
            "arrival_interval": float(row["arrival_interval"]),
            "queue_length_at_arrival": int(row["queue_length_at_arrival"])
        })
    return processes
