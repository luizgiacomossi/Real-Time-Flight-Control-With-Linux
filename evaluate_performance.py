import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import glob

def compute_metrics(df):
    """Compute RMSE, Settling Time, and Peak Error from a flight log DataFrame."""
    metrics = {}
    
    # 1. RMSE of yaw tracking
    rmse = np.sqrt(np.mean((df['ref_yaw'] - df['yaw'])**2))
    metrics['rmse_deg'] = np.degrees(rmse)
    
    # 2. Settling Time (Step starts at t=2.0, target=0.5236 rad ~ 30 deg)
    step_start_idx = df[df['time'] >= 2.0].index[0]
    target_yaw = 0.5236
    error_band = 0.02 * target_yaw # 2% error band
    
    # Find the last time the yaw was outside the 2% error band
    df_step = df.iloc[step_start_idx:step_start_idx+750] # look at next 3 seconds
    outside_band = df_step[np.abs(df_step['yaw'] - target_yaw) > error_band]
    
    if len(outside_band) == 0:
        metrics['settling_ms'] = 0.0
    else:
        last_outside_time = outside_band['time'].iloc[-1]
        settling_time_s = last_outside_time - 2.0
        metrics['settling_ms'] = settling_time_s * 1000.0
        
    # 3. Peak Error (Disturbance starts at t=5.0)
    dist_start_idx = df[df['time'] >= 5.0].index[0]
    df_dist = df.iloc[dist_start_idx:dist_start_idx+500] # look at next 2 seconds
    
    # Disturbance is on roll axis, target is 0
    peak_error_rad = np.max(np.abs(df_dist['roll']))
    metrics['peak_error_deg'] = np.degrees(peak_error_rad)
    
    return metrics

def main():
    kernels = ['standard', 'preempt_rt']
    configs = [
        "other_n0", "other_n19", "fifo_p50", "fifo_p99",
        "rr_p50", "rr_p99", "deadline_R400", "deadline_R800"
    ]
    stresses = ['idle', 'stress']
    
    results = []
    
    for kernel in kernels:
        for config in configs:
            for stress in stresses:
                file_name = f"experiments_flight/flight_log_{kernel}_{config}_{stress}.csv"
                if os.path.exists(file_name):
                    df = pd.read_csv(file_name)
                    metrics = compute_metrics(df)
                    results.append({
                        'Kernel': kernel,
                        'Config': config,
                        'Load': stress,
                        'RMSE (deg)': metrics['rmse_deg'],
                        'Settling (ms)': metrics['settling_ms'],
                        'Peak Err (deg)': metrics['peak_error_deg']
                    })
                else:
                    print(f"Warning: Missing file {file_name}")
                    
    if not results:
        print("No flight log files found in experiments_flight/. Please run run_path_a_experiments.sh first.")
        return

    results_df = pd.DataFrame(results)
    
    # Pivot table for LaTeX
    print("\n--- Control Performance Metrics Table (LaTeX) ---")
    latex_table = results_df.to_latex(index=False, float_format="%.2f")
    print(latex_table)
    
    # Generate Figure: Standard Stress vs RT Stress for worst/best case
    worst_file = "experiments_flight/flight_log_standard_other_n0_stress.csv"
    best_file = "experiments_flight/flight_log_preempt_rt_deadline_R800_stress.csv"
    
    if os.path.exists(worst_file) and os.path.exists(best_file):
        df_worst = pd.read_csv(worst_file)
        df_best = pd.read_csv(best_file)
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
        
        # Plot 1: Yaw Step Response
        ax1.plot(df_worst['time'], np.degrees(df_worst['ref_yaw']), 'k--', label='Reference')
        ax1.plot(df_worst['time'], np.degrees(df_worst['yaw']), 'r-', label='Standard Kernel (OTHER nice 0)')
        ax1.plot(df_best['time'], np.degrees(df_best['yaw']), 'g-', label='PREEMPT_RT Kernel (DEADLINE R800)')
        ax1.set_xlim(1.5, 4.0)
        ax1.set_ylabel('Yaw Angle (deg)')
        ax1.set_title('Step Response Under CPU Stress (Settling Time)')
        ax1.legend()
        ax1.grid(True)
        
        # Plot 2: Disturbance Rejection
        ax2.plot(df_worst['time'], np.degrees(df_worst['roll']), 'r-', label='Standard Kernel (OTHER nice 0)')
        ax2.plot(df_best['time'], np.degrees(df_best['roll']), 'g-', label='PREEMPT_RT Kernel (DEADLINE R800)')
        ax2.axvspan(5.0, 5.5, color='gray', alpha=0.2, label='Disturbance Pulse')
        ax2.set_xlim(4.5, 7.0)
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('Roll Angle (deg)')
        ax2.set_title('Disturbance Rejection Under CPU Stress (Peak Error)')
        ax2.legend()
        ax2.grid(True)
        
        plt.tight_layout()
        plt.savefig('control_performance_comparison.pdf')
        print("\nGenerated control_performance_comparison.pdf")
    else:
        print("\nSkipping plot generation: Required files for worst/best case comparison not found.")
        print(f"Looking for: {worst_file} and {best_file}")

if __name__ == '__main__':
    main()
