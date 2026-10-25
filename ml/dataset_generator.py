"""
Synthetic Workload and Process Telemetry Dataset Generator.

Generates realistic process metadata and pre-scheduling telemetry without data leakage.
Supports 5 distinct workload types:
1. CPU-Bound
2. I/O-Bound
3. Mixed
4. Heavy-Tailed (Pareto)
5. Bursty (Clustered Poisson Arrivals)
"""

import numpy as np
import pandas as pd
from typing import Tuple, List, Dict, Any

RANDOM_SEED = 42

WORKLOAD_CATEGORIES = {
    "interactive": 0,
    "batch": 1,
    "computational": 2,
    "system": 3
}

def generate_workload_processes(
    workload_type: str = "mixed",
    n_samples: int = 1000,
    seed: int = RANDOM_SEED
) -> pd.DataFrame:
    """
    Generates a synthetic dataset of processes representing a specific workload profile.
    
    Features available at scheduling time (NO LEAKAGE):
    - process_category: integer code (0=interactive, 1=batch, 2=computational, 3=system)
    - memory_mb: requested memory footprint in MB
    - priority_level: process nice level (-20 to 19 mapped to 0-10)
    - thread_count: process thread count
    - past_burst_avg: historical average of past CPU bursts for this executable
    - past_io_rate: historical frequency of I/O wait operations
    - arrival_interval: inter-arrival gap
    - queue_length_at_arrival: instant load queue depth
    
    Target:
    - burst_time: actual future CPU burst duration
    """
    np.random.seed(seed)
    
    records: List[Dict[str, Any]] = []
    current_time = 0.0
    active_queue = 0
    
    for i in range(n_samples):
        pid = f"P{i+1}"
        
        if workload_type == "cpu_bound":
            # High computational requirements, long bursts, high memory
            cat = np.random.choice([1, 2], p=[0.2, 0.8])
            base_burst = np.random.normal(loc=28.0, scale=8.0)
            base_burst = max(8.0, base_burst)
            mem = np.random.uniform(512.0, 4096.0)
            threads = np.random.choice([4, 8, 16, 32], p=[0.3, 0.4, 0.2, 0.1])
            io_rate = np.random.uniform(0.01, 0.15)
            inter_arrival = np.random.exponential(scale=6.0)
            
        elif workload_type == "io_bound":
            # Frequent I/O, short CPU bursts before yielding, low memory
            cat = np.random.choice([0, 3], p=[0.7, 0.3])
            base_burst = np.random.exponential(scale=3.0)
            base_burst = max(1.0, min(8.0, base_burst))
            mem = np.random.uniform(64.0, 512.0)
            threads = np.random.choice([1, 2, 4], p=[0.5, 0.3, 0.2])
            io_rate = np.random.uniform(0.60, 0.95)
            inter_arrival = np.random.exponential(scale=1.5)
            
        elif workload_type == "heavy_tailed":
            # 80% short bursts, 20% massive bursts (Pareto distribution)
            is_heavy = np.random.random() < 0.20
            if is_heavy:
                cat = 2 # computational
                base_burst = (np.random.pareto(a=1.4) + 1.0) * 15.0
                base_burst = min(120.0, base_burst)
                mem = np.random.uniform(1024.0, 8192.0)
                threads = np.random.choice([8, 16, 32])
                io_rate = np.random.uniform(0.05, 0.25)
            else:
                cat = np.random.choice([0, 1])
                base_burst = np.random.uniform(1.0, 6.0)
                mem = np.random.uniform(128.0, 1024.0)
                threads = np.random.choice([1, 2, 4])
                io_rate = np.random.uniform(0.30, 0.70)
            inter_arrival = np.random.exponential(scale=3.0)
            
        elif workload_type == "bursty":
            # Clustered arrivals with high variance bursts
            cat = np.random.choice([0, 1, 2, 3])
            # Bursty inter-arrival: 70% clustered arrivals (< 0.5s), 30% quiet intervals
            if np.random.random() < 0.70:
                inter_arrival = np.random.uniform(0.05, 0.5)
            else:
                inter_arrival = np.random.exponential(scale=8.0)
            base_burst = np.random.gamma(shape=2.5, scale=4.0)
            base_burst = max(1.0, min(40.0, base_burst))
            mem = np.random.uniform(128.0, 2048.0)
            threads = np.random.choice([1, 2, 4, 8])
            io_rate = np.random.uniform(0.1, 0.6)
            
        else: # "mixed" (default)
            cat = np.random.choice([0, 1, 2, 3], p=[0.35, 0.30, 0.25, 0.10])
            if cat == 0: # interactive
                base_burst = np.random.uniform(1.5, 6.0)
                mem = np.random.uniform(128.0, 1024.0)
                io_rate = np.random.uniform(0.5, 0.85)
                threads = np.random.choice([1, 2, 4])
            elif cat == 1: # batch
                base_burst = np.random.uniform(8.0, 22.0)
                mem = np.random.uniform(512.0, 2048.0)
                io_rate = np.random.uniform(0.15, 0.4)
                threads = np.random.choice([2, 4, 8])
            elif cat == 2: # computational
                base_burst = np.random.uniform(20.0, 45.0)
                mem = np.random.uniform(1024.0, 4096.0)
                io_rate = np.random.uniform(0.02, 0.2)
                threads = np.random.choice([4, 8, 16])
            else: # system
                base_burst = np.random.uniform(0.5, 4.0)
                mem = np.random.uniform(64.0, 256.0)
                io_rate = np.random.uniform(0.7, 0.95)
                threads = 1
            inter_arrival = np.random.exponential(scale=3.5)
            
        current_time += round(inter_arrival, 2)
        arrival_time = round(current_time, 2)
        
        # Priority nice level (0 to 10)
        priority = int(np.random.choice(range(11)))
        
        # Pre-execution telemetry: historical past burst average (noisy reflection of base burst)
        # Note: Noise represents realistic variability in past program invocations
        history_noise = np.random.normal(loc=1.0, scale=0.18)
        past_burst_avg = max(1.0, round(base_burst * history_noise, 2))
        
        # True burst time with slight execution runtime variance
        execution_variance = np.random.normal(loc=0.0, scale=0.10 * base_burst)
        actual_burst = max(1.0, round(base_burst + execution_variance, 2))
        
        # Simple queue depth tracking
        if inter_arrival < 2.0:
            active_queue = min(15, active_queue + 1)
        else:
            active_queue = max(0, active_queue - 1)
            
        records.append({
            "pid": pid,
            "arrival_time": arrival_time,
            "burst_time": actual_burst,
            "workload_type": workload_type,
            "process_category": int(cat),
            "memory_mb": round(mem, 1),
            "priority_level": priority,
            "thread_count": int(threads),
            "past_burst_avg": past_burst_avg,
            "past_io_rate": round(io_rate, 3),
            "arrival_interval": round(inter_arrival, 2),
            "queue_length_at_arrival": int(active_queue)
        })
        
    df = pd.DataFrame(records)
    return df

def generate_multiworkload_dataset(
    n_per_workload: int = 1000,
    seed: int = RANDOM_SEED
) -> pd.DataFrame:
    """Generates a composite chronologically sorted dataset across all 5 workloads."""
    frames = []
    workloads = ["cpu_bound", "io_bound", "mixed", "heavy_tailed", "bursty"]
    for idx, w in enumerate(workloads):
        df_w = generate_workload_processes(workload_type=w, n_samples=n_per_workload, seed=seed + idx * 7)
        frames.append(df_w)
    
    combined = pd.concat(frames, ignore_index=True)
    combined = combined.sort_values(by="arrival_time").reset_index(drop=True)
    return combined

if __name__ == "__main__":
    df = generate_multiworkload_dataset(n_per_workload=300)
    print(f"Generated multi-workload dataset with {len(df)} samples.")
    print("Workload distribution:\n", df["workload_type"].value_counts())
    print("\nSample features:\n", df.head(3))
