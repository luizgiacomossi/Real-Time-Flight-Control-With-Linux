#ifndef QUADROTOR_DYNAMICS_H
#define QUADROTOR_DYNAMICS_H

#include "fcs_drone.h"

// Constants (typical small quadcopter)
#define QUAD_MASS 1.0f       // mass (kg)
#define QUAD_G 9.81f         // gravity (m/s^2)
#define QUAD_IX 0.0123f      // inertia x (kg*m^2)
#define QUAD_IY 0.0123f      // inertia y
#define QUAD_IZ 0.0224f      // inertia z

// Evaluate dynamics derivative
void quadrotor_dynamics(const QuadcopterState* state, float F, const Vector3* tau, QuadcopterState* dx);

// Integration step using Euler
void quadrotor_step_euler(QuadcopterState* state, float F, const Vector3* tau, float dt);

#endif // QUADROTOR_DYNAMICS_H
