#ifndef QUADCOPTER_CONTROL_H
#define QUADCOPTER_CONTROL_H

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
