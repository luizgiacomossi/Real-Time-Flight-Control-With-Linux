import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import subprocess
import glob
from plot_test_dynamics import plot_waypoints

def compute_metrics(df):
    """Compute 3D RMSE and Peak Error from a 4-waypoint flight log DataFrame."""
    metrics = {}
    
    # Calculate Euclidean error distance at every timestep
    ex = df['ref_x'] - df['x']
    ey = df['ref_y'] - df['y']
    ez = df['ref_z'] - df['z']
    err_sq = ex**2 + ey**2 + ez**2
    err_3d = np.sqrt(err_sq)
    
    # 1. RMSE of 3D trajectory tracking
    rmse = np.sqrt(np.mean(err_sq))
    metrics['rmse_3d_m'] = rmse
    
    # 2. Peak Error (Maximum distance from reference)
    # Note: For step-based waypoints, peak error occurs right after target switch.
    metrics['peak_err_m'] = np.max(err_3d)
    
    return metrics

def load_waypoints(filepath):
    waypoints = []
    if os.path.exists(filepath):
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split()
                if len(parts) >= 3:
                    waypoints.append((float(parts[0]), float(parts[1]), float(parts[2])))
    return waypoints

def generate_comparative_plot(worst_file, best_file, out_dir):
    """Generate a single figure with 2 subplots (Position and Attitude) comparing Worst vs Best."""
    if not os.path.exists(worst_file) or not os.path.exists(best_file):
        print("  [SKIP] Missing worst/best files for comparative plot.")
        return
        
    df_worst = pd.read_csv(worst_file)
    df_best = pd.read_csv(best_file)
    
    fig, (ax_pos, ax_att) = plt.subplots(2, 1, figsize=(10, 8))
    
    time_worst = df_worst['time']
    time_best = df_best['time']
    
    # Colors
    colors = {'x': '#e74c3c', 'y': '#2ecc71', 'z': '#3498db',
              'roll': '#e67e22', 'pitch': '#9b59b6', 'yaw': '#1abc9c'}
              
    POS_MIN, POS_MAX = -2, 6
    #ATT_MIN, ATT_MAX = -np.pi/6, np.pi/6
    ATT_MIN, ATT_MAX = -0.4, 0.36

    def clip_p(s): return np.clip(s, POS_MIN, POS_MAX)
    def clip_a(s): return s
              
    # --- SUBPLOT 1: Position (X, Y, Z) ---
    # Plot References
    ax_pos.plot(time_best, clip_p(df_best['ref_x']), color=colors['x'], linestyle='--', linewidth=2, label='Ref X')
    ax_pos.plot(time_best, clip_p(df_best['ref_y']), color=colors['y'], linestyle='--', linewidth=2, label='Ref Y')
    ax_pos.plot(time_best, clip_p(df_best['ref_z']), color=colors['z'], linestyle='--', linewidth=2, label='Ref Z')
    
    # Plot Worst Case
    ax_pos.plot(time_worst, clip_p(df_worst['x']), color=colors['x'], linestyle='-', alpha=0.7, linewidth=1.5, label='Worst X')
    ax_pos.plot(time_worst, clip_p(df_worst['y']), color=colors['y'], linestyle='-', alpha=0.7, linewidth=3.5, label='Worst Y')
    ax_pos.plot(time_worst, clip_p(df_worst['z']), color=colors['z'], linestyle='-', alpha=0.7, linewidth=1.5, label='Worst Z')
    
    # Plot Best Case
    ax_pos.plot(time_best, clip_p(df_best['x']), color=colors['x'], linestyle='-', linewidth=2.0, label='Best X')
    ax_pos.plot(time_best, clip_p(df_best['y']), color=colors['y'], linestyle='-', linewidth=2.0, label='Best Y')
    ax_pos.plot(time_best, clip_p(df_best['z']), color=colors['z'], linestyle='-', linewidth=2.0, label='Best Z')
    
    ax_pos.set_title("Position Tracking (Best vs Worst Case under Stress)")
    ax_pos.set_ylabel("Position (m)")
    ax_pos.set_ylim(POS_MIN, POS_MAX)  # Prevent exploded trajectories from ruining the scale
    ax_pos.grid(True, alpha=0.3)
    # legend location must be on top right of the plot, inside to save space
    ax_pos.legend(loc='upper center', bbox_to_anchor=(0.6, 0.95), fontsize=10, ncol=2)
    
    # --- SUBPLOT 2: Attitude (Roll, Pitch, Yaw) ---
    # Reference for Roll, Pitch, and Yaw are all 0 in the waypoint test
    ax_att.axhline(0, color='gray', linestyle='--', linewidth=2, label='Ref Roll/Pitch/Yaw (0)')
    
    # Plot Worst Case
    ax_att.plot(time_worst, clip_a(df_worst['roll']), color=colors['roll'], linestyle='-', alpha=0.5, linewidth=.5, label='Worst Roll')
    ax_att.plot(time_worst, clip_a(df_worst['pitch']), color=colors['pitch'], linestyle='-', alpha=0.5, linewidth=.5, label='Worst Pitch')
    ax_att.plot(time_worst, clip_a(df_worst['yaw']), color=colors['yaw'], linestyle='-', alpha=0.5, linewidth=.5, label='Worst Yaw')
    
    # Plot Best Case
    ax_att.plot(time_best, clip_a(df_best['roll']), color=colors['roll'], linestyle='-', linewidth=2.0, label='Best Roll')
    ax_att.plot(time_best, clip_a(df_best['pitch']), color=colors['pitch'], linestyle='-', linewidth=2.0, label='Best Pitch')
    ax_att.plot(time_best, clip_a(df_best['yaw']), color=colors['yaw'], linestyle='-', linewidth=2.0, label='Best Yaw')
    
    ax_att.set_title("Attitude Tracking (Best vs Worst Case under Stress)")
    ax_att.set_xlabel("Time (s)")
    ax_att.set_ylabel("Angle (rad)")
    ax_att.set_ylim(ATT_MIN, ATT_MAX) # Prevent exploded angles from ruining the scale
    ax_att.grid(True, alpha=0.3)
    # legend location must be on top right of the plot, inside to save space
    ax_att.legend(loc='upper center', bbox_to_anchor=(0.6, 0.95), fontsize=10, ncol=2)
    
    plt.tight_layout()
    out_path = os.path.join(out_dir, "comparative_best_vs_worst.pdf")
    plt.savefig(out_path, bbox_inches='tight')
    plt.close()
    print(f"Generated comparative plot: {out_path}")


