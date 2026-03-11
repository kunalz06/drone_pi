#!/bin/bash
# =============================================================
# Arena Drone Workspace Setup
# Run on Raspberry Pi 5 (Ubuntu 24.04) from the workspace root
# =============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS_DIR="$SCRIPT_DIR"

echo "=== Arena Drone ROS 2 Jazzy Workspace Setup ==="
echo "Workspace: $WS_DIR"

# 1. Check if ROS 2 Jazzy is already installed
if ! command -v ros2 &> /dev/null; then
    echo "ROS 2 not found. Installing ROS 2 Jazzy..."
    sudo apt update && sudo apt install locales
    sudo locale-gen en_US en_US.UTF-8
    sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
    export LANG=en_US.UTF-8

    sudo apt install software-properties-common -y
    sudo add-apt-repository universe -y

    sudo apt update && sudo apt install curl -y
    sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg

    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

    sudo apt update
    sudo apt install ros-jazzy-ros-base python3-argcomplete -y
else
    echo "ROS 2 is already installed. Skipping base installation."
fi

# 2. Install workspace dependencies
echo "Installing ROS 2 dependencies (mavros, behaviortree_cpp)..."
sudo apt update
sudo apt install -y \
    ros-jazzy-mavros \
    ros-jazzy-mavros-extras \
    ros-jazzy-behaviortree-cpp \
    ros-jazzy-ament-cmake \
    python3-colcon-common-extensions \
    build-essential \
    git \
    udev

# 3. Install MAVROS geographiclib datasets (required for GPS)
echo "Installing MAVROS geographiclib datasets..."
sudo /opt/ros/jazzy/lib/mavros/install_geographiclib_datasets.sh || true

# 4. Copy USB rules (Pixhawk, Lidar, Cameras, TFmini)
echo "Setting up udev rules..."
# Create a dummy rules file if the bringup package doesn't exist yet, to ensure the setup script completes
sudo tee /etc/udev/rules.d/99-drone-usb.rules > /dev/null << 'EOF'
KERNEL=="ttyACM*", ATTRS{idVendor}=="26ac", ATTRS{idProduct}=="0011", SYMLINK+="ttyfcu", MODE="0666"
KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", SYMLINK+="ttyrplidar", MODE="0666"
KERNEL=="ttyUSB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="ttytfmini", MODE="0666"
KERNEL=="video*", SUBSYSTEM=="video4linux", ATTR{name}=="*Arducam*", SYMLINK+="arducam%n", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger

# Add user to dialout group (needed for serial ports)
sudo usermod -a -G dialout "$USER"

# 5. Build workspace
echo "Building ROS 2 workspace..."
cd "$WS_DIR"
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install

# 6. Setup auto-sourcing
if ! grep -q "source $WS_DIR/install/setup.bash" ~/.bashrc; then
    echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
    echo "source $WS_DIR/install/setup.bash" >> ~/.bashrc
    echo "Added workspace sourcing to ~/.bashrc"
fi

echo ""
echo "=== Setup Complete ==="
echo "If this is your first time, please reboot your Raspberry Pi 5"
echo "Then test the motors with: ./test_motors.sh"
