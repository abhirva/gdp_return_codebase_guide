#!/usr/bin/env python

import Jetson.GPIO as GPIO
import time
import subprocess
import os

# Set the GPIO mode
GPIO.setmode(GPIO.BOARD)

# Define the GPIO pins for LEDs
NETWORK_LED = 11  # Pin 11 for network status
ROS_LED = 13      # Pin 13 for ROS node status

# Set up the pins as outputs
GPIO.setup(NETWORK_LED, GPIO.OUT)
GPIO.setup(ROS_LED, GPIO.OUT)

def check_network():
    """Check if ground station is reachable"""
    return subprocess.call(["ping", "-c", "1", "-W", "1", "192.168.1.100"]) == 0

def check_ros_node():
    """Check if the flight controller node is running"""
    try:
        output = subprocess.check_output(["rosnode", "list"]).decode('utf-8')
        return "flight_controller_node" in output
    except:
        return False

try:
    while True:
        # Check network
        if check_network():
            GPIO.output(NETWORK_LED, GPIO.HIGH)  # LED on
        else:
            GPIO.output(NETWORK_LED, GPIO.LOW)   # LED off
            
        # Check ROS node
        if check_ros_node():
            GPIO.output(ROS_LED, GPIO.HIGH)      # LED on
        else:
            GPIO.output(ROS_LED, GPIO.LOW)       # LED off
            
        time.sleep(1)  # Check every second
        
except KeyboardInterrupt:
    pass
finally:
    GPIO.cleanup()  # Clean up on exit 