def generate_3d_comparison_plot(worst_file, best_file, waypoints, out_dir):
    """Generate a single 3D figure overlaying the best and worst case trajectories."""
    if not os.path.exists(worst_file) or not os.path.exists(best_file):
        print("  [SKIP] Missing worst/best files for 3D comparison plot.")
        return

    df_worst = pd.read_csv(worst_file)
    df_best  = pd.read_csv(best_file)

    fig = plt.figure(figsize=(9, 7))
    ax  = fig.add_subplot(111, projection='3d')

    # Clip spatial data to a readable range
    LIM = 20.0
    def c(s): return np.clip(s, -LIM, LIM)

    # Plot ideal reference path through waypoints
    if waypoints:
        wps = np.array(waypoints)
        ax.plot(wps[:, 0], wps[:, 1], wps[:, 2],
                'k--', linewidth=1.5, alpha=0.6, label='Ideal path')
        ax.scatter(wps[:, 0], wps[:, 1], wps[:, 2],
                   color=['#e74c3c', '#e67e22', '#2ecc71', '#9b59b6'],
                   s=300, marker='*', alpha=0.9, zorder=5)   #marker='o' for circles, 's' for squares, '*' for stars, s is size of markers zorder
        for k, (wx, wy, wz) in enumerate(waypoints):
            ax.text(wx + 0.2, wy, wz + 0.4, f'WP{k+1}', fontsize=10)

    # plot also starting point
    ax.scatter(df_worst['x'].iloc[0], df_worst['y'].iloc[0], df_worst['z'].iloc[0],
               color='black', s=50, marker='o', alpha=0.9, zorder=5, label='Start Point')
  
    # add an X to worst case to illustrate a crash when it goes to Z < 0
    # plot it even if the point is outside the limits
    ax.scatter(df_worst['x'].iloc[-1], df_worst['y'].iloc[-1], df_worst['z'].iloc[-1],
               color='red', s=20, marker='x', alpha=0.5, zorder=5, label='Crash Point')

    # Worst case trajectory (faded) but dont plot points outside the limits
    # plot only points with z values between 0 and 5
    mask = (df_worst['z'] >= 0) & (df_worst['z'] <= 5)
    ax.plot(c(df_worst['x'][mask]), c(df_worst['y'][mask]), c(df_worst['z'][mask]),
            color='#e74c3c', alpha=0.99, linewidth=1.5, label='Worst Case (OTHER_n0, stress)')

    # Best case trajectory (prominent)
    ax.plot(c(df_best['x']), c(df_best['y']), c(df_best['z']),
            color='#2ecc71', linewidth=2.0, label='Best Case (FIFO_p99, stress)')

    ax.set_xlabel('X (m)'); ax.set_ylabel('Y (m)'); ax.set_zlabel('Z (m)')
    ax.set_xlim(0, 3.5); ax.set_ylim(-0.5, 3.2); ax.set_zlim(0, 5)
    ax.set_title('3D Trajectory Comparison: Best vs Worst Case under Stress')
    ax.legend(fontsize=12, loc='upper left')

    # ensure that the z axis label is present in the final figure save
    ax.zaxis.label.set_visible(True)
    ax.set_zlabel('Z (m)', rotation=0)
    ax.set_zlabel('Z (m)', fontsize=12)
    # and that it is not cut off
    ax.set_zlabel('Z (m)', fontsize=12, labelpad=20)



    out_path = os.path.join(out_dir, "comparative_3d_trajectory.pdf")
    plt.savefig(out_path, bbox_inches='tight')
    plt.close()
    print(f"Generated 3D trajectory comparison: {out_path}")

