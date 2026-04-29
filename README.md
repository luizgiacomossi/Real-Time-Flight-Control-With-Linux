# Real-Time Quadcopter Flight Control With Linux

![4-Waypoint Sequence](animations/anim_waypoints.gif)

---



## Overview

This repository implements a **real-time quadrotor flight control system** for Linux, combined with a **Software-in-the-Loop (SITL) simulation framework** for rigorous performance evaluation. The primary goal is to measure the direct impact of Linux scheduling policies and kernel preemption on closed-loop flight control stability — a methodology used to support academic research into real-time operating systems.

The system combines:
- A **6-DOF physics plant** (Bouabdallah dynamics) running inside the real-time loop
- A **cascaded PD flight controller** (position → attitude → motor mixing)
- A **real-time scheduling harness** for configuring `SCHED_FIFO`, `SCHED_RR`, `SCHED_DEADLINE`, and `SCHED_OTHER`
- A **Python analysis and visualization pipeline** for publication-ready results

---

## System Architecture

### Data Structures (`fcs_drone.h`)

| Structure | Purpose |
|-----------|---------|
| `Vector3` | 3D vectors for position, velocity, force |
| `Attitude` | Roll, pitch, yaw angles (radians) |
| `AngularVelocity` | Body angular rates p, q, r (rad/s) |
| `MotorCommands` | Individual motor speed commands (rad/s) |
| `ControllerGains` | PD gains for position, attitude, and yaw controllers |
| `QuadcopterState` | Full state: position, velocity, attitude, angular velocity, motors |
| `Reference` | Target position (x, y, z) and yaw setpoint |
| `ControlOutputs` | Intermediate and final control signals |

### Control Flow

The controller executes the following cascade every loop iteration:

```
Wake up → Measure dt (actual OS delay) → Simulate physics (OLD motor speeds, dt) →
→ Update setpoint → Position PD → Allocator 1 (force → attitude) →
→ Attitude PD → Allocator 2 (torque → motors) → Sleep
```

> **SITL Key Property:** Physics integration uses the *actual measured `dt`* from `clock_gettime`, not a fixed 4ms step. Any scheduling delay causes the drone to drift in simulation exactly as it would in hardware.

