# drone_controller/arm_and_spin_node.py
import rclpy
from rclpy.node import Node
from mavros_msgs.srv import CommandBool, SetMode
from mavros_msgs.msg import State, OverrideRCIn
import time

class ArmAndSpinNode(Node):
    def __init__(self):
        super().__init__('arm_and_spin_node')
        self.current_state = State()

        # Quality of Service (QoS) for MAVROS topics
        # Reliable, Volatile, Depth 10 is standard for MAVROS state
        from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy, HistoryPolicy
        qos_state = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.VOLATILE,
            history=HistoryPolicy.KEEP_LAST,
            depth=10
        )

        self.state_sub = self.create_subscription(
            State, '/mavros/state', self.state_cb, qos_profile=qos_state)

        # Standard QoS for commands
        self.rc_override_pub = self.create_publisher(
            OverrideRCIn, '/mavros/rc/override', 10)

        self.arm_client = self.create_client(
            CommandBool, '/mavros/cmd/arming')
        self.mode_client = self.create_client(
            SetMode, '/mavros/set_mode')

        while not self.arm_client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info('Waiting for arming service...')
        
        self.get_logger().info('Arming service available.')

    def state_cb(self, msg):
        self.current_state = msg

    def set_mode(self, mode):
        req = SetMode.Request()
        req.custom_mode = mode
        self.get_logger().info(f'Sending request to change mode to {mode}...')
        future = self.mode_client.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        
        result = future.result()
        if result is not None:
            self.get_logger().info(f'Mode set to {mode}: {result.mode_sent}')
            return result.mode_sent
        else:
            self.get_logger().error(f'Service call failed for mode change to {mode}')
            return False

    def arm(self, state: bool):
        req = CommandBool.Request()
        req.value = state
        action = 'ARM' if state else 'DISARM'
        self.get_logger().info(f'Sending {action} request...')
        future = self.arm_client.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        
        result = future.result()
        if result is not None:
            status = 'ARMED' if state else 'DISARMED'
            self.get_logger().info(f'Drone {status} status: {result.success}, result code: {result.result}')
            return result.success
        else:
            self.get_logger().error(f'Service call failed for {action}')
            return False

    def send_throttle(self, throttle_percent):
        # RC Channel 3 = Throttle (Indices are 0-based, so index 2)
        # 1000µs = 0%, 2000µs = 100%
        # Safe bench test range: 10-20% only
        pwm = int(1000 + (throttle_percent / 100.0) * 1000)
        
        # Hard clamp for safety on bench: 1000 to 1200µs
        pwm = max(1000, min(1200, pwm)) 

        msg = OverrideRCIn()
        # Initialize channels with 0 (No override) except throttle
        # OverrideRCIn.channels is an array of 18 uint16
        msg.channels = [65535] * 18 # 65535 means 'ignore' for MAVROS OverrideRCIn
        msg.channels[2] = pwm       # Channel 3 is throttle
        
        self.rc_override_pub.publish(msg)

    def stop_all(self):
        self.get_logger().info('Stopping all motors...')
        msg = OverrideRCIn()
        msg.channels = [65535] * 18
        msg.channels[2] = 1000  # Minimum throttle
        self.rc_override_pub.publish(msg)
        time.sleep(1)
        self.arm(False)

def main():
    rclpy.init()
    node = ArmAndSpinNode()

    node.get_logger().info('Waiting for FCU connection...')
    # Use a loop to check for connection
    try:
        while rclpy.ok() and not node.current_state.connected:
            rclpy.spin_once(node, timeout_sec=0.1)
            # node.get_logger().info('Waiting for connection...', once=True) # ROS 2 doesn't have once=True in Python logger like C++

        if not rclpy.ok():
            return

        node.get_logger().info('FCU connected.')
        print('\n--- SAFETY CHECK ---')
        print('PROPS OFF confirmed?')
        input('Press Enter to set STABILIZE mode...')
        
        if not node.set_mode('STABILIZE'):
            node.get_logger().error('Failed to set STABILIZE mode. Aborting.')
            return

        time.sleep(1)

        input('Press Enter to ARM motors...')
        success = node.arm(True)

        if not success:
            node.get_logger().error('Arming failed — check pre-arm messages in Mission Planner / MAVROS logs')
            # Try to disarm just in case
            node.arm(False)
            return

        time.sleep(2)

        input('Press Enter to spin at 15% throttle for 3 seconds...')
        node.get_logger().info('Spinning at 15% (1150µs)...')

        start_time = time.time()
        while rclpy.ok() and (time.time() - start_time < 3.0):
            node.send_throttle(15)
            # We must spin the node to handle state updates or other background tasks if any
            rclpy.spin_once(node, timeout_sec=0.05)

        node.get_logger().info('Spinning complete. Stopping and disarming...')
        node.stop_all()

    except KeyboardInterrupt:
        node.get_logger().warn('Interrupted by user. Emergency stop.')
        node.stop_all()
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
