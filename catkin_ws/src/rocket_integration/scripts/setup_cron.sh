#!/bin/bash

# Make watchdog executable
chmod +x ~/rocket-ros-ws/catkin_ws/src/rocket_integration/scripts/watchdog.sh

# First, remove any existing cron jobs for the watchdog
crontab -l | grep -v "watchdog.sh" | crontab -

# Add cron job to check every 5 minutes (without sudo)
(crontab -l 2>/dev/null; echo "*/5 * * * * ~/rocket-ros-ws/catkin_ws/src/rocket_integration/scripts/watchdog.sh") | crontab -

echo "Cron job set up to run watchdog every 5 minutes (without sudo)"
echo "Watchdog logs will be stored in ~/watchdog.log" 