[Control Architecture Diagram](https://github.com/user-attachments/files/19633628/Diagram_control.drawio.5.pdf)
![Control Diagram](https://github.com/user-attachments/assets/2628333f-567b-4c15-91fd-aa5f5ca0aa0a)

---

## Repository File Reference

### Core C Sources

| File | Description |
|------|-------------|
| `fcs_drone.h` | **Complete FCS implementation** — all controller functions as `static inline`. Edit `initializeController()` here to tune gains. |
| `quadrotor_dynamics.h` / `quadrotor_dynamics.c` | 6-DOF Bouabdallah quadrotor plant. Provides `quadrotor_step_euler()` for forward Euler integration. |
| `experiment_fcs_closed_loop.c` | **Main SITL experiment runner.** Configures the Linux scheduler, runs the closed-loop controller + physics for N seconds, and saves a flight log CSV. Accepts waypoints from `waypoints_tests.txt`. |
| `experiment_fcs.c` | Original open-loop latency measurement harness (used for Table III/IV in the paper). |
| `test_dynamics.c` | Unit test suite for the physics plant and controller. Runs 5 test scenarios and outputs CSV files for plotting. |

### Shell Scripts

| Script | Description |
|--------|-------------|
| `run_path_a_experiments.sh <kernel>` | **Main experiment driver.** Compiles `experiment_fcs_closed_loop.c`, then runs 8 scheduling configurations × idle/stress, saving all 16 CSVs to `experiments_flight/`. Run once per kernel. |
| `experiments.sh` | Original latency-only experiment script. |
| `experiments_auto.sh` | Automated latency experiments with report generation. |
| `plot_experiments.sh` | Gnuplot-based latency histogram plotting. |
| `plot_experiments_grouped.sh` | Grouped bar chart plotting for latency results. |
| `plot_experiments_schedulers.sh` | Per-scheduler latency distribution plots. |
| `plot_histograms.sh` | Histogram generation for scheduling jitter data. |
| `generate_report.sh` | Aggregates latency results into a statistical report. |

### Python Scripts

| Script | Description |
|--------|-------------|
| `evaluate_performance.py` | **Main analysis pipeline.** Reads all flight log CSVs from `experiments_flight/`, computes 3D RMSE and Peak Error, outputs a LaTeX table, and generates comparative PDF and GIF figures. |
| `plot_test_dynamics.py` | Generates 5 SVG figures per test scenario (3D trajectory, top-down, position, attitude, overview). Supports waypoint-switch markers. |
| `animate_drone.py` | Generates animated GIFs of drone flight from any CSV log. Supports all 5 standard test scenarios or any custom `--csv` log. Can be called directly or invoked by `evaluate_performance.py`. |

### Configuration Files

| File | Description |
|------|-------------|
| `waypoints_tests.txt` | Waypoint list loaded at runtime by `experiment_fcs_closed_loop.c`. Format: one `X Y Z` per line. Comments with `#` are supported. |
| `fcs_drone.c` | Legacy stub (functions now implemented inline in `fcs_drone.h`). |

---

## Software-in-the-Loop (SITL) Methodology

The SITL loop inside `experiment_fcs_closed_loop.c` operates as follows on each iteration:

```
1. clock_gettime() → measure actual wake-up time
2. Compute dt  = now - prev  (the TRUE elapsed time, including any OS delay)
3. Compute latency_us = now - expected  (scheduling jitter)
4. quadrotor_step_euler(old_outputs, dt)  ← physics FIRST with OLD motor speeds
5. Update active waypoint if within 0.3 m threshold
6. updateFlightControl() → compute NEW motor speeds
7. Log: time, x, y, z, roll, pitch, yaw, ref_x, ref_y, ref_z, F, tau_x, tau_y, tau_z, dt, latency_us
8. clock_nanosleep() → sleep until next period
```

**Why physics before control?** If the OS preempts the thread for 15ms, the drone physically continues to fly for those 15ms with the *previous* motor speeds — it cannot benefit from commands that haven't been computed yet. Running physics with old outputs before computing new ones is the only physically correct model.

---

## Tuning the Controller

All gains are defined in `fcs_drone.h` inside `initializeController()`:

```c
void initializeController(ControllerGains *gains) {
    gains->Kp_pos = 1.0f;   // Position proportional
    gains->Kd_pos = 1.5f;   // Position derivative
    gains->Kp_att = 8.0f;   // Attitude proportional
    gains->Kd_att = 2.5f;   // Attitude derivative
    gains->Kp_yaw = 3.0f;   // Yaw proportional
    gains->Kd_yaw = 1.0f;   // Yaw derivative
}
```

After changing gains, recompile before running experiments.

---

## Running the Tests (macOS / Linux Development)

### 1. Compile and run the unit test suite

```bash
gcc -o test_dynamics test_dynamics.c -lm
./test_dynamics
# Output: test_hover.csv, test_roll.csv, test_climb.csv, test_hard.csv, test_waypoints.csv
```

### 2. Generate plots for all test scenarios

```bash
python plot_test_dynamics.py
# Output: *_overview.svg, *_3d.svg, *_topdown.svg, *_position.svg, *_attitude.svg
```

### 3. Generate animated GIFs for all test scenarios

```bash
python animate_drone.py
# Output: anim_hover.gif, anim_roll.gif, anim_climb.gif, anim_hard.gif, anim_waypoints.gif
```

To animate a custom flight log from an experiment:

```bash
python animate_drone.py --csv experiments_flight/flight_log_preempt_rt_fifo_p99_stress.csv \
                        --gif my_flight.gif \
                        --title "PREEMPT_RT FIFO p99 under stress"
```

---

## Running the SITL Experiments (Raspberry Pi)

> These steps require a Linux system with real-time scheduling privileges (`sudo`) and optionally `stress-ng` for CPU load injection.

### Step 1 — Standard kernel

Boot into your standard Linux kernel and run:

```bash
chmod +x run_path_a_experiments.sh
./run_path_a_experiments.sh standard
```

### Step 2 — PREEMPT_RT kernel

Reboot into your PREEMPT_RT-patched kernel and run:

```bash
./run_path_a_experiments.sh preempt_rt
```

Each run saves 16 CSVs (8 configs × idle/stress) to `experiments_flight/`.

### Step 3 — Analyse results

Once you have all 32 CSV files, run the evaluator:

```bash
python evaluate_performance.py
```

This generates:
- LaTeX-formatted performance table (printed to stdout)
- `experiments_flight/plots/comparative_best_vs_worst.pdf`
- `experiments_flight/plots/comparative_3d_trajectory.pdf`
- `experiments_flight/plots/comparative_3d_trajectory_subplots.pdf`
- Per-run SVG plots in `experiments_flight/plots/`
- Animated GIFs for worst and best cases under stress

---

## Scheduling Configurations

The experiment covers 8 configurations that exactly mirror Table III of the paper:

| Name | Policy | Priority / Nice | Runtime (µs) |
|------|--------|----------------|--------------|
| `other_n0` | `SCHED_OTHER` | nice 0 | — |
| `other_n19` | `SCHED_OTHER` | nice -19 | — |
| `fifo_p50` | `SCHED_FIFO` | priority 50 | — |
| `fifo_p99` | `SCHED_FIFO` | priority 99 | — |
| `rr_p50` | `SCHED_RR` | priority 50 | — |
| `rr_p99` | `SCHED_RR` | priority 99 | — |
| `deadline_R400` | `SCHED_DEADLINE` | — | 400 µs / 4000 µs |
| `deadline_R800` | `SCHED_DEADLINE` | — | 800 µs / 4000 µs |

---

## CSV Log Format

Each flight log CSV produced by `experiment_fcs_closed_loop.c` has the following columns:

```
time, x, y, z, roll, pitch, yaw, ref_x, ref_y, ref_z, F, tau_x, tau_y, tau_z, dt, latency_us
```

| Column | Unit | Description |
|--------|------|-------------|
| `time` | s | Elapsed simulation time |
| `x`, `y`, `z` | m | Drone position |
| `roll`, `pitch`, `yaw` | rad | Euler angles |
| `ref_x`, `ref_y`, `ref_z` | m | Active waypoint position |
| `F` | N | Total thrust command |
| `tau_x`, `tau_y`, `tau_z` | N·m | Body torque commands |
| `dt` | s | Actual loop period (measured by OS clock) |
| `latency_us` | µs | Scheduling jitter (actual wake-up vs expected wake-up) |

---

## Prerequisites

- GCC / Clang with `-lm`
- Linux (for real-time scheduling experiments; `SCHED_DEADLINE` requires Linux ≥ 3.14)
- Python 3 with: `pandas`, `numpy`, `matplotlib`
- `stress-ng` (for CPU/memory/IO load injection during experiments)

---

## License

MIT License — see the LICENSE file for details.

## Acknowledgments

Research conducted at ... . Special thanks to contributors ...
