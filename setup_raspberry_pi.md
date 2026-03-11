# Raspberry Pi 5 - Hardware Setup Guide

This guide covers setting up the Raspberry Pi 5 with Ubuntu 24.04 and ROS 2 Jazzy Jalisco, connecting your hardware (Pixhawk / CUAV X7+ via MAVROS), and running the Phase 1 motor tests.

## 1. Operating System Installation (Ubuntu 24.04)

1. Download the **Raspberry Pi Imager** on your main computer (`sudo apt install rpi-imager` on Ubuntu, or download from the Raspberry Pi website).
2. Insert a fast MicroSD card (U3/A2 class recommended, at least 32GB).
3. Open Raspberry Pi Imager:
   - **Choose Device**: Raspberry Pi 5
   - **Choose OS**: Other general-purpose OS > Ubuntu > **Ubuntu 24.04 LTS (64-bit)** (Server or Desktop, Server headless is recommended for drones).
   - **Choose Storage**: Select your SD card.
4. Click **Next** and click **Edit Settings**:
   - Set hostname to `pi5`.
   - Set username and password (e.g. `kunal` / `yourpassword`).
   - Configure Wireless LAN (your Wi-Fi SSID and password).
   - Enable SSH (Use password authentication).
5. Click **Save** and write the image.

## 2. Connect to the Pi and Initial Setup

Insert the SD card into the Pi 5, power it on, and wait ~2 minutes for the first boot.

On your main computer, SSH into the Pi:

```bash
ssh kunal@pi5.local
```

Update the system:

```bash
sudo apt update && sudo apt upgrade -y
sudo reboot
```

## 3. Clone Repository & Install ROS 2

SSH back into your Pi.

```bash
git clone https://github.com/YOUR_GITHUB_USERNAME/arena_drone_ws.git ~/arena_drone_ws
cd ~/arena_drone_ws
```

> **Note**: Replace the github URL above with your actual repository URL after you push this code.

Run the automated setup script. This script installs ROS 2 Jazzy (if missing), `mavros`, `behaviortree_cpp`, configures USB udev rules, and compiles the workspace.

```bash
chmod +x setup_workspace.sh
./setup_workspace.sh
```

## 4. Hardware Connection (Pixhawk / CUAV x7+)

1. Connect the flight controller via USB to the Raspberry Pi 5.
2. The `setup_workspace.sh` created udev rules, so your device should be mapped.
3. Verify the connection:

```bash
ls -l /dev/ttyfcu
```

_(If it doesn't show up, try unplugging and replugging the USB cable, or check `dmesg | grep tty`)._

## 5. Test the Motors

Warning: **Remove all propellers before testing motors!**
The motor test will arm the drone and spin each motor sequentially using the behavior tree. The drone will NOT take off.

```bash
cd ~/arena_drone_ws
chmod +x test_motors.sh
./test_motors.sh
```

## 6. Retrieving Logs

The `test_motors.sh` wrapper automatically records everything to a timestamped file. To retrieve it and share:

```bash
ls -l ~/drone_logs/
```

You can use `scp` from your laptop to download the log and share it for debugging.

```bash
scp kunal@pi5.local:~/drone_logs/motor_test_latest.log .
```
