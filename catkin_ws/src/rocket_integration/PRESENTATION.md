# Rocket Communication System - Final Presentation

## Presentation Outline

### 1. Introduction (1-2 minutes)
- **Project Overview**: Rocket system integration with ROS-based communication
- **Team Role**: Communication and software integration lead
- **Objective**: Demonstrate reliable bidirectional communication for rocket control

### 2. System Architecture (3-4 minutes)
- **Hardware Components**:
  - *Ground Station*: Ubuntu 18.04, ROS Melodic, [specific hardware specs]
  - *Onboard Computer*: Jetson Nano, [processing capabilities]
  - *Flight Controller*: Custom STM32, [mention controller designer]
  - *Vicon System*: Motion capture for precision positioning
  - *Vicon PC*: Running UDP relay script for motion data forwarding

- **Communication Diagram**:
  - Show visual diagram of data flow between components
  - Highlight key interfaces and protocols
  - Emphasize system integration points

### 3. Communication Protocol (3-4 minutes)
- **Protocols Used**:
  - ROS topics over WiFi (ground-to-Jetson)
  - Custom serial protocol over UART (Jetson-to-FC)
  - UDP (Vicon-to-ground station via relay)

- **Protocol Details**:
  - Command format: `CMD,param1,param2,...\n`
  - Telemetry format: `CTRL,val1,val2,...,val18\n`
  - Position format: `POSE,x,y,z,qx,qy,qz,qw\n`

- **Key Metrics**:
  - Baudrate: 115200 bps
  - Maximum frequency: 100 Hz (telemetry), 150 Hz (Vicon)
  - Typical latency: [measured values] ms

### 4. Robustness Features (3-4 minutes)
- **Command Reliability**:
  - 3x redundant command transmission
  - Command blocking during critical operations
  - Parameter validation before transmission

- **Error Handling**:
  - Serial port disconnection recovery
  - Data format validation
  - Timeout detection and handling

- **Monitoring Systems**:
  - End-to-end latency tracking
  - Signal quality monitoring
  - Comprehensive logging

### 5. Data Management (2-3 minutes)
- **Telemetry Data**:
  - 18 values from flight controller
  - Data categories: VICON, Euler, Estimator, IMU, Controller
  - CSV recording with timestamps

- **Visualization Tools**:
  - Custom RQT-based visualization
  - Single and multi-value viewing
  - Real-time and post-flight analysis

### 6. Vicon Integration Challenges (2-3 minutes)
- **SDK Compatibility Issues**:
  - ROS Melodic/Ubuntu 18.04 constraints
  - Boost library version conflicts
  - Solution: Custom UDP parsing implementation

- **Network Configuration**:
  - Vicon software IP limitations
  - Challenge: UDP stream targeting
  - Solution: Custom relay script on Vicon PC

- **UDP Relay Implementation**:
  - Python-based forwarding script
  - Reliable packet transmission
  - Maintaining 150Hz data rate

- **Demonstration**:
  - Show real-time position data flow
  - Display position accuracy and update rate

### 7. Live Demo (4-5 minutes)
- **Setup**:
  - Launch ground station: `roslaunch rocket_integration ground_system.launch`
  - Enable recording: `enable_recording:=true run_name:="demo_flight"`
  - Show visualization: `roslaunch rocket_integration view_fc_data.launch view_mode:=group data_type:=controller`

- **Live Operations**:
  - Demonstrate command sending (ARM, MODE selection)
  - Show telemetry reception and visualization
  - Create a simulated error and show recovery

- **Data Analysis**:
  - Quick analysis of recorded data
  - Show latency measurements

### 8. Technical Challenges & Solutions (2-3 minutes)
- **Challenge 1**: High-frequency data handling
  - *Solution*: Optimized serial reading with non-blocking I/O

- **Challenge 2**: Command reliability during high-speed operation
  - *Solution*: Command prioritization and blocking mechanisms

- **Challenge 3**: Integrating with custom STM32 firmware
  - *Solution*: Collaborative protocol design with controller team

- **Challenge 4**: Vicon integration with older ROS version
  - *Solution*: Custom UDP relay and parsing solution

### 9. Performance Validation (2-3 minutes)
- **Test Results**:
  - Commanded vs. actual latency measurements
  - Reliability statistics (packet loss rate)
  - Maximum sustained telemetry rate
  - Vicon position data accuracy and reliability

- **Key Metrics**:
  - Command latency: [x] ms (target: <50ms)
  - End-to-end latency: [x] ms (target: <100ms)
  - Telemetry reception rate: [x] Hz (target: 100Hz)
  - Vicon data rate: 150 Hz (consistent throughout operation)

### 10. Future Improvements (1-2 minutes)
- **Potential Enhancements**:
  - Integration with additional sensors
  - Improved error correction
  - Wireless telemetry backup
  - Binary protocol for reduced bandwidth

- **Scalability**:
  - Supporting additional telemetry values
  - Protocol extensibility for new commands
  - Higher frequency position updates

### 11. Conclusion (1 minute)
- **Summary of Achievements**:
  - Reliable bidirectional communication
  - Robust error handling
  - Comprehensive data recording
  - Flexible visualization
  - Successful integration of multiple subsystems

- **Acknowledgments**:
  - Thank controller designer for collaboration
  - Thank advisors/sponsors

## Presentation Tips

### Demonstration Preparation
1. Ensure all systems are powered and connected before presentation
2. Have backup recordings in case of live demo issues
3. Prepare specific commands to demonstrate during live demo
4. Verify Vicon PC relay script is running before demo
5. Consider a "stress test" to show robustness

### Key Numbers to Highlight
- Serial baudrate: 115200 bps
- Number of telemetry values: 18
- Command latency: [measure and include]
- Maximum telemetry frequency: [measure and include]
- End-to-end latency: [measure and include]
- Vicon capture rate: 150 Hz

### Visual Aids
- Architecture diagram (communication flow)
- Protocol format specifications
- Latency graphs
- Live telemetry visualization
- Network topology diagram showing UDP relay configuration

### Questions to Anticipate
1. How does the system handle communication failures?
2. What is the maximum range/distance for reliable operation?
3. How scalable is the protocol for additional telemetry data?
4. How was the protocol optimized for low latency?
5. What redundancy measures are in place?
6. How did you solve the Vicon integration challenges with older ROS?
7. What is the impact of the UDP relay on overall system latency?

### Demo Sequence
1. Launch ground station
2. Show initial telemetry reception
3. Demonstrate command sending
4. View real-time telemetry visualization
5. Show data recording and playback
6. Demonstrate Vicon position tracking accuracy 