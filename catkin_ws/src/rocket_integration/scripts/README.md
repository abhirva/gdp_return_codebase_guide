# Scripts Documentation

This directory contains various utility scripts used in the rocket communication system. Below is a description of each script and its deployment location.

## Deployment Locations
- **Ground Station**: Scripts running on the ground control computer
- **Jetson**: Scripts running on the flight computer (NVIDIA Jetson)

## Script Descriptions

### udp_relay.py
**Location**: Ground Station
**Purpose**: Acts as a UDP relay server for forwarding telemetry data between the flight computer and ground station. Implements error handling and data validation.

### rocket-fc.service
**Location**: Jetson
**Purpose**: Systemd service file for managing the flight computer ROS nodes. Ensures automatic startup and recovery of critical flight systems.

### setup_cron.sh
**Location**: Jetson
**Purpose**: Configures cron jobs for periodic system checks and maintenance tasks. Sets up automated health monitoring and logging.

### setup_wifi.sh
**Location**: Jetson
**Purpose**: Configures WiFi network settings for the flight computer. Handles network interface setup and connection management.

### startup.sh
**Location**: Jetson
**Purpose**: Main startup script that initializes all necessary ROS nodes and services. Manages the boot sequence of the flight computer system.

### status_led.py
**Location**: Jetson
**Purpose**: Controls status LEDs to provide visual feedback about system state. Implements different patterns for various system conditions.

### watchdog.sh
**Location**: Jetson
**Purpose**: Monitors system health and restarts critical services if they fail. Implements a robust watchdog mechanism for system reliability.

## Usage Notes
- All scripts should be executed with appropriate permissions
- System-specific configurations may need to be adjusted based on the deployment environment
- Log files are maintained for debugging and monitoring purposes

## Security Considerations
- Network configuration scripts include security measures to prevent unauthorized access
- Service files implement proper user permissions and isolation
- All scripts include error handling and logging for security monitoring 