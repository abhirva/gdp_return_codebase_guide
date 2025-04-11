# Rocket ROS Integration: System Architecture

## Overview

This repository contains the ROS-based communication and integration system for the rocket project. The system provides robust bidirectional communication between the ground station and the flight controller (FC), with a Jetson Nano serving as the onboard computer.

## System Architecture

### Hardware Components

1. **Ground Station**: Ubuntu 18.04 running ROS Melodic
   - Operates command center interface
   - Visualizes telemetry data
   - Records flight data

2. **Onboard Computer (Jetson Nano)**: Ubuntu 18.04 running ROS Melodic
   - Bridges ground station with flight controller
   - Processes Vicon position data
   - Handles telemetry data

3. **Flight Controller**: Custom STM32 board
   - Controls rocket actuators
   - Processes sensor data
   - Runs control algorithms
   - Communicates via serial UART

4. **Vicon Motion Capture System**:
   - Provides high-precision position tracking
   - Transmits UDP data packets at 150Hz
   - Connected to dedicated Vicon PC running Vicon Tracker software

### Communication Channels

The system uses a multi-layered communication approach:

```
                   +---------------+
                   | Vicon System  |
                   +-------+-------+
                           |
                           | UDP (150Hz)
                           v
+-------------+     +------+------+     +----------------+
| Vicon PC    +---->+ Ground      +---->+ Jetson Nano    +---->+ Flight       +
| (UDP Relay) |     | Station     |     | (Onboard)      |     | Controller   |
+-------------+     +-------------+     +----------------+     +--------------+
                     ^   |                ^   |
                     |   |                |   |
                     +---+ ROS/WiFi       +---+ Serial UART
                         | (100Hz)            | (115200 bps)
```

1. **Ground Station ↔ Jetson**: 
   - Protocol: ROS topics over WiFi
   - Data: Commands, telemetry, position information
   - Frequency: Up to 100Hz

2. **Jetson ↔ Flight Controller**:
   - Protocol: Custom serial protocol over UART
   - Baud Rate: 115200 bps
   - Data: Position, orientation, commands, telemetry

3. **Vicon System → Vicon PC → Ground Station**:
   - Protocol: UDP packets
   - Data: Position and orientation data
   - Frequency: 150Hz from Vicon system
   - Custom UDP relay script to forward packets to ground station

## Vicon Integration Details

### Challenges and Solutions

Due to the constraints of using older software versions (Ubuntu 18.04 and ROS Melodic), several challenges were encountered during Vicon integration:

1. **SDK Compatibility Issues**:
   - **Challenge**: Official Vicon ROS packages required newer libraries than available in ROS Melodic
   - **Solution**: Developed a custom UDP parsing node to directly receive and interpret Vicon data

2. **Network Configuration**:
   - **Challenge**: Vicon PC software only identified its own IP addresses for UDP streaming, despite being on the same network as the ground station
   - **Solution**: Implemented a lightweight UDP relay script on the Vicon PC that:
     - Captures UDP packets from the Vicon system
     - Forwards these packets to the ground station IP address
     - Preserves timing and data integrity

3. **Library Conflicts**:
   - **Challenge**: Boost library version mismatches between Vicon SDK requirements and ROS Melodic
   - **Solution**: Bypassed SDK entirely with direct UDP communication

### UDP Packet Structure

The Vicon system sends UDP packets containing:
- Subject ID (rocket identifier)
- Position data (X, Y, Z in millimeters)
- Rotation data (quaternion format: qx, qy, qz, qw)
- Frame information (timing data)

### UDP Relay Implementation

A Python-based relay script running on the Vicon PC:
1. Binds to the local UDP port receiving Vicon data
2. Parses incoming packets to verify data integrity
3. Forwards valid packets to the ground station IP
4. Provides basic logging for debugging purposes

### Processing Pipeline

1. Vicon system captures motion at 150Hz
2. UDP packets sent to Vicon PC
3. Relay script forwards packets to ground station
4. `vicon_udp_node` receives and parses the UDP data
5. Position data published as ROS messages (`/vicon/rocket/pose`)
6. Data forwarded to Jetson and ultimately to flight controller

