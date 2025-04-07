# Real-Time-Flight-Control-With-Linux
# Quadcopter Flight Control System

## Overview

This code implements a basic flight control system for a quadcopter, focusing on position and attitude control. The system includes controllers for position, attitude, and yaw, and allocates control forces and torques to each motor based on the current state of the quadcopter and the desired reference position and attitude.

## Features

*   **Position Control:** Uses a Proportional-Derivative (PD) controller to calculate the desired force for each axis based on the position and velocity error.
*   **Attitude Control:** Uses a PD controller to calculate the desired torque for roll, pitch, and yaw based on the attitude and angular velocity error.
*   **Control Allocation:** Maps the calculated desired total thrust and torques to individual motor commands based on the quadcopter's physical design (motor distance, thrust/torque coefficients).
*   **Flight Control Loop:** The main loop updates the state of the quadcopter, calculates control outputs (forces, torques, motor commands), and simulates the next time step.

## Code Structure

The code utilizes several structures to organize data:

*   `Vector3`: Represents a 3D vector (used for position, velocity, force, etc.).
*   `Attitude`: Represents the roll, pitch, and yaw angles of the quadcopter (in radians).
*   `AngularVelocity`: Represents the rates of change of roll, pitch, and yaw (in radians per second).
*   `MotorCommands`: Represents the speeds of the four motors (in radians per second).
*   `ControllerGains`: Contains the proportional (Kp) and derivative (Kd) gains for the position, attitude, and yaw controllers.
*   `QuadcopterState`: Holds the complete current state of the quadcopter, including position, velocity, attitude, angular velocity, and motor speeds.
*   `Reference`: Holds the desired target position and yaw for the quadcopter.
*   `ControlOutputs`: Contains the intermediate and final control outputs, including desired forces, desired torques, and the final motor commands.

## Key Functions

*   `initializeController()`: Initializes the `ControllerGains` structure with default gain values.
*   `positionController()`: Calculates the desired net force vector based on position and velocity errors relative to the `Reference`.
*   `controlAllocator1()`: Converts the horizontal components of the desired force into desired roll and pitch angles. It also passes through the vertical force component as the total desired thrust.
*   `attitudeController()`: Calculates the desired torque vector (roll, pitch, yaw moments) based on the attitude and angular velocity errors relative to the desired attitude (from `controlAllocator1`) and the reference yaw.
*   `controlAllocator2()`: Takes the total desired thrust and the desired torque vector and calculates the required speed for each of the four motors.
*   `updateFlightControl()`: Acts as the main integration point for one control cycle. It calls the controllers and allocators in sequence to compute the final `MotorCommands` based on the current `QuadcopterState` and `Reference`.
*   `updateCurrentState()`: Simulates the update of the quadcopter's state based on new sensor inputs or simulation results (in this basic example, it might just update based on previous commands or dummy data).

## Usage

The program simulates the flight control process for a quadcopter.

1.  A `Reference` position and yaw are defined.
2.  The `QuadcopterState` is initialized or updated.
3.  The `updateFlightControl` function is called periodically.
4.  Inside `updateFlightControl`:
    *   The `positionController` calculates the necessary force.
    *   `controlAllocator1` determines the desired attitude and total thrust.
    *   The `attitudeController` calculates the necessary torques.
    *   `controlAllocator2` computes the individual `MotorCommands`.
5.  These motor commands would typically be sent to the motors (or used in a physics simulation to calculate the next state).
6.  The main loop simulates this control process over time and prints key control outputs.

### Example Output

```yaml
Control Outputs:
Position Error: [1.00, 1.00, 1.00] m
Force: [2.50, 2.50, 9.81] N
Desired Attitude: [0.20, -0.20, 0.00] rad
Total Thrust: 5.00 N
Torque: [0.10, -0.05, 0.02] Nm
Motor Commands: [1.50, 1.50, 1.50, 1.50] rad/s
--------------------------------------------------
Woke up after 1.5 seconds
Use code with caution.
Markdown
(Note: The specific values depend on the initial state, reference, gains, and simulation time step).
```

## Setup
To compile and run the provided C code:

Prerequisites: Ensure you have a C compiler installed (e.g., GCC on Linux/macOS, MinGW or MSVC on Windows).

Save: Save the code to a file named quad_control.c.

Compile: Open a terminal or command prompt, navigate to the directory where you saved the file, and compile using:

gcc -o quad_control quad_control.c -lm
Use code with caution.
Bash
The -lm flag links the math library.

Run: Execute the compiled program:

./quad_control
Use code with caution.
Bash
(On Windows, you might run quad_control.exe)

Dependencies
- C Compiler: GCC, Clang, MSVC, or compatible.

- Standard C Library: Included with the compiler.

- Math Library (math.h): Required for functions like sqrtf, asinf, etc. Needs to be linked during compilation using the -lm flag with GCC/Clang.

## Notes
- Gain Tuning: The controller gains (Kp, Kd values) provided in initializeController are likely default or example values. For stable and performant flight on a real quadcopter (or a more accurate simulation), these gains must be carefully tuned based on the specific hardware characteristics (mass, inertia, motor response, etc.).

- Simulation: The time simulation in the example uses nanosleep for basic periodic execution. This is a simplified approach and does not represent a full physics simulation of quadcopter dynamics.

- Units: Ensure consistency in units (e.g., meters, seconds, kilograms, radians) throughout the calculations.