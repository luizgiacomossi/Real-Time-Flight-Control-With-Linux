#define _GNU_SOURCE // This should be defined before including headers

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/mman.h>    // For mlockall
#include <sched.h>       // For sched_setaffinity, cpu_set_t, CPU_SET, CPU_ZERO
#include <sys/syscall.h> // For syscall
#include <errno.h>
#include <sys/resource.h> // For setpriority
#include <stdint.h>
#include "fcs_drone.h" // Include your flight control system header

/*
// Scheduling updateFlightControlttr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
}
*/
// Wrapper for sched_setattr syscall
// static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
//    return syscall(SYS_sched_setattr, pid, attr, flags);
//}


void simulate_data_acquisition(int samples, long delay_ns) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = delay_ns;

    for (int i = 0; i < samples; i++) {
        // Simulate reading sensor data
        //printf("Acquiring sample %d\n", i + 1);

        // Delay to simulate acquisition interval
        nanosleep(&req, NULL);
    }
}
    

// Function for performing computations (busy wait)
void perform_computation(long computation_amount)
{
    volatile unsigned long acc = 171717;
    for (long i = 0; i < computation_amount; i++)
    {
        for (long j = 0; j < 100; j++)
        {
            acc += i + j * 17;
        }
    }
}

// Function for performing computations -- fcs
void perform_computation_fcs(long computation_amount)
{

    // Initialize drone state
    QuadcopterState current_state = {
        .position = {0.0f, 0.0f, 0.0f},
        .velocity = {0.0f, 0.0f, 0.0f},
        .attitude = {0.0f, 0.0f, 0.0f},
        .angular = {0.0f, 0.0f, 0.0f},
        .motors = {0.0f, 0.0f, 0.0f, 0.0f}};

    bool PRINT_INFO = false;

    // Initialize timekeeping
    struct timespec req, rem; // req is the requested time, rem is the remaining time
    req.tv_sec = 1;
    req.tv_nsec = 500000000; // 500ms in nanoseconds
    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    double time_taken;

    struct timespec start, end;
    long seconds, nanoseconds;
    struct timespec start_time, end_time;

    clock_gettime(CLOCK_MONOTONIC, &start); // Get the start time

    // Set reference position and yaw
    Reference reference = {
        .position = {1.0f, 1.0f, 1.0f}, // Target position [x, y, z]
        .yaw = 0.0f                     // Target heading
    };

    // Initialize controller gains
    ControllerGains gains;
    initializeController(&gains); // Set default gains

    // Physical parameters - stantadt values for a quadcopter, change as needed
    // These values are typically determined by the drone's design and motor characteristics
    // and should be calibrated for the specific drone being used.
    float motor_distance = 0.25f;       // Distance from center to motor (m)
    float thrust_coefficient = 1.0e-5f; // Thrust coefficient (N/(rad/s)²)
    float torque_coefficient = 1.0e-6f; // Torque coefficient (Nm/(rad/s)²)

    // Control outputs
    ControlOutputs control_outputs;
    MotorCommands motor_commands;
    // Update flight control

    // Simulate some sensor updates (for testing)
    // adds a delay to simulate sensor updates before value updates -> IMU with 500 Hz using the function simulate_data_acquisition
    //simulate_data_acquisition(1, 2000000); // 1 sample, 2ms delay between samples



    current_state.position.x += 0.1f;
    current_state.position.y += 0.1f;
    current_state.position.z += 0.1f;

    current_state.velocity.x += 0.01f;
    current_state.velocity.y += 0.01f;
    current_state.velocity.z += 0.01f;

    current_state.attitude.roll += 0.01f;
    current_state.attitude.pitch += 0.01f;
    current_state.attitude.yaw += 0.01f;

    current_state.angular.p += 0.01f;
    current_state.angular.q += 0.01f;
    current_state.angular.r += 0.01f;

    // Simulate a delay (for example, 100ms)
    // In a real system, this would be the time between control updates
    // Sleep for 1.5 seconds
    // nanosleep(&req, NULL);

    // Update flight control
    updateFlightControl(
        &current_state,
        &reference,
        &gains,
        motor_distance,
        thrust_coefficient,
        torque_coefficient,
        &control_outputs,
        &motor_commands);

    // printf("Controller updated:\n");
}

