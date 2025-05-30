#ifndef QUADCOPTER_CONTROL_H
#define QUADCOPTER_CONTROL_H
#include <math.h>

#include <stdbool.h>

// Vector and matrix utility types
typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    float roll;     // phi (φ)
    float pitch;    // theta (θ)
    float yaw;      // psi (ψ)
} Attitude;

typedef struct {
    float p, q, r;  // Roll, pitch, yaw rates
} AngularVelocity;

typedef struct {
    float motor1, motor2, motor3, motor4;  // Motor speeds (ω₁, ω₂, ω₃, ω₄)
} MotorCommands;

typedef struct {
    float Kp_pos;
    float Kd_pos;
    float Kp_att;
    float Kd_att;
    float Kp_yaw;
    float Kd_yaw;
} ControllerGains;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Attitude attitude;
    AngularVelocity angular;
    MotorCommands motors;
} QuadcopterState;

typedef struct {
    Vector3 position;
    float yaw;
} Reference;

typedef struct {
    Vector3 force;
    Vector3 torque;
    float total_thrust;
    Attitude desired_att;
} ControlOutputs;

void initializeController(ControllerGains *gains);

void positionController(
    const Vector3 *current_pos,
    const Vector3 *current_vel,
    const Vector3 *reference_pos,
    const ControllerGains *gains,
    Vector3 *output_force);

void controlAllocator1(
    const Vector3 *force,
    float reference_yaw,
    Attitude *desired_attitude,
    float *total_thrust);

void attitudeController(
    const Attitude *current_attitude,
    const AngularVelocity *current_angular,
    const Attitude *desired_attitude,
    const ControllerGains *gains,
    Vector3 *output_torque);

void controlAllocator2(
    float thrust,
    const Vector3 *torque,
    float motor_distance,
    float thrust_coefficient,
    float torque_coefficient,
    MotorCommands *motor_commands);

void updateFlightControl(
    const QuadcopterState *current_state,
    const Reference *reference,
    const ControllerGains *gains,
    float motor_distance,
    float thrust_coefficient,
    float torque_coefficient,
    ControlOutputs *control_outputs,
    MotorCommands *motor_commands);

void updateCurrentState(
    QuadcopterState *state,
    const Vector3 *new_position,
    const Vector3 *new_velocity,
    const Attitude *new_attitude,
    const AngularVelocity *new_angular);

#endif // QUADCOPTER_CONTROL_H


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