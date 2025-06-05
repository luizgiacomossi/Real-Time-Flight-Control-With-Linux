#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration Parameters ---

# Common parameters for fcs real-time application (experiment_fcs)
# Updated for 250 Hz control loop (4 ms period)
PERIOD_US=4000         # Period in microseconds (4ms)
COMPUTATION_AMOUNT=10000 # Computation amount (iterations) - Remains the same
NUM_JOBS=10000         # Number of jobs to execute - Remains the same
CPU_AFFINITY=2         # Pin rt_app to CPU core 2
MLOCKALL_FLAG="true"   # Enable memory locking (true/false)

# SCHED_OTHER parameters
NICE_VALUES=(0 -19)

# SCHED_FIFO/RR parameters
RT_PRIORITIES=(50 99)

# For 250 Hz control loop:
# Period (P) = 4 ms = 4,000,000 ns
# Deadline (D) = 4 ms = 4,000,000 ns (typical to match period)
# If the original 500 Hz control loop had a WCET of 400 µs,
# then for 250 Hz, the computation amount might remain similar,
# but the runtime budget can be proportionally larger.
# Let's assume the WCET for the *computation* is still around 400 µs
# as the "COMPUTATION_AMOUNT" hasn't changed.
# For SCHED_DEADLINE, the runtime should reflect the actual execution time.

# attr.sched_runtime  = 400  000;    // Assuming the same 400 µs WCET for the computation block
# attr.sched_deadline = 4000  000;   // 4 ms
# attr.sched_period   = 4000  000;   // 4 ms

# SCHED_DEADLINE parameters
# Runtime should be an estimate of your task's actual execution time.
# If the original 500 Hz task used a 400us runtime, and the computation amount is the same,
# we'll still use 400us as a baseline for the runtime.
# We'll test with 400us and 800us (20% budget of 4ms) for comparison.
DEADLINE_RUNTIME_US=(400 800) # Runtime for SCHED_DEADLINE
PERIOD_NS=$((PERIOD_US )) # Convert period to nanoseconds
DEADLINE_NS=${PERIOD_NS}       # Deadline (typical to match period)


#run_test "deadline" "-R ${runtime_val} -D ${DEADLINE_NS}" "deadline_R${runtime_val}_D${DEADLINE_NS}_no_stress.txt" "false"

# Stress-ng parameters
# Adjust --cpu based on your system's core count. If rt_app is on CPU 0,
# --cpu 3 would stress cores 1, 2, 3 on a 4-core system.
# The timeout should be slightly longer than your rt_app's total execution time.
# rt_app duration = NUM_JOBS * PERIOD_US / 1,000,000 = 10000 * 2000 / 1,000,000 = 20 seconds.
STRESS_NG_TIMEOUT="5000s" # Set timeout slightly longer than rt_app run time
STRESS_NG_ARGS="--cpu 4 --cpu-method matrixprod \
                --vm 2 --vm-bytes 75% --vm-method all --verify \
                --hdd 1 --hdd-bytes 1G \
                --pipe 4 --mq 2 --sock 2 \
                --sched 2 \
                --timeout ${STRESS_NG_TIMEOUT}"

# Output directory
OUTPUT_DIR="experiment_results"

# --- Script Start ---

echo "--- Starting Real-time Scheduling Experiments ---"

# Create output directory if it doesn't exist
mkdir -p "${OUTPUT_DIR}"
echo "Output files will be saved in: ${OUTPUT_DIR}"
echo ""