## Data Flow

### Command Flow (Ground → FC)
1. User inputs command via command center
2. Command transmitted as ROS message (`/fc_command`)
3. Flight controller node receives command
4. Command formatted as `CMD,param1,param2,...\n`
5. Command sent to FC with redundancy (3 attempts)
6. FC acknowledges receipt

### Telemetry Flow (FC → Ground)
1. FC generates telemetry data
2. Data transmitted as `CTRL,val1,val2,...,val18\n`
3. Flight controller node parses data
4. Data published as ROS message (`/flight_controller_node/data`)
5. Ground station visualizes/records data

### Position Data Flow
1. Vicon captures position data
2. Data transmitted via UDP
3. Vicon node receives and processes data
4. Position published as ROS message (`/vicon/rocket/pose`)
5. Flight controller node formats position data
6. Position sent to FC as `POSE,x,y,z,qx,qy,qz,qw\n`

## Software Components

### ROS Nodes

1. **`flight_controller_node`** (fc_node.cpp)
   - Manages serial communication with FC
   - Converts between ROS messages and serial protocol
   - Handles command and telemetry processing

2. **`vicon_udp_node`**
   - Receives UDP packets from Vicon
   - Publishes position data as ROS messages
   - Calculates latency

3. **`command_center`**
   - Provides user interface for sending commands
   - Displays command status

4. **`latency_monitor`**
   - Tracks end-to-end latency in the system
   - Logs latency data for analysis

5. **`csv_recorder`**
   - Records telemetry data to CSV files
   - Handles file management and timestamping

### Launch Files

1. **`ground_system.launch`**
   - Main launch file for ground station
   - Starts all necessary nodes
   - Configures communication parameters

2. **`record_flight_data.launch`**
   - Dedicated recording functionality
   - Configurable output formats (CSV, ROS bag)

3. **`view_fc_data.launch`**
   - Visualizes telemetry data
   - Multiple viewing modes (single, group, custom)

## Telemetry Data Format

The flight controller transmits 18 values in the following format:

```
CTRL,val1,val2,...,val18\n
```

These values represent:
1. **VICON Position** (0-2): X, Y, Z coordinates from Vicon
2. **Euler Angles** (3-5): Roll, Pitch, Yaw from Vicon quaternions
3. **Estimator Position** (6-8): X, Y, Z from onboard estimator
4. **Estimator Velocity** (9-11): Vx, Vy, Vz from onboard estimator
5. **IMU Rate** (12-14): Angular rates around X, Y, Z axes
6. **Controller Values** (15-17): Thrust (N), Delta Y, Delta Z

## Command Format

Commands are sent to the flight controller in the following format:

```
CMD,param1,param2,...\n
```

Common commands include:
- `ARM`: Arm the rocket
- `DISARM`: Disarm the rocket
- `MODE,mode_number`: Set the control mode
- `START`: Start the flight sequence
- `ABORT`: Abort the flight sequence

## Robustness Features

The system incorporates several features to ensure robust operation:

1. **Command Redundancy**:
   - Commands sent multiple times (3x)
   - Delays between retries
   - Blocking pose updates during command transmission

2. **Error Handling**:
   - Telemetry validation
   - Serial communication error detection
   - Graceful degradation on connection loss

3. **Data Validation**:
   - Format checking for all incoming data
   - Bounds checking for critical values
   - Timeout detection for stale data

4. **Latency Monitoring**:
   - End-to-end latency tracking
   - Performance optimization based on latency data
   - Early warning of communication issues

5. **Data Recording**:
   - Multiple backup formats (CSV, ROS bag)
   - Automatic timestamping
   - Error-resistant file handling

## Setup and Configuration

### Ground Station Setup

1. Clone this repository on the ground station
2. Install ROS Melodic on Ubuntu 18.04
3. Install dependencies:
   ```bash
   sudo apt install ros-melodic-rqt-plot python-catkin-tools
   ```
4. Build the workspace:
   ```bash
   cd rocket-ros-ws
   catkin_make
   source devel/setup.bash
   ```
5. Configure network (update `ROS_IP` in launch files)

