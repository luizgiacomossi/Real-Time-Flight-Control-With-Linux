#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/resource.h>
#include <stdint.h>
#include <math.h>

#include "fcs_drone.h"
#include "quadrotor_dynamics.h"

// For sched_setattr
#include <linux/types.h>
#include <linux/unistd.h>

#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE 6
#endif

struct sched_attr {
    __u32 size;
    __u32 sched_policy;
    __u64 sched_flags;
    __s32 sched_nice;
    __u32 sched_priority;
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

// Function for performing computations (busy wait)
void perform_computation(long computation_amount) {
    volatile unsigned long acc = 171717;
    for (long i = 0; i < computation_amount; i++) {
        for (long j = 0; j < 100; j++) {
            acc += i + j * 17;
        }
    }
}

// Print usage information
void print_usage(char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -p, --period=MICROSEC      Set period in microseconds (default: 4000)\n");
    printf("  -c, --computation=AMOUNT   Set computation amount (default: 100)\n");
    printf("  -j, --jobs=NUM             Set number of jobs to execute (default: 3750)\n");
    printf("  -a, --affinity=CPU         Set CPU affinity (default: disabled)\n");
    printf("  -m, --mlockall             Enable memory locking (default: disabled)\n");
    printf("  -s, --scheduler=POLICY     Set scheduling policy (other, rr, fifo, deadline) (default: other)\n");
    printf("  -n, --nice=NICE            Set nice value for SCHED_OTHER (-20 to 19) (default: 0)\n");
    printf("  -r, --rtprio=PRIO          Set real-time priority for SCHED_RR/FIFO (1-99) (default: 50)\n");
    printf("  -R, --runtime=MICROSEC     Set runtime for SCHED_DEADLINE in microseconds (default: 2000)\n");
    printf("  -D, --deadline=MICROSEC    Set relative deadline for SCHED_DEADLINE in microseconds (default: period)\n");
    printf("  -o, --output=FILE          Output metrics to CSV file (default: flight_log.csv)\n");
    printf("  -h, --help                 Display this help message\n");
}

int main(int argc, char *argv[]) {
    // Default parameters
    long period_us = 4000;         // 4ms period -> 250 Hz
    long computation_amount = 100; // Default computation amount
    int num_jobs = 3750;           // 15 seconds at 250 Hz
    int cpu_affinity = -1;         // Default: no CPU affinity
    int mlockall_flag = 0;         // Default: no memory locking
    char sched_policy[10] = "other";// Default scheduler
    int nice_value = 0;            // Default nice value
    int rt_priority = 50;          // Default RT priority
    long runtime_us = 2000;        // Default runtime for SCHED_DEADLINE
    long deadline_us = 0;          // Default deadline (0 means use period)
    char output_file[256] = "flight_log.csv"; // Default output file

    // Parse command-line arguments
    struct option long_options[] = {
        {"period", required_argument, 0, 'p'},
        {"computation", required_argument, 0, 'c'},
        {"jobs", required_argument, 0, 'j'},
        {"affinity", required_argument, 0, 'a'},
        {"mlockall", no_argument, 0, 'm'},
        {"scheduler", required_argument, 0, 's'},
        {"nice", required_argument, 0, 'n'},
        {"rtprio", required_argument, 0, 'r'},
        {"runtime", required_argument, 0, 'R'},
        {"deadline", required_argument, 0, 'D'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opt, option_index = 0;
    while ((opt = getopt_long(argc, argv, "p:c:j:a:ms:n:r:R:D:o:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p': period_us = atol(optarg); break;
            case 'c': computation_amount = atol(optarg); break;
            case 'j': num_jobs = atoi(optarg); break;
            case 'a': cpu_affinity = atoi(optarg); break;
            case 'm': mlockall_flag = 1; break;
            case 's': strncpy(sched_policy, optarg, sizeof(sched_policy) - 1); break;
            case 'n': nice_value = atoi(optarg); break;
            case 'r': rt_priority = atoi(optarg); break;
            case 'R': runtime_us = atol(optarg); break;
            case 'D': deadline_us = atol(optarg); break;
            case 'o': strncpy(output_file, optarg, sizeof(output_file) - 1); break;
            case 'h': print_usage(argv[0]); return EXIT_SUCCESS;
            default: print_usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (deadline_us == 0) {
        deadline_us = period_us;
    }

    // Allocate memory for logs
    double *log_time = malloc(num_jobs * sizeof(double));
    double *log_ref_yaw = malloc(num_jobs * sizeof(double));
    double *log_yaw = malloc(num_jobs * sizeof(double));
    double *log_roll = malloc(num_jobs * sizeof(double));
    double *log_pitch = malloc(num_jobs * sizeof(double));
    double *log_F = malloc(num_jobs * sizeof(double));
    double *log_tau_x = malloc(num_jobs * sizeof(double));
    double *log_tau_y = malloc(num_jobs * sizeof(double));
    double *log_tau_z = malloc(num_jobs * sizeof(double));
    double *log_dt = malloc(num_jobs * sizeof(double));
    double *log_latency = malloc(num_jobs * sizeof(double));

    if (!log_time || !log_ref_yaw || !log_yaw || !log_roll || !log_pitch || 
        !log_F || !log_tau_x || !log_tau_y || !log_tau_z || !log_dt || !log_latency) {
        perror("Failed to allocate memory for logs");
        return EXIT_FAILURE;
    }

    // Set CPU affinity
    if (cpu_affinity >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_affinity, &cpuset);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("Failed to set CPU affinity");
            return EXIT_FAILURE;
        }
    }

    // Lock memory
    if (mlockall_flag) {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            perror("Failed to lock memory");
            return EXIT_FAILURE;
        }
    }

    // Set scheduling policy
    if (strcmp(sched_policy, "other") == 0) {
        if (setpriority(PRIO_PROCESS, 0, nice_value) != 0) {
            perror("Failed to set nice value");
            return EXIT_FAILURE;
        }
    } else if (strcmp(sched_policy, "rr") == 0 || strcmp(sched_policy, "fifo") == 0) {
        struct sched_param param;
        param.sched_priority = rt_priority;
        int policy = (strcmp(sched_policy, "rr") == 0) ? SCHED_RR : SCHED_FIFO;
        if (sched_setscheduler(0, policy, &param) != 0) {
            perror("Failed to set RT scheduling policy");
            return EXIT_FAILURE;
        }
    } else if (strcmp(sched_policy, "deadline") == 0) {
        struct sched_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime = runtime_us * 1000ULL;
        attr.sched_deadline = deadline_us * 1000ULL;
        attr.sched_period = period_us * 1000ULL;
        if (sched_setattr(0, &attr, 0) != 0) {
            perror("Failed to set SCHED_DEADLINE");
            return EXIT_FAILURE;
        }
    }

    // FCS Initialization
    QuadcopterState current_state = {
        .position = {0.0f, 0.0f, 0.0f},
        .velocity = {0.0f, 0.0f, 0.0f},
        .attitude = {0.0f, 0.0f, 0.0f},
        .angular = {0.0f, 0.0f, 0.0f},
        .motors = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    Reference reference = {
        .position = {0.0f, 0.0f, 1.0f},  // Target hover at z=1.0m
        .yaw = 0.0f
    };

    ControllerGains gains;
    initializeController(&gains);
    
    // Physical parameters
    float motor_distance = 0.25f;
    float thrust_coefficient = 1.0e-5f;
    float torque_coefficient = 1.0e-6f;

    // Initialize outputs so the first physics step doesn't blow up
    ControlOutputs control_outputs = {
        .total_thrust = 1.0f * 9.81f, // QUAD_MASS * QUAD_G
        .torque = {0.0f, 0.0f, 0.0f}
    };
    MotorCommands motor_commands;

    printf("Starting closed-loop FCS with %s scheduler...\n", sched_policy);

    struct timespec ts_next, ts_start, ts_prev, ts_expected;
    clock_gettime(CLOCK_MONOTONIC, &ts_next);
    ts_start = ts_next;
    ts_prev = ts_next;
    ts_expected = ts_next;

    // Main job loop
    for (int k = 0; k < num_jobs; k++) {
        struct timespec ts_now;
        clock_gettime(CLOCK_MONOTONIC, &ts_now);

        double latency_us = ((ts_now.tv_sec - ts_expected.tv_sec) * 1e6) + ((ts_now.tv_nsec - ts_expected.tv_nsec) / 1000.0);
        double elapsed_total = (ts_now.tv_sec - ts_start.tv_sec) + (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9;
        double dt = (ts_now.tv_sec - ts_prev.tv_sec) + (ts_now.tv_nsec - ts_prev.tv_nsec) / 1e9;
        
        if (k == 0) {
            dt = period_us / 1e6; // Default to nominal dt on first step
            latency_us = 0;
        }
        ts_prev = ts_now;

        // 1. Calculate Disturbance
        Vector3 tau_dist = {0.0f, 0.0f, 0.0f};
        if (elapsed_total >= 5.0 && elapsed_total <= 5.5) {
            tau_dist.x = 0.1f; // Roll disturbance
        }

        Vector3 applied_torque = {
            control_outputs.torque.x + tau_dist.x,
            control_outputs.torque.y + tau_dist.y,
            control_outputs.torque.z + tau_dist.z
        };

        // 2. Simulate Physics BEFORE Control Calculation!
        // This perfectly models the fact that during the OS delay (dt), 
        // the drone was flying using the OLD motor speeds.
        quadrotor_step_euler(&current_state, control_outputs.total_thrust, &applied_torque, (float)dt);

        // 3. Update Trajectory Setpoint
        if (elapsed_total >= 2.0) {
            reference.yaw = 0.5236f; // 30 degrees
        } else {
            reference.yaw = 0.0f;
        }

        // 4. Run Control Law
        // Calculate the NEW motor speeds based on the newly updated state
        updateFlightControl(
            &current_state,
            &reference,
            &gains,
            motor_distance,
            thrust_coefficient,
            torque_coefficient,
            &control_outputs,
            &motor_commands);

        // Log Data
        log_time[k] = elapsed_total;
        log_ref_yaw[k] = reference.yaw;
        log_yaw[k] = current_state.attitude.yaw;
        log_roll[k] = current_state.attitude.roll;
        log_pitch[k] = current_state.attitude.pitch;
        log_F[k] = control_outputs.total_thrust;
        log_tau_x[k] = control_outputs.torque.x;
        log_tau_y[k] = control_outputs.torque.y;
        log_tau_z[k] = control_outputs.torque.z;
        log_dt[k] = dt;
        log_latency[k] = latency_us;

        // Simulate CPU Load
        perform_computation(computation_amount);

        // Sleep until next period
        ts_next.tv_nsec += period_us * 1000;
        while (ts_next.tv_nsec >= 1000000000L) {
            ts_next.tv_sec++;
            ts_next.tv_nsec -= 1000000000L;
        }
        
        ts_expected = ts_next;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_next, NULL);
    }

    // Write logs to disk
    if (strlen(output_file) > 0) {
        FILE *f = fopen(output_file, "w");
        if (f) {
            fprintf(f, "time,ref_yaw,yaw,roll,pitch,F,tau_x,tau_y,tau_z,dt,latency_us\n");
            for (int i = 0; i < num_jobs; i++) {
                fprintf(f, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", 
                        log_time[i], log_ref_yaw[i], log_yaw[i], log_roll[i], log_pitch[i],
                        log_F[i], log_tau_x[i], log_tau_y[i], log_tau_z[i], log_dt[i], log_latency[i]);
            }
            fclose(f);
            printf("Logged metrics to %s\n", output_file);
        } else {
            perror("Failed to open output file");
        }
    }

    free(log_time);
    free(log_ref_yaw);
    free(log_yaw);
    free(log_roll);
    free(log_pitch);
    free(log_F);
    free(log_tau_x);
    free(log_tau_y);
    free(log_tau_z);
    free(log_dt);
    free(log_latency);

    return EXIT_SUCCESS;
}
