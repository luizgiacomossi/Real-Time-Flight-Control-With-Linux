#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "fcs_drone.h"  //  header



// Usage of the flight control system
int main() {
    // Initialize drone state
    QuadcopterState current_state = {
        .position = {0.0f, 0.0f, 0.0f},
        .velocity = {0.0f, 0.0f, 0.0f},
        .attitude = {0.0f, 0.0f, 0.0f},
        .angular = {0.0f, 0.0f, 0.0f},
        .motors = {0.0f, 0.0f, 0.0f, 0.0f}
    };
    
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

    clock_gettime(CLOCK_MONOTONIC, &start);  // Get the start time


    // Set reference position and yaw
    Reference reference = {
        .position = {1.0f, 1.0f, 1.0f},  // Target position [x, y, z]
        .yaw = 0.0f                      // Target heading
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
    for (int i = 0; i < 10; i++) {
        // Simulate some state changes (for testing)
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

        req.tv_sec = 1;        // 1 second
        req.tv_nsec = 500000000; // 500 milliseconds

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        updateFlightControl(
            &current_state,
            &reference,
            &gains,
            motor_distance,
            thrust_coefficient,
            torque_coefficient,
            &control_outputs,
            &motor_commands);

        if(PRINT_INFO){

                // Display results
        printf("Control Outputs:\n");
        printf("Position Error: [%.2f, %.2f, %.2f] m\n", 
            reference.position.x - current_state.position.x,
            reference.position.y - current_state.position.y,
            reference.position.z - current_state.position.z);
        printf("Force: [%.2f, %.2f, %.2f] N\n", 
            control_outputs.force.x, control_outputs.force.y, control_outputs.force.z);
        printf("Desired Attitude: [%.2f, %.2f, %.2f] rad\n", 
            control_outputs.desired_att.roll, control_outputs.desired_att.pitch, control_outputs.desired_att.yaw);
        printf("Total Thrust: %.2f N\n", control_outputs.total_thrust);
        printf("Torque: [%.2f, %.2f, %.2f] Nm\n", 
            control_outputs.torque.x, control_outputs.torque.y, control_outputs.torque.z);
        printf("Motor Commands: [%.2f, %.2f, %.2f, %.2f] rad/s\n", 
            motor_commands.motor1, motor_commands.motor2, motor_commands.motor3, motor_commands.motor4);

        printf("--------------------------------------------------\n");

        }
        // Simulate a delay (for example, 100ms)
        // In a real system, this would be the time between control updates
        // Sleep for 1.5 seconds
        nanosleep(&req, NULL);
        
        printf("Woke up after 1.5 seconds\n");

        // Step 3: Record end time
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        // Step 4: Calculate the elapsed time in seconds
        // Note: The elapsed time is calculated as the difference between end_time and start_time

        seconds = end_time.tv_sec - start_time.tv_sec;
        nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
        if (nanoseconds < 0) {
             seconds--;
             nanoseconds += 1000000000;
         }
        printf("Elapsed time: %ld seconds and %ld nanoseconds\n", seconds, nanoseconds);


    }


    
    return 0;
}