# Function to run a single test case
run_test() {
    local scheduler=$1
    local scheduler_args=$2
    local output_filename=$3
    local stress_active=$4

    echo "Running test: Scheduler=${scheduler}, Args='${scheduler_args}', Stress=${stress_active}"

    local cmd="sudo ./experiment_fcs \
      -p ${PERIOD_US} \
      -j ${NUM_JOBS} \
      -a ${CPU_AFFINITY}"

    if [ "${MLOCKALL_FLAG}" = "true" ]; then
        cmd="${cmd} -m"
    fi

    cmd="${cmd} -s ${scheduler} ${scheduler_args} -o ${OUTPUT_DIR}/${output_filename}"

    # Execute the command
    eval "${cmd}"
    echo "Test completed. Output saved to ${OUTPUT_DIR}/${output_filename}"
    echo ""
}

# --- Phase 1: Run without stress-ng ---
echo "--- Phase 1: Running experiments WITHOUT stress-ng ---"
echo ""

# SCHED_OTHER tests
for nice_val in "${NICE_VALUES[@]}"; do
    run_test "other" "-n ${nice_val}" "other_n${nice_val}_no_stress.txt" "false"
done

# SCHED_FIFO tests
for rt_prio in "${RT_PRIORITIES[@]}"; do
    run_test "fifo" "-r ${rt_prio}" "fifo_p${rt_prio}_no_stress.txt" "false"
done

# SCHED_RR tests
for rt_prio in "${RT_PRIORITIES[@]}"; do
    run_test "rr" "-r ${rt_prio}" "rr_p${rt_prio}_no_stress.txt" "false"
done

# SCHED_DEADLINE tests
for runtime_val in "${DEADLINE_RUNTIME_US[@]}"; do
    run_test "deadline" "-p ${DEADLINE_NS} -R ${runtime_val} -D ${DEADLINE_NS}" "deadline_R${runtime_val}_D${DEADLINE_NS}_no_stress.txt" "false"
done

echo "--- Phase 1: All 'no stress' experiments completed. ---"
echo ""

# --- Phase 2: Run with stress-ng ---
echo "--- Phase 2: Running experiments WITH stress-ng ---"
echo ""

echo "Starting stress-ng in background with args: ${STRESS_NG_ARGS}"
# Note: stress-ng is run without sudo here, but since the whole script is sudo, it inherits root.
#stress-ng ${STRESS_NG_ARGS} &
#STRESS_PID=$!

stress-ng \
  --cpu 4 --cpu-method matrixprod \
  --vm 2 --vm-bytes 75% --vm-method all --verify\
  --hdd 1 --hdd-bytes 1G \
  --pipe 4 --mq 2 --sock 2 \
  --sched other \
  --timeout 30000s  & STRESS_PID=$!

echo "stress-ng Running"

echo "Waiting 5 seconds for stress-ng to warm up..."
sleep 5 # Give stress-ng a bit more time to get going

# SCHED_OTHER tests
for nice_val in "${NICE_VALUES[@]}"; do
    run_test "other" "-n ${nice_val}" "other_n${nice_val}_stress.txt" "true"
done

# SCHED_FIFO tests
for rt_prio in "${RT_PRIORITIES[@]}"; do
    run_test "fifo" "-r ${rt_prio}" "fifo_p${rt_prio}_stress.txt" "true"
done

# SCHED_RR tests
for rt_prio in "${RT_PRIORITIES[@]}"; do
    run_test "rr" "-r ${rt_prio}" "rr_p${rt_prio}_stress.txt" "true"
done

# SCHED_DEADLINE tests
for runtime_val in "${DEADLINE_RUNTIME_US[@]}"; do
    run_test "deadline" "-p ${DEADLINE_NS} -R ${runtime_val} -D ${DEADLINE_NS}" "deadline_R${runtime_val}_D${DEADLINE_NS}_stress.txt" "false"
done


echo "--- Phase 2: All 'with stress' experiments completed. ---"
echo ""

echo "Stopping stress-ng (PID: ${STRESS_PID})..."
sudo kill "${STRESS_PID}"
wait "${STRESS_PID}" # Wait for stress-ng to terminate gracefully
echo "stress-ng stopped."
echo ""

echo "--- All experiments finished. Results are in the '${OUTPUT_DIR}' directory. ---"
