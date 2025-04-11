#!/bin/bash

# Script to configure the Rocket WiFi network for auto-connection

echo "Checking current WiFi connections..."
nmcli con show | grep RocketNetwork_5G

# Enable auto-connect for the Rocket network
echo "Setting RocketNetwork_5G to auto-connect..."
sudo nmcli con modify "RocketNetwork_5G" connection.autoconnect yes

# Verify the settings
echo "Verifying auto-connect settings..."
nmcli con show "RocketNetwork_5G" | grep autoconnect

echo "WiFi configuration complete. The Jetson will now automatically connect to RocketNetwork_5G on boot."
echo "Testing connection by reconnecting..."

# Test by reconnecting to ensure it works
sudo nmcli con down "RocketNetwork_5G" && sudo nmcli con up "RocketNetwork_5G"

# Ping ground station to check connectivity
echo "Checking connectivity to ground station..."
ping -c 3 192.168.1.100 