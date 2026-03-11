#!/bin/bash
# =============================================================
# Arena Drone Motor Test Wrapper
# Launches the BT motor test and tees output to a log file
# =============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="$HOME/drone_logs"

mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/motor_test_$TIMESTAMP.log"

echo "==============================================="
echo "  ARENA DRONE MOTOR TEST"
echo "  WARNING: REMOVE ALL PROPELLERS NOW!"
echo "==============================================="
echo "Logging to: $LOG_FILE"
echo ""
echo "Starting in 5 seconds... Press Ctrl+C to abort."
sleep 5

# Source workspace
source /opt/ros/jazzy/setup.bash || true
source "$SCRIPT_DIR/install/setup.bash" || true

# Run the launch file and tee the output
# (stdbuf is used to disable output buffering so we see logs in realtime)
stdbuf -oL -eL ros2 launch drone_behavior_tree motor_test.launch.py 2>&1 | tee "$LOG_FILE"

# Create a symlink to the latest log for easy access
ln -sf "$LOG_FILE" "$LOG_DIR/motor_test_latest.log"

echo "==============================================="
echo "Motor test finished."
echo "Log saved at: $LOG_FILE"
