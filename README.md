# Quadcopter Flight Control System

## Overview

This repository implements a real-time flight control system for quadcopters on Linux. The system focuses on precise position and attitude control through a series of interconnected controllers that translate desired positions into motor commands. This implementation prioritizes stability and control accuracy in a real-time environment.

## Features

* **Position Control:** PD controller calculates desired force vectors based on position and velocity errors
* **Attitude Control:** PD controller determines torque requirements for roll, pitch, and yaw based on attitude and angular velocity errors
* **Control Allocation:** Maps calculated thrust and torques to individual motor commands based on quadcopter geometry
* **Real-Time Processing:** Optimized for Linux systems with precise timing control
* **Configurable Gains:** Adjustable PID parameters for tuning control response
* **Simulation Mode:** Test flight algorithms before deployment to hardware

## System Architecture

The system is organized using the following data structures:

| Structure | Purpose |
|-----------|---------|
| `Vector3` | Represents 3D vectors for position, velocity, force, etc. |
| `Attitude` | Stores roll, pitch, and yaw angles (radians) |
| `AngularVelocity` | Tracks rates of change for roll, pitch, and yaw (rad/s) |
| `MotorCommands` | Contains speed commands for all four motors (rad/s) |
| `ControllerGains` | Holds PD gains for position, attitude, and yaw controllers |
| `QuadcopterState` | Maintains the complete state of the quadcopter |
| `Reference` | Stores target position and yaw orientation |
| `ControlOutputs` | Contains intermediate and final control calculations |

## Control Flow

The control system operates in a continuous loop:

1. State acquisition (from sensors or simulation)
2. Position controller calculates required forces
3. Control allocator 1 converts forces to desired attitude and thrust
4. Attitude controller determines required torques
5. Control allocator 2 maps thrust and torques to motor commands
6. Motor commands are applied to actuators
7. System state is updated

[Diagram_control.drawio (5).pdf](https://github.com/user-attachments/files/19633628/Diagram_control.drawio.5.pdf)
![Screenshot 2025-04-07 at 16 10 55](https://github.com/user-attachments/assets/2628333f-567b-4c15-91fd-aa5f5ca0aa0a)


## Key Functions

* `initializeController()`: Sets up controller gains with default or configured values
* `positionController()`: Calculates force vector based on position/velocity errors
* `controlAllocator1()`: Converts horizontal forces into roll/pitch angles and calculates thrust
* `attitudeController()`: Determines torques based on attitude and angular velocity errors
* `controlAllocator2()`: Maps thrust and torques to individual motor speeds
* `updateFlightControl()`: Orchestrates the complete control cycle
* `updateCurrentState()`: Updates quadcopter state from sensors or simulation

## Installation

### Prerequisites

* C Compiler (GCC/Clang)
* Linux operating system (recommended)
* Math library (libm)
* Real-time extensions (optional, for improved timing precision)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/Real-Time-Flight-Control-With-Linux.git
cd Real-Time-Flight-Control-With-Linux

# Build the project
gcc -o quad_control quad_control.c -lm

# Run the controller
./quad_control
```

## Configuration and Tuning

The controller gains in `initializeController()` must be tuned for your specific quadcopter. The default values are:

```c
// Position controller gains
controller.position.Kp = 2.5f;
controller.position.Kd = 1.5f;

// Attitude controller gains
controller.attitude.Kp = 5.0f;
controller.attitude.Kd = 1.0f;

// Yaw controller gains
controller.yaw.Kp = 2.0f;
controller.yaw.Kd = 0.5f;
```

Adjust these values based on your quadcopter's mass, inertia, and motor characteristics. Too high gains may cause oscillations, while too low gains will result in sluggish response.

## Example Output

When running properly, the system outputs control information at each step:

```
Control Outputs:
Position Error: [1.00, 1.00, 1.00] m
Force: [2.50, 2.50, 9.81] N
Desired Attitude: [0.20, -0.20, 0.00] rad
Total Thrust: 5.00 N
Torque: [0.10, -0.05, 0.02] Nm
Motor Commands: [1.50, 1.50, 1.50, 1.50] rad/s
--------------------------------------------------
```

## Performance Considerations

* Timing: The system uses `nanosleep()` for periodic execution, which is suitable for basic testing but may require real-time extensions for production use
* Processing: Optimize math operations when deploying to resource-constrained systems
* Sampling: Ensure sensor sampling rates are appropriate for control loop frequency

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

* Special thanks to contributors and the open-source flight control community
* Research papers and resources that inspired this implementation
