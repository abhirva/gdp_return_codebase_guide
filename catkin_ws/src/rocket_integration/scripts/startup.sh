#!/bin/bash

# Source ROS and workspace setup
source /opt/ros/melodic/setup.bash
source /home/jetsonog/rocket-ros-ws/catkin_ws/devel/setup.bash

# Wait for network to be available (adjust timeout as needed)
MAX_RETRIES=30
RETRY_DELAY=2
retries=0

while [ $retries -lt $MAX_RETRIES ]; do
    if ping -c 1 192.168.1.100 &> /dev/null; then
        echo "Ground station at 192.168.1.100 is reachable, starting ROS nodes..."
        break
    fi
    echo "Waiting for ground station network... (attempt $retries of $MAX_RETRIES)"
    sleep $RETRY_DELAY
    retries=$((retries+1))
done

if [ $retries -eq $MAX_RETRIES ]; then
    echo "ERROR: Could not connect to ground station after $MAX_RETRIES attempts."
    exit 1
fi

# Launch the flight controller node
roslaunch rocket_integration jetson_fc.launch 