#!/bin/bash

# Set ROS environment variables explicitly
export ROS_MASTER_URI=http://192.168.1.100:11311
export ROS_IP=192.168.1.3

# Source ROS environment
source /opt/ros/melodic/setup.bash
source /home/jetsonog/rocket-ros-ws/catkin_ws/devel/setup.bash

# Log file location
LOG_FILE="/home/jetsonog/watchdog.log"

echo "$(date): Running watchdog check..." >> $LOG_FILE

# Check if roscore is reachable
if ! timeout 5 rostopic list > /dev/null 2>&1; then
    echo "$(date): ERROR: Unable to communicate with master!" >> $LOG_FILE
    echo "$(date): Will not attempt restart since service appears to be running." >> $LOG_FILE
    exit 1
fi

# Use a more reliable check for the node
NODE_RUNNING=$(rostopic list | grep -c "flight_controller_node")

if [ "$NODE_RUNNING" -eq 0 ]; then
    echo "$(date): Flight controller node not running, but service may be." >> $LOG_FILE
    # Instead of using sudo, we'll use systemctl directly
    systemctl --user restart rocket-fc.service >> $LOG_FILE 2>&1 || echo "$(date): Failed to restart service. Is it installed as a user service?" >> $LOG_FILE
else
    echo "$(date): Flight controller node is running properly." >> $LOG_FILE
fi 