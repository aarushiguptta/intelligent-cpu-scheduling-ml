"""
Feature Pipeline and Data Leakage Prevention for CPU Burst Prediction.
"""

import numpy as np
import pandas as pd
from typing import Tuple, List
from sklearn.preprocessing import StandardScaler

FEATURE_COLUMNS: List[str] = [
    "process_category",
    "memory_mb",
    "priority_level",
    "thread_count",
    "past_burst_avg",
    "past_io_rate",
    "arrival_interval",
    "queue_length_at_arrival"
]

TARGET_COLUMN: str = "burst_time"

def extract_features_and_target(df: pd.DataFrame) -> Tuple[pd.DataFrame, pd.Series]:
    """
    Extracts features matrix X and target y with strict assertion against data leakage.
    Ensures actual future CPU burst duration is never an input feature.
    """
    # Strict anti-leakage verification
    for col in FEATURE_COLUMNS:
        assert col != TARGET_COLUMN, f"Data leakage error: {TARGET_COLUMN} cannot be an input feature!"
        assert col in df.columns, f"Required feature column {col} is missing from dataset!"
        
    X = df[FEATURE_COLUMNS].copy()
    y = df[TARGET_COLUMN].copy()
    return X, y

def chronological_split(
    df: pd.DataFrame,
    train_ratio: float = 0.80
) -> Tuple[pd.DataFrame, pd.DataFrame]:
    """
    Splits the dataset chronologically (based on arrival sequence) to replicate realistic OS telemetry.
    """
    # Ensure ordered by arrival time
    sorted_df = df.sort_values(by="arrival_time").reset_index(drop=True)
    split_idx = int(len(sorted_df) * train_ratio)
    
    train_df = sorted_df.iloc[:split_idx].reset_index(drop=True)
    test_df = sorted_df.iloc[split_idx:].reset_index(drop=True)
    return train_df, test_df
