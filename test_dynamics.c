#include "quadrotor_dynamics.c" // Include C file directly to bypass fcs_drone.h multiple definitions
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void print_state(const char *label, const QuadcopterState *state) {
  printf("%s\n", label);
  printf("Pos: [%.4f, %.4f, %.4f]\n", state->position.x, state->position.y,
         state->position.z);
  printf("Vel: [%.4f, %.4f, %.4f]\n", state->velocity.x, state->velocity.y,
         state->velocity.z);
  printf("Att: [%.4f, %.4f, %.4f]\n", state->attitude.roll,
         state->attitude.pitch, state->attitude.yaw);
  printf("Ang: [%.4f, %.4f, %.4f]\n", state->angular.p, state->angular.q,
         state->angular.r);
  printf("----------------------------------\n");
}

void log_state(FILE *f, float t, const QuadcopterState *state,
               const Reference *ref) {
  if (ref) {
    fprintf(f, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", t, state->position.x,
            state->position.y, state->position.z, state->attitude.roll,
            state->attitude.pitch, state->attitude.yaw, ref->position.x,
            ref->position.y, ref->position.z);
  } else {
    fprintf(f, "%f,%f,%f,%f,%f,%f,%f,0,0,0\n", t, state->position.x,
            state->position.y, state->position.z, state->attitude.roll,
            state->attitude.pitch, state->attitude.yaw);
  }
}

int main() {
  float dt = 0.01f;
  float F = QUAD_MASS * QUAD_G;

  // Test 1: Hover
  FILE *f1 = fopen("test_hover.csv", "w");
  if (f1) {
    fprintf(f1, "time,x,y,z,roll,pitch,yaw,ref_x,ref_y,ref_z\n");
    QuadcopterState state1 = {0};
    Vector3 tau1 = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 100; i++) {
      log_state(f1, i * dt, &state1, NULL);
      quadrotor_step_euler(&state1, F, &tau1, dt);
    }
    fclose(f1);
  }

  // Test 2: Roll
  FILE *f2 = fopen("test_roll.csv", "w");
  if (f2) {
    fprintf(f2, "time,x,y,z,roll,pitch,yaw,ref_x,ref_y,ref_z\n");
    QuadcopterState state2 = {0};
    Vector3 tau2 = {0.01f, 0.0f, 0.0f};
    for (int i = 0; i < 100; i++) {
      log_state(f2, i * dt, &state2, NULL);
      quadrotor_step_euler(&state2, F, &tau2, dt);
    }
    fclose(f2);
  }

  float motor_dist = 0.25f;
  float thrust_coefficient = 1.0e-5f;
  float torque_coefficient = 1.0e-6f;

  // Test 3: Climb only (Z=5)
  FILE *f3 = fopen("test_climb.csv", "w");
  if (f3) {
    fprintf(f3, "time,x,y,z,roll,pitch,yaw,ref_x,ref_y,ref_z\n");
    QuadcopterState state3 = {0}; // Start at origin
    Reference ref3 = {.position = {0.0f, 0.0f, 5.0f}, .yaw = 0.0f};
    ControllerGains gains3;
    initializeController(&gains3);
    ControlOutputs ctrl3;
    MotorCommands cmds3;

    // Run for 10 seconds
    for (int i = 0; i < 1000; i++) {
      log_state(f3, i * dt, &state3, &ref3);
      updateFlightControl(&state3, &ref3, &gains3, motor_dist,
                          thrust_coefficient, torque_coefficient, &ctrl3,
                          &cmds3);
      quadrotor_step_euler(&state3, ctrl3.total_thrust, &ctrl3.torque, dt);
    }
    fclose(f3);
  }

  // Test 4: Complex trajectory (X=2, Y=3, Z=8)
  FILE *f4 = fopen("test_hard.csv", "w");
  if (f4) {
    fprintf(f4, "time,x,y,z,roll,pitch,yaw,ref_x,ref_y,ref_z\n");
    QuadcopterState state4 = {0}; // Start at origin
    Reference ref4 = {.position = {2.0f, 3.0f, 10.0f}, .yaw = 0.0f};
    ControllerGains gains4;
    initializeController(&gains4);
    ControlOutputs ctrl4;
    MotorCommands cmds4;

    // Run for 15 seconds to allow settling
    for (int i = 0; i < 1500; i++) {
      log_state(f4, i * dt, &state4, &ref4);
      updateFlightControl(&state4, &ref4, &gains4, motor_dist,
                          thrust_coefficient, torque_coefficient, &ctrl4,
                          &cmds4);
      quadrotor_step_euler(&state4, ctrl4.total_thrust, &ctrl4.torque, dt);
    }
    fclose(f4);
  }

  // Test 5: Sequential waypoints (WP1→WP2→WP3→WP4)
  FILE *f5 = fopen("test_waypoints.csv", "w");
  if (f5) {
    fprintf(f5, "time,x,y,z,roll,pitch,yaw,ref_x,ref_y,ref_z\n");
    QuadcopterState state5 = {0}; // Start at origin
    ControllerGains gains5;
    initializeController(&gains5);
    ControlOutputs ctrl5;
    MotorCommands cmds5;

    // 4 waypoints to visit in sequence
    Reference waypoints[4] = {
        {.position = {0.0f, 0.0f, 3.0f}, .yaw = 0.0f}, // WP1: takeoff
        {.position = {3.0f, 0.0f, 3.0f}, .yaw = 0.0f}, // WP2: fly east
        {.position = {3.0f, 3.0f, 5.0f}, .yaw = 0.0f}, // WP3: fly north & climb
        {.position = {0.0f, 0.0f, 1.0f}, .yaw = 0.0f}, // WP4: return & descend
    };
    int num_waypoints = 4;
    int wp_idx = 0;

    // Run for 30 seconds max
    int total_steps = 3000;
    for (int i = 0; i < total_steps; i++) {
      Reference *ref5 = &waypoints[wp_idx];
      log_state(f5, i * dt, &state5, ref5);

      // Switch to next waypoint when within 0.3 m
      float ex = state5.position.x - ref5->position.x;
      float ey = state5.position.y - ref5->position.y;
      float ez = state5.position.z - ref5->position.z;
      float dist = sqrtf(ex * ex + ey * ey + ez * ez);
      if (dist < 0.3f && wp_idx < num_waypoints - 1) {
        wp_idx++;
      }

      updateFlightControl(&state5, ref5, &gains5, motor_dist,
                          thrust_coefficient, torque_coefficient, &ctrl5,
                          &cmds5);
      quadrotor_step_euler(&state5, ctrl5.total_thrust, &ctrl5.torque, dt);
    }
    fclose(f5);
  }

  printf("Tests completed. Data saved to test_hover.csv, test_roll.csv, "
         "test_climb.csv, test_hard.csv, test_waypoints.csv\n");
  return 0;
}
