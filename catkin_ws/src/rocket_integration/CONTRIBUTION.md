5. Abhirva Navalakhe

5.1. Communication and System Architecture
- Conducted extensive research on embedded computing options and selected Jetson Nano as optimal onboard computer based on:
  * Processing power for real-time control
  * ROS compatibility
  * Power efficiency
  * Cost-effectiveness
  * Community support
- Designed and implemented complete communication architecture from scratch:
  * Multi-layered communication system between ground station, Jetson, and flight controller
  * Custom protocols for command, telemetry, and VICON data transmission
  * Robust error handling and redundancy features
  * Network topology design and implementation
  * Security measures and monitoring systems
- Solved complex Vicon integration challenges:
  * Developed custom UDP parsing node due to SDK compatibility issues
  * Created UDP relay system for data forwarding
  * Implemented data validation and processing
  * Optimized network performance for 150Hz data rate
- Selected and justified Ubuntu 18.04 and ROS Melodic as optimal software stack:
  * Evaluated compatibility requirements
  * Analyzed package availability
  * Considered long-term maintenance
  * Assessed community support

5.2. System Integration and Development
- Designed initial power distribution framework and collaborated with Rohit on component selection:
  * Created initial power system architecture
  * Defined power requirements and specifications
  * Collaborated on component selection and optimization
  * Handed over detailed implementation to Rohit
- Set up complete system infrastructure:
  * Ground station setup from scratch (OS, ROS, development tools)
  * Jetson Nano configuration (Ubuntu, ROS, network, serial ports)
  * Local network architecture and security
  * Development environment and build systems
- Collaborated with Maria and Han for STM32 flight controller integration:
  * Developed communication protocols
  * Implemented command and telemetry formats
  * Created testing procedures
  * Validated system performance
- Implemented comprehensive data management:
  * Developed data recording tools for flight analysis
  * Created visualization systems for real-time monitoring
  * Designed data validation and processing pipelines
  * Implemented error detection and recovery systems
- Led system integration and testing:
  * Developed integration test procedures
  * Created performance benchmarking tools
  * Implemented system validation protocols
  * Documented testing results and improvements
- Created detailed technical documentation:
  * System architecture and communication protocols
  * Setup and configuration guides
  * Troubleshooting procedures
  * Performance analysis reports
  * Future development guidelines

5.3. Flight Testing and Safety
- Served as ground station operator during flight tests:
  * Monitored real-time telemetry data
  * Operated command and control systems
  * Managed data recording and visualization
  * Coordinated with flight team members
- Collaborated with Jeff, Maria, and Rohit on Security Tether System (STS):
  * Participated in design and implementation
  * Assisted in safety protocol development
  * Contributed to pre-flight checks
  * Supported on-field operations
- Collaborated with Jeff on mass budget analysis:
  * Analyzed component weights
  * Evaluated system balance
  * Contributed to optimization strategies
- Assisted Charlie with 3D printing issues:
  * Provided technical support
  * Helped with troubleshooting
  * Contributed to quality control 