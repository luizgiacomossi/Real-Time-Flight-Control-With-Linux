#include "quadrotor_dynamics.h"
#include <math.h>

void quadrotor_dynamics(const QuadcopterState* state, float F, const Vector3* tau, QuadcopterState* dx) {
    float phi = state->attitude.roll;
    float theta = state->attitude.pitch;
    float psi = state->attitude.yaw;
    
    float cphi = cosf(phi), sphi = sinf(phi);
    float cth = cosf(theta), sth = sinf(theta);
    float cpsi = cosf(psi), spsi = sinf(psi);

    // Position derivative = velocity
    dx->position.x = state->velocity.x;
    dx->position.y = state->velocity.y;
    dx->position.z = state->velocity.z;

    // Linear acceleration aligned with the FCS controller's frame convention:
    // (Positive pitch tilts thrust backward -> negative X acc)
    // (Positive roll tilts thrust right -> positive Y acc)
    dx->velocity.x = (F / QUAD_MASS) * (-cpsi * sth * cphi - spsi * sphi);
    dx->velocity.y = (F / QUAD_MASS) * (-spsi * sth * cphi + cpsi * sphi);
    dx->velocity.z = (F / QUAD_MASS) * (cth * cphi) - QUAD_G;

    // Euler angle rates from body angular velocity
    float p = state->angular.p;
    float q = state->angular.q;
    float r = state->angular.r;
    
    dx->attitude.roll = p + sphi * tanf(theta) * q + cphi * tanf(theta) * r;
    dx->attitude.pitch = cphi * q - sphi * r;
    
    // Avoid division by zero when pitch is 90 degrees
    float cth_safe = (fabsf(cth) < 1e-6f) ? 1e-6f : cth;
    dx->attitude.yaw = (sphi / cth_safe) * q + (cphi / cth_safe) * r;

    // Angular acceleration (Euler's equations)
    dx->angular.p = (tau->x + (QUAD_IY - QUAD_IZ) * q * r) / QUAD_IX;
    dx->angular.q = (tau->y + (QUAD_IZ - QUAD_IX) * p * r) / QUAD_IY;
    dx->angular.r = (tau->z + (QUAD_IX - QUAD_IY) * p * q) / QUAD_IZ;
}

void quadrotor_step_euler(QuadcopterState* state, float F, const Vector3* tau, float dt) {
    QuadcopterState dx;
    quadrotor_dynamics(state, F, tau, &dx);

    state->position.x += dx.position.x * dt;
    state->position.y += dx.position.y * dt;
    state->position.z += dx.position.z * dt;

    state->velocity.x += dx.velocity.x * dt;
    state->velocity.y += dx.velocity.y * dt;
    state->velocity.z += dx.velocity.z * dt;

    state->attitude.roll += dx.attitude.roll * dt;
    state->attitude.pitch += dx.attitude.pitch * dt;
    state->attitude.yaw += dx.attitude.yaw * dt;

    state->angular.p += dx.angular.p * dt;
    state->angular.q += dx.angular.q * dt;
    state->angular.r += dx.angular.r * dt;

    // Wrap yaw to [-pi, pi] to prevent unbounded growth
    while (state->attitude.yaw >  M_PI) state->attitude.yaw -= 2.0f * M_PI;
    while (state->attitude.yaw < -M_PI) state->attitude.yaw += 2.0f * M_PI;
}