// Print usage information
void print_usage(char *program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -p, --period=MICROSEC      Set period in microseconds (default: 10000)\n");
    printf("  -c, --computation=AMOUNT   Set computation amount (default: 100)\n");
    printf("  -j, --jobs=NUM             Set number of jobs to execute (default: 100)\n");
    printf("  -a, --affinity=CPU         Set CPU affinity (default: disabled)\n");
    printf("  -m, --mlockall             Enable memory locking (default: disabled)\n");
    printf("  -s, --scheduler=POLICY     Set scheduling policy (other, rr, fifo, deadline) (default: other)\n");
    printf("  -n, --nice=NICE            Set nice value for SCHED_OTHER (-20 to 19) (default: 0)\n");
    printf("  -r, --rtprio=PRIO          Set real-time priority for SCHED_RR/FIFO (1-99) (default: 50)\n");
    printf("  -R, --runtime=MICROSEC     Set runtime for SCHED_DEADLINE in microseconds (default: 5000)\n");
    printf("  -D, --deadline=MICROSEC    Set relative deadline for SCHED_DEADLINE in microseconds (default: period)\n");
    printf("  -o, --output=FILE          Output response times to file (default: no file output)\n");
    printf("  -h, --help                 Display this help message\n");
}

int main(int argc, char *argv[])
{
    // Default parameters
    long period_us = 10000;        // 10ms default period
    long computation_amount = 100; // Default computation amount
    int num_jobs = 1000;           // Default number of jobs
    int cpu_affinity = -1;         // Default: no CPU affinity
    int mlockall_flag = 0;         // Default: no memory locking
    char sched_policy[10] = "fifo";  // Default scheduler
    int nice_value = 0;            // Default nice value
    int rt_priority = 50;          // Default RT priority
    long runtime_us = 5000;        // Default runtime for SCHED_DEADLINE (50% of period)
    long deadline_us = 0;          // Default deadline (0 means use period)
    char output_file[256] = "";    // Default: no file output

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
    while ((opt = getopt_long(argc, argv, "p:c:j:a:ms:n:r:R:D:o:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'p':
            period_us = atol(optarg);
            if (period_us <= 0)
            {
                fprintf(stderr, "Period must be positive\n");
                return EXIT_FAILURE;
            }
            break;
        case 'c':
            computation_amount = atol(optarg);
            if (computation_amount <= 0)
            {
                fprintf(stderr, "Computation amount must be positive\n");
                return EXIT_FAILURE;
            }
            break;
        case 'j':
            num_jobs = atoi(optarg);
            if (num_jobs <= 0)
            {
                fprintf(stderr, "Number of jobs must be positive\n");
                return EXIT_FAILURE;
            }
            break;
        case 'a':
            cpu_affinity = atoi(optarg);
            if (cpu_affinity < 0)
            {
                fprintf(stderr, "CPU affinity must be non-negative\n");
                return EXIT_FAILURE;
            }
            break;
        case 'm':
            mlockall_flag = 1;
            break;
        case 's':
            strncpy(sched_policy, optarg, sizeof(sched_policy) - 1);
            break;
        case 'n':
            nice_value = atoi(optarg);
            if (nice_value < -20 || nice_value > 19)
            {
                fprintf(stderr, "Nice value must be between -20 and 19\n");
                return EXIT_FAILURE;
            }
            break;
        case 'r':
            rt_priority = atoi(optarg);
            if (rt_priority < 1 || rt_priority > 99)
            {
                fprintf(stderr, "Real-time priority must be between 1 and 99\n");
                return EXIT_FAILURE;
            }
            break;
        case 'R':
            runtime_us = atol(optarg);
            if (runtime_us <= 0)
            {
                fprintf(stderr, "Runtime must be positive\n");
                return EXIT_FAILURE;
            }
            break;
        case 'D':
            deadline_us = atol(optarg);
            if (deadline_us <= 0)
            {
                fprintf(stderr, "Deadline must be positive\n");
                return EXIT_FAILURE;
            }
            break;
        case 'o':
            strncpy(output_file, optarg, sizeof(output_file) - 1);
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // If deadline not specified, use period
    if (deadline_us == 0)
    {
        deadline_us = period_us;
    }

    // Allocate memory for response times
    long *resptimes_us = malloc(num_jobs * sizeof(long));
    if (!resptimes_us)
    {
        perror("Failed to allocate memory for response times");
        return EXIT_FAILURE;
    }

    // Set CPU affinity if requested
    if (cpu_affinity >= 0)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_affinity, &cpuset);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0)
        {
            perror("Failed to set CPU affinity");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
        printf("Thread affinity set to CPU %d\n", cpu_affinity);
    }

    // Lock memory if requested
    if (mlockall_flag)
    {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        {
            perror("Failed to lock memory");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
        printf("Memory locking enabled\n");
    }

    // Set scheduling policy
    if (strcmp(sched_policy, "other") == 0)
    {
        // For SCHED_OTHER, we use nice value
        if (setpriority(PRIO_PROCESS, 0, nice_value) != 0)
        {
            perror("Failed to set nice value");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
        printf("Using SCHED_OTHER with nice value: %d\n", nice_value);
    }
    else if (strcmp(sched_policy, "rr") == 0 || strcmp(sched_policy, "fifo") == 0)
    {
        // Set RT priority for SCHED_RR or SCHED_FIFO
        struct sched_param param;
        param.sched_priority = rt_priority;

        int policy = (strcmp(sched_policy, "rr") == 0) ? SCHED_RR : SCHED_FIFO;
        if (sched_setscheduler(0, policy, &param) != 0)
        {
            perror("Failed to set RT scheduling policy");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
        printf("Using %s with priority: %d\n",
               (policy == SCHED_RR) ? "SCHED_RR" : "SCHED_FIFO",
               rt_priority);
    }
    else if (strcmp(sched_policy, "deadline") == 0)
    {
        // Set SCHED_DEADLINE parameters
        struct sched_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime = runtime_us * 1000ULL;   // Convert to ns
        attr.sched_deadline = deadline_us * 1000ULL; // Convert to ns
        attr.sched_period = period_us * 1000ULL;     // Convert to ns

        if (sched_setattr(0, &attr, 0) != 0)
        {
            perror("Failed to set SCHED_DEADLINE");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
        printf("Using SCHED_DEADLINE with runtime: %ld us, deadline: %ld us, period: %ld us\n",
               runtime_us, deadline_us, period_us);
    }
    else
    {
        fprintf(stderr, "Unknown scheduling policy: %s\n", sched_policy);
        free(resptimes_us);
        return EXIT_FAILURE;
    }

    // Print configuration summary
    printf("Starting periodic real-time application with:\n");
    printf("  Period: %ld us\n", period_us);
    printf("  Computation amount: %ld iterations\n", computation_amount);
    printf("  Number of jobs: %d\n", num_jobs);

    // Initialize next activation time
    struct timespec ts_next;
    clock_gettime(CLOCK_MONOTONIC, &ts_next);

    // Main job loop
    for (int k = 0; k < num_jobs; k++)
    {
        // Perform computation
        perform_computation_fcs(computation_amount);

        // Measure response time
        struct timespec ts_current;
        clock_gettime(CLOCK_MONOTONIC, &ts_current);

        // Calculate execution time in microseconds
        resptimes_us[k] = (ts_current.tv_sec - ts_next.tv_sec) * 1000000L +
                          (ts_current.tv_nsec - ts_next.tv_nsec) / 1000L;

        // Print current response time
        //printf("Job %d: Response time = %ld us\n", k, resptimes_us[k]);

        // Calculate next activation time
        ts_next.tv_nsec += period_us * 1000; // Convert us to ns
        while (ts_next.tv_nsec >= 1000000000L)
        { // Handle nanosecond overflowf
            ts_next.tv_sec++;
            ts_next.tv_nsec -= 1000000000L;
        }

        // Sleep until next activation time
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_next, NULL) != 0)
        {
            perror("clock_nanosleep error");
            free(resptimes_us);
            return EXIT_FAILURE;
        }
    }

    // Calculate statistics
    long min_resp = resptimes_us[0];
    long max_resp = resptimes_us[0];
    long total_resp = 0;

    for (int i = 0; i < num_jobs; i++)
    {
        if (resptimes_us[i] < min_resp)
            min_resp = resptimes_us[i];
        if (resptimes_us[i] > max_resp)
            max_resp = resptimes_us[i];
        total_resp += resptimes_us[i];
    }

    double avg_resp = (double)total_resp / num_jobs;

    // Print statistics
    printf("\nResponse Time Statistics:\n");
    printf("  Minimum: %ld us\n", min_resp);
    printf("  Maximum: %ld us\n", max_resp);
    printf("  Average: %.2f us\n", avg_resp);

    // Write response times to file if requested
    if (strlen(output_file) > 0)
    {
        FILE *f = fopen(output_file, "w");
        if (!f)
        {
            perror("Failed to open output file");
        }
        else
        {
            for (int i = 0; i < num_jobs; i++)
            {
                fprintf(f, "%ld\n", resptimes_us[i]);
            }
            fclose(f);
            printf("Response times written to %s\n", output_file);
        }
    }

    // Cleanup
    free(resptimes_us);
    return EXIT_SUCCESS;
}