### Jetson Nano Setup

1. Install Ubuntu 18.04 and ROS Melodic
2. Clone this repository
3. Build the workspace
4. Configure serial port permissions:
   ```bash
   sudo usermod -a -G dialout $USER
   ```
5. Set up automatic launch on boot (optional)

## Usage Instructions

### Starting the Ground Station

```bash
# Basic operation
roslaunch rocket_integration ground_system.launch

# With data recording enabled
roslaunch rocket_integration ground_system.launch enable_recording:=true run_name:="flight1"
```

### Viewing Telemetry Data

```bash
# View a single value (best for scaling)
roslaunch rocket_integration view_fc_data.launch view_mode:=single index:=15

# View a group of related values
roslaunch rocket_integration view_fc_data.launch view_mode:=group data_type:=controller

# View custom selection of values
roslaunch rocket_integration view_fc_data.launch view_mode:=custom index1:=0 index2:=6 index3:=9 index4:=15
```

### Recording Flight Data

```bash
# Basic recording
roslaunch rocket_integration record_flight_data.launch run_name:="test_flight_1"

# With ROS bag recording
roslaunch rocket_integration record_flight_data.launch run_name:="test_flight_1" record_rosbag:=true

# Detailed output
roslaunch rocket_integration record_flight_data.launch run_name:="test_flight_1" verbose:=true
```

## Performance Parameters

The system is designed to operate within the following parameters:

- **Maximum Telemetry Rate**: 100 Hz
- **Command Latency**: < 50 ms
- **Position Update Rate**: 150 Hz from Vicon, 100 Hz to FC
- **Serial Baud Rate**: 115200 bps
- **End-to-end Latency**: < 100 ms (typical)

## Extending the System

To extend this system with new functionality:

1. **Add New Commands**:
   - Update the command processing in `fc_node.cpp`
   - Add command validation logic
   - Implement corresponding functionality in the FC firmware

2. **Add New Telemetry Data**:
   - Update the telemetry parsing in `fc_node.cpp`
   - Update the CSV headers in `csv_recorder.cpp`
   - Update visualization options in `view_fc_data.launch`

3. **Integrate New Sensors**:
   - Create new ROS nodes for sensor interfacing
   - Publish sensor data as ROS messages
   - Subscribe to these messages in the flight controller node

## Troubleshooting

Common issues and solutions:

1. **Serial Communication Errors**:
   - Check port permissions
   - Verify baud rate matches FC configuration
   - Inspect physical connections

2. **Data Visualization Issues**:
   - Check that FC is sending data in expected format
   - Verify ROS topics are being published
   - Check for syntax errors in launch files

3. **Latency Problems**:
   - Monitor CPU usage on Jetson
   - Check WiFi signal strength
   - Verify Vicon data quality

4. **Recording Failures**:
   - Check disk space
   - Verify write permissions
   - Check for file path errors

5. **Vicon Data Issues**:
   - Check UDP relay script is running on Vicon PC
   - Verify network connectivity between Vicon PC and ground station
   - Confirm correct IP addresses are configured in relay script
   - Check Vicon tracking quality and marker visibility

## Contributors

- **Abhirva Navalakhe** - Communication and Software Integration Lead
  - Led comprehensive research and component selection process, including Jetson Nano as onboard computer
  - Collaborated with Rohit on power distribution system design and component selection
  - Designed and implemented the complete local network infrastructure
  - Selected and justified Ubuntu 18.04 and ROS Melodic as the optimal software stack
  - Set up the ground station system from scratch, including OS, ROS, and development environment
  - Designed and implemented the complete communication architecture
  - Developed custom protocols for command, telemetry, and position data transmission
  - Created robust error handling and redundancy features for reliable communication
  - Led the complete setup and configuration of Jetson Nano from Ubuntu installation to ROS deployment
  - Solved complex Vicon integration challenges with custom UDP solutions
  - Collaborated with Maria and Han for STM32 flight controller integration
  - Implemented comprehensive data recording and visualization tools
  - Led system integration and testing efforts
  - Created detailed technical documentation for future development

- [Controller Designer] - STM32 Flight Controller

## License

[Your License Information]