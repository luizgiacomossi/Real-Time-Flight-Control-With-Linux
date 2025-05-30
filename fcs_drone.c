#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "fcs_drone.h"  //  header



// Initialize controller with default gains
void initializeController(ControllerGains *gains) {
    // Position controller gains
    gains->Kp_pos = 2.0f;
    gains->Kd_pos = 1.5f;
    
    // Attitude controller gains
    gains->Kp_att = 8.0f;
    gains->Kd_att = 2.5f;
    
    // Yaw controller gains
    gains->Kp_yaw = 3.0f;
    gains->Kd_yaw = 1.0f;
}

// Position controller: calculate desired force based on position error
void positionController(
    const Vector3 *current_pos,
    const Vector3 *current_vel,
    const Vector3 *reference_pos,
    const ControllerGains *gains,
    Vector3 *output_force)
{
    // Calculate position error
    Vector3 pos_error;
    pos_error.x = reference_pos->x - current_pos->x;
    pos_error.y = reference_pos->y - current_pos->y;
    pos_error.z = reference_pos->z - current_pos->z;
    
    // PD controller for each axis
    output_force->x = gains->Kp_pos * pos_error.x - gains->Kd_pos * current_vel->x;
    output_force->y = gains->Kp_pos * pos_error.y - gains->Kd_pos * current_vel->y;
    output_force->z = gains->Kp_pos * pos_error.z - gains->Kd_pos * current_vel->z + 9.81f; // Add gravity compensation
}

// Control allocator 1: convert horizontal forces to desired attitude
void controlAllocator1(
    const Vector3 *force,
    float reference_yaw,
    Attitude *desired_attitude,
    float *total_thrust)
{
    // Calculate total thrust
    *total_thrust = sqrtf(force->x * force->x + force->y * force->y + force->z * force->z);
    
    if (*total_thrust < 0.1f) {
        // Avoid division by near-zero
        desired_attitude->roll = 0.0f;
        desired_attitude->pitch = 0.0f;
    } else {
        // Calculate desired roll and pitch based on desired acceleration
        desired_attitude->roll = asinf(force->y / *total_thrust);
        desired_attitude->pitch = -asinf(force->x / (*total_thrust * cosf(desired_attitude->roll)));
    }
    
    // Use reference yaw directly
    desired_attitude->yaw = reference_yaw;
}

// Attitude controller: calculate desired torque based on attitude error
void attitudeController(
    const Attitude *current_attitude,
    const AngularVelocity *current_angular,
    const Attitude *desired_attitude,
    const ControllerGains *gains,
    Vector3 *output_torque)
{
    // Calculate attitude error
    float roll_error = desired_attitude->roll - current_attitude->roll;
    float pitch_error = desired_attitude->pitch - current_attitude->pitch;
    float yaw_error = desired_attitude->yaw - current_attitude->yaw;
    
    // Normalize yaw error to [-π, π]
    while (yaw_error > M_PI) yaw_error -= 2.0f * M_PI;
    while (yaw_error < -M_PI) yaw_error += 2.0f * M_PI;
    
    // PD controller for roll and pitch
    output_torque->x = gains->Kp_att * roll_error - gains->Kd_att * current_angular->p;
    output_torque->y = gains->Kp_att * pitch_error - gains->Kd_att * current_angular->q;
    
    // PD controller for yaw
    output_torque->z = gains->Kp_yaw * yaw_error - gains->Kd_yaw * current_angular->r;
}

// Control allocator 2: convert thrust and torque to motor commands
void controlAllocator2(
    float thrust,
    const Vector3 *torque,
    float motor_distance,
    float thrust_coefficient,
    float torque_coefficient,
    MotorCommands *motor_commands)
{
    // Simplified quadcopter motor mixing for an X configuration
    // Assuming:
    // - motor 1: front right
    // - motor 2: rear left
    // - motor 3: front left
    // - motor 4: rear right
    
    float l = motor_distance / sqrtf(2.0f); // Moment arm for roll/pitch
    float k_t = thrust_coefficient;         // Thrust coefficient
    float k_m = torque_coefficient;         // Torque coefficient
    
    // Calculate individual motor thrusts
    float t1 = thrust/4.0f - torque->y/(4.0f*l) + torque->x/(4.0f*l) - torque->z/(4.0f*k_m);
    float t2 = thrust/4.0f + torque->y/(4.0f*l) - torque->x/(4.0f*l) - torque->z/(4.0f*k_m);
    float t3 = thrust/4.0f - torque->y/(4.0f*l) - torque->x/(4.0f*l) + torque->z/(4.0f*k_m);
    float t4 = thrust/4.0f + torque->y/(4.0f*l) + torque->x/(4.0f*l) + torque->z/(4.0f*k_m);
    
    // Convert thrust to motor angular velocity (ω = √(T/k_t))
    motor_commands->motor1 = sqrtf(fmaxf(t1, 0.0f) / k_t);
    motor_commands->motor2 = sqrtf(fmaxf(t2, 0.0f) / k_t);
    motor_commands->motor3 = sqrtf(fmaxf(t3, 0.0f) / k_t);
    motor_commands->motor4 = sqrtf(fmaxf(t4, 0.0f) / k_t);
}

// Main control function that ties everything together
void updateFlightControl(
    const QuadcopterState *current_state,
    const Reference *reference,
    const ControllerGains *gains,
    float motor_distance,
    float thrust_coefficient,
    float torque_coefficient,
    ControlOutputs *control_outputs,
    MotorCommands *motor_commands)
{
    // 1. Position Controller
    positionController(
        &current_state->position,
        &current_state->velocity,
        &reference->position,
        gains,
        &control_outputs->force);
    
    // 2. Control Allocator 1
    controlAllocator1(
        &control_outputs->force,
        reference->yaw,
        &control_outputs->desired_att,
        &control_outputs->total_thrust);
    
    // 3. Attitude Controller
    attitudeController(
        &current_state->attitude,
        &current_state->angular,
        &control_outputs->desired_att,
        gains,
        &control_outputs->torque);
    
    // 4. Control Allocator 2
    controlAllocator2(
        control_outputs->total_thrust,
        &control_outputs->torque,
        motor_distance,
        thrust_coefficient,
        torque_coefficient,
        motor_commands);
}

void updateCurrentState(
    QuadcopterState *state,
    const Vector3 *new_position,
    const Vector3 *new_velocity,
    const Attitude *new_attitude,
    const AngularVelocity *new_angular)
{
    state->position = *new_position;
    state->velocity = *new_velocity;
    state->attitude = *new_attitude;
    state->angular = *new_angular;
}

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
    
    bool PRINT_INFO = true;

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