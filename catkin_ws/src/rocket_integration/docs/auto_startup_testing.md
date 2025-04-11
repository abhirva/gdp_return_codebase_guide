# Auto-Startup Testing Procedure

This document outlines the steps to verify that the Jetson's auto-startup functionality is working properly.

## Prerequisites
- Jetson is fully set up with all scripts and services installed
- Ground station computer is available and configured

## Testing Process

### 1. Full System Test
1. Power off the Jetson completely
2. Ensure the ground station is running and network is set up
3. Power on the Jetson
4. Wait approximately 1-2 minutes for full boot
5. On the ground station, run:
   ```
   rostopic list | grep flight_controller
   ```
6. Verify that the flight controller's topics are listed, confirming the node is running

### 2. Network Resilience Test
1. With the Jetson running, disconnect the network (turn off WiFi or router)
2. Wait 1 minute
3. Reconnect the network
4. Wait up to 5 minutes for auto-recovery
5. Verify the flight controller node is running again

### 3. Service Status Check
On the Jetson, you can check the service status with:
```
sudo systemctl status rocket-fc.service
```

### 4. Log Inspection
Check the startup logs for any issues:
```
journalctl -u rocket-fc.service
```

## Troubleshooting

### If the service doesn't start:
1. Check logs: `journalctl -u rocket-fc.service -n 50`
2. Verify the startup script permissions: `ls -l ~/rocket-ros-ws/catkin_ws/src/rocket_integration/scripts/startup.sh`
3. Try running the startup script manually: `~/rocket-ros-ws/catkin_ws/src/rocket_integration/scripts/startup.sh`

### If network connection fails:
1. Check WiFi configuration: `nmcli con show`
2. Verify "RocketNetwork_5G" is set to auto-connect: `nmcli con show "RocketNetwork_5G" | grep autoconnect`
3. Verify the ground station IP is correct in startup.sh
4. Test basic connectivity: `ping 192.168.1.100`

### If watchdog shows permission errors:
1. Check watchdog logs: `cat ~/watchdog.log`
2. If you see "sudo: no tty present" errors, update and reinstall the watchdog:
   ```
   # Update the cron job to not use sudo
   ~/rocket-ros-ws/catkin_ws/src/rocket_integration/scripts/setup_cron.sh
   ```
3. If you see "ERROR: Unable to communicate with master!" messages:
   ```
   # Restart the service
   sudo systemctl restart rocket-fc.service
   
   # Wait a few minutes for the watchdog to run again
   ``` 