def generate_3d_comparison_plot_subfigures(worst_file, best_file, waypoints, out_dir):
    """Generate one figure with 2 subfigures: left=worst case, right=best case."""
    if not os.path.exists(worst_file) or not os.path.exists(best_file):
        print("  [SKIP] Missing worst/best files for 3D comparison plot.")
        return

    df_worst = pd.read_csv(worst_file)
    df_best  = pd.read_csv(best_file)

    fig = plt.figure(figsize=(14, 6))
    ax1 = fig.add_subplot(121, projection='3d')
    ax2 = fig.add_subplot(122, projection='3d')

    LIM = 20.0
    def c(s): return np.clip(s, -LIM, LIM)

    WP_COLORS = ['#e74c3c', '#e67e22', '#2ecc71', '#9b59b6']

    def _decorate_ax(ax, df, traj_color, traj_label, alpha=1.0):
        """Draw waypoints, start marker, and trajectory on a single 3D axis."""
        if waypoints:
            wps = np.array(waypoints)
            ax.plot(wps[:, 0], wps[:, 1], wps[:, 2],
                    'k--', linewidth=1.5, alpha=0.6, label='Ideal path')
            ax.scatter(wps[:, 0], wps[:, 1], wps[:, 2],
                       color=WP_COLORS, s=300, marker='*', alpha=0.9, zorder=5)
            for k, (wx, wy, wz) in enumerate(waypoints):
                ax.text(wx + 0.2, wy, wz + 0.4, f'WP{k+1}', fontsize=10)

        ax.scatter(df['x'].iloc[0], df['y'].iloc[0], df['z'].iloc[0],
                   color='black', s=60, marker='o', alpha=0.9, zorder=6, label='Start')

        ax.plot(c(df['x']), c(df['y']), c(df['z']),
                color=traj_color, alpha=alpha, linewidth=1.8, label=traj_label)

        ax.set_xlabel('X (m)', fontsize=11, labelpad=8)
        ax.set_ylabel('Y (m)', fontsize=11, labelpad=8)
        ax.set_zlabel('Z (m)', fontsize=11, labelpad=12)
        ax.set_xlim(0, 3.5)
        ax.set_ylim(-0.5, 3.2)
        ax.set_zlim(-15, 5)
        ax.legend(fontsize=9, loc='upper left')

    _decorate_ax(ax1, df_worst, '#e74c3c', 'Worst Case (Standard kernel)', alpha=0.7)
    ax1.set_title('Worst Case under Stress', fontsize=12, pad=10)

    _decorate_ax(ax2, df_best, '#2ecc71', 'Best Case (PREEMPT_RT)', alpha=1.0)
    ax2.set_title('Best Case under Stress', fontsize=12, pad=10)

    plt.suptitle('3D Trajectory Comparison: Best vs Worst Case', fontsize=13, y=1.01)
    plt.tight_layout()

    out_path = os.path.join(out_dir, "comparative_3d_trajectory_subplots.pdf")
    plt.savefig(out_path, bbox_inches='tight')
    plt.close()
    print(f"Generated 3D trajectory subplots: {out_path}")


