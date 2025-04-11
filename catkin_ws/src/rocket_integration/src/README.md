# Source Files Documentation

This directory contains the core implementation files for the rocket communication system. Each file represents a specific ROS node or component of the system.

## Deployment Locations
- **Ground Station**: Nodes running on the ground control computer
- **Jetson**: Nodes running on the flight computer
- **Both**: Nodes that can run on either system

## Source File Descriptions


### vicon_data_recorder.cpp
**Location**: Ground Station
**Purpose**: Records VICON system data to files for post-processing and analysis. Implements data formatting and storage mechanisms.


### vicon_udp_node.cpp
**Location**: Ground Station
**Purpose**: Handles UDP communication for VICON data transmission. Implements data packaging and network protocols.

### csv_recorder.cpp
**Location**: Ground Station
**Purpose**: General-purpose CSV data recording system. Handles formatting and storage of various system data types.

### echo_data_reference.cpp
**Location**: Both
**Purpose**: Implements data reference echoing for system testing and validation. Useful for debugging communication pipelines.

### fc_node.cpp
**Location**: Jetson
**Purpose**: Main flight computer node implementation. Handles core flight operations, telemetry processing, and command execution.

### latency_monitor.cpp
**Location**: Both
**Purpose**: Monitors and analyzes communication latency between systems. Provides real-time latency metrics and alerts.

### signal_monitor.cpp
**Location**: Both
**Purpose**: Implements signal strength and quality monitoring. Provides real-time feedback on communication link status.

### command_center.cpp
**Location**: Ground Station
**Purpose**: Implements the ground station command center. Handles command processing, validation, and transmission to the flight computer.

## Implementation Details
- All nodes follow ROS 2 design patterns and best practices
- Error handling and recovery mechanisms are implemented throughout
- Real-time performance considerations are taken into account
- Security measures are implemented for critical operations

## Dependencies
- ROS 2 (specific version requirements in package.xml)
- System-specific libraries and drivers
- Network communication libraries
- Data processing and analysis tools

## Build and Deployment
- Follow the build instructions in the main README
- Ensure all dependencies are properly installed
- Verify system-specific configurations before deployment
- Test each component thoroughly before integration 