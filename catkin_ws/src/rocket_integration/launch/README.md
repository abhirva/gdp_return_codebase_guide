# Launch Files Documentation

This directory contains ROS launch files used to start and configure various components of the rocket communication system. Each launch file is designed for specific deployment scenarios and system configurations.

## Deployment Locations
- **Ground Station**: Launch files for ground control systems
- **Jetson**: Launch files for flight computer systems
- **Both**: Launch files that can be used on either system

## Launch File Descriptions

### jetson_fc.launch
**Location**: Jetson
**Purpose**: Primary launch file for the flight computer system. Initializes all necessary nodes for flight operations including telemetry, control, and monitoring systems.

### jetson_fc_test.launch
**Location**: Jetson
**Purpose**: Test configuration for the flight computer system. Includes additional debugging nodes and reduced safety constraints for testing purposes.

### latency_monitoring.launch
**Location**: Both
**Purpose**: Launches the latency monitoring system to measure and analyze communication delays between ground station and flight computer.

### record_flight_data.launch
**Location**: Ground Station
**Purpose**: Configures data recording for flight telemetry and system states. Sets up bag files and CSV recording for post-flight analysis.

### signal_monitor.launch
**Location**: Both
**Purpose**: Launches the signal strength and quality monitoring system. Provides real-time feedback on communication link status.

### signal_visualizer.launch
**Location**: Ground Station
**Purpose**: Starts the visualization tools for monitoring signal strength and communication quality in real-time.

### vicon_recorder.launch
**Location**: Ground Station
**Purpose**: Configures the VICON motion capture system data recording. Handles position and orientation data logging.

### view_fc_data.launch
**Location**: Ground Station
**Purpose**: Launches the flight computer data visualization system. Provides real-time monitoring of flight computer states and telemetry.

### view_telemetry.launch
**Location**: Ground Station
**Purpose**: Starts the telemetry data visualization system. Displays real-time flight data and system status.

### ground_system.launch
**Location**: Ground Station
**Purpose**: Main launch file for the ground control system. Initializes all ground station components including telemetry reception, command transmission, and monitoring systems.

## Usage Guidelines
1. Always verify the correct launch file for your intended operation
2. Check system requirements before launching
3. Monitor system resources during operation
4. Follow safety protocols when using test configurations

## Configuration Notes
- Launch files include parameter configurations for different operating modes
- Environment variables may need to be set before launching
- Network configurations are specified in the launch files
- Logging and debugging options can be enabled/disabled as needed 