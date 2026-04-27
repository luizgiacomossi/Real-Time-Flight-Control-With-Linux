#!/bin/bash
# run_path_a_experiments.sh
# Runs the closed-loop tracking experiments with and without stress-ng

if [ -z "$1" ]; then
    echo "Usage: $0 <kernel_name>"
    echo "Example: $0 standard"
    echo "Example: $0 preempt_rt"
    exit 1
fi
KERNEL_NAME=$1

# 1. Compile the code (notice we don't compile fcs_drone.c because fcs_drone.h has the implementations)
gcc -o fcs_closed_loop experiment_fcs_closed_loop.c quadrotor_dynamics.c -lm

# 8 configs matching Table III exactly
POLICIES=("other" "other" "fifo" "fifo" "rr" "rr" "deadline" "deadline")
NICE_VALS=("0" "-19" "0" "0" "0" "0" "0" "0")
PRIORITIES=("0" "0" "50" "99" "50" "99" "0" "0")
RUNTIMES=("0" "0" "0" "0" "0" "0" "400" "800")
PERIODS=("0" "0" "0" "0" "0" "0" "4000" "4000")
NAMES=("other_n0" "other_n19" "fifo_p50" "fifo_p99" "rr_p50" "rr_p99" "deadline_R400" "deadline_R800")

# Create output directory
OUT_DIR="experiments_flight"
mkdir -p "$OUT_DIR"

echo "Running standard experiments on kernel: $KERNEL_NAME..."
for i in "${!POLICIES[@]}"; do
    pol="${POLICIES[$i]}"
    n="${NICE_VALS[$i]}"
    r="${PRIORITIES[$i]}"
    R="${RUNTIMES[$i]}"
    D="${PERIODS[$i]}"
    name="${NAMES[$i]}"
    
    echo "Running idle config: $name"
    sudo ./fcs_closed_loop -p 4000 -c 1000 -j 7500 -a 0 -m -s "$pol" -n "$n" -r "$r" -R "$R" -D "$D" -o "$OUT_DIR/flight_log_${KERNEL_NAME}_${name}_idle.csv"
done

echo "Starting stress-ng..."
stress-ng \
  --cpu 4 --cpu-method matrixprod \
  --vm 2 --vm-bytes 75% --vm-method all --verify\
  --hdd 1 --hdd-bytes 1G \
  --pipe 4 --mq 2 --sock 2 \
  --switch 2 \
  --timeout 3000s & STRESS_PID=$!

sleep 5

echo "Running stress experiments on kernel: $KERNEL_NAME..."
for i in "${!POLICIES[@]}"; do
    pol="${POLICIES[$i]}"
    n="${NICE_VALS[$i]}"
    r="${PRIORITIES[$i]}"
    R="${RUNTIMES[$i]}"
    D="${PERIODS[$i]}"
    name="${NAMES[$i]}"
    
    echo "Running stressed config: $name"
    sudo ./fcs_closed_loop -p 4000 -c 1000 -j 7500 -a 0 -m -s "$pol" -n "$n" -r "$r" -R "$R" -D "$D" -o "$OUT_DIR/flight_log_${KERNEL_NAME}_${name}_stress.csv"
done

echo "Stopping stress-ng..."
sudo kill $STRESS_PID
wait $STRESS_PID 2>/dev/null

echo "Experiments complete for kernel: $KERNEL_NAME. Logs saved in $OUT_DIR/."