def main():
    kernels = ['standard', 'preempt_rt']
    configs = [
        "other_n0", "other_n19", "fifo_p50", "fifo_p99",
        "rr_p50", "rr_p99", "deadline_R400", "deadline_R800"
    ]
    stresses = ['idle', 'stress']
    
    results = []
    
    out_plots_dir = "experiments_flight/plots"
    os.makedirs(out_plots_dir, exist_ok=True)
    
    waypoints = load_waypoints("waypoints_tests.txt")
    if not waypoints:
        print("Warning: waypoints_tests.txt not found or empty. Using defaults.")
        waypoints = [(0.0, 0.0, 3.0), (3.0, 0.0, 3.0), (3.0, 3.0, 5.0), (0.0, 0.0, 1.0)]

    print("--- Evaluating Flight Logs ---")
    for kernel in kernels:
        for config in configs:
            for stress in stresses:
                stem = f"flight_log_{kernel}_{config}_{stress}"
                file_name = f"experiments_flight/{stem}.csv"
                
                if os.path.exists(file_name):
                    print(f"Processing: {stem}")
                    df = pd.read_csv(file_name)
                    metrics = compute_metrics(df)
                    results.append({
                        'Kernel': kernel,
                        'Config': config,
                        'Load': stress,
                        'RMSE 3D (m)': metrics['rmse_3d_m'],
                        'Peak Err (m)': metrics['peak_err_m']
                    })
                    
                    # Generate the SVG plots for this scenario
                    title = f"Kernel: {kernel} | Policy: {config} | Load: {stress}"
                    plot_waypoints(file_name, stem, out_dir=out_plots_dir, title=title, waypoints=waypoints)
                else:
                    pass # File not found (could be partially run)
                    
    if not results:
        print("No flight log files found in experiments_flight/. Please run run_path_a_experiments.sh first.")
        return

    results_df = pd.DataFrame(results)
    
    # Pivot table for LaTeX
    print("\n--- Tracking Performance Metrics Table (LaTeX) ---")
    latex_table = results_df.to_latex(index=False, float_format="%.3f")
    print(latex_table)
    
    print(f"\nAll static SVG plots generated in: {out_plots_dir}/")
    
    # Generate 3D Animations (GIFs) for Best vs Worst Cases under Stress
    worst_file = "experiments_flight/flight_log_preempt_rt_other_n0_stress.csv"
    best_file = "experiments_flight/flight_log_preempt_rt_fifo_p99_stress.csv"
    
    worst_gif = os.path.join(out_plots_dir, "anim_worst_standard_other_n0_stress.gif")
    best_gif = os.path.join(out_plots_dir, "anim_best_preempt_rt_fifo_p99_stress.gif")
    
    print("\n--- Generating Static Comparative Plots ---")
    generate_comparative_plot(worst_file, best_file, out_plots_dir)
    generate_3d_comparison_plot(worst_file, best_file, waypoints, out_plots_dir)
    generate_3d_comparison_plot_subfigures(worst_file, best_file, waypoints, out_plots_dir)
    
    print("\n--- Generating 3D Animations for Worst vs Best Cases ---")
    if os.path.exists(worst_file):
        print(f"Animating Worst Case (Standard, OTHER, Stress)... This may take a few minutes.")
        subprocess.run([
            "python", "animate_drone.py", 
            "--csv", worst_file, 
            "--gif", worst_gif, 
            "--title", "Worst Case: Standard Kernel (OTHER n0) under Stress",
            "--waypoints", "waypoints_tests.txt"
        ])
    else:
        print(f"Missing {worst_file} for animation.")
        
    if os.path.exists(best_file):
        print(f"Animating Best Case (PREEMPT_RT, FIFO, Stress)... This may take a few minutes.")
        subprocess.run([
            "python", "animate_drone.py", 
            "--csv", best_file, 
            "--gif", best_gif, 
            "--title", "Best Case: PREEMPT_RT (FIFO p99) under Stress",
            "--waypoints", "waypoints_tests.txt"
        ])
    else:
        print(f"Missing {best_file} for animation.")

if __name__ == '__main__':
    main()
