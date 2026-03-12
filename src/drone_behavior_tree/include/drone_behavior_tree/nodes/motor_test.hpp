#include <memory>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "mavros_msgs/srv/command_long.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

/**
 * ActionNode to test a specific motor.
 * Uses MAV_CMD_DO_MOTOR_TEST (Command ID 209).
 * Parameters:
 *  - motor: The motor number to test (1 to 8)
 *  - throttle: The throttle percentage (0 to 100, e.g. 5 for testing)
 *  - duration: The duration in seconds to spin the motor.
 */
class MotorTestNode : public BT::StatefulActionNode {
public:
  MotorTestNode(const std::string &name, const BT::NodeConfig &config,
                rclcpp::Node::SharedPtr node)
      : BT::StatefulActionNode(name, config), node_(node) {
    client_ = node_->create_client<mavros_msgs::srv::CommandLong>(
        "/mavros/cmd/command");
  }

  static BT::PortsList providedPorts() {
    return {
        BT::InputPort<int>("motor", "Motor number (1-8)"),
        BT::InputPort<float>("throttle", 5.0, "Throttle percentage (0-100)"),
        BT::InputPort<float>("duration", 3.0, "Duration in seconds")};
  }

  BT::NodeStatus onStart() override {
    if (!client_->wait_for_service(3s)) {
      RCLCPP_ERROR(node_->get_logger(), "CommandLong service not available");
      return BT::NodeStatus::FAILURE;
    }

    int motor = 1;
    float throttle = 5.0;
    float duration = 3.0;

    getInput("motor", motor);
    getInput("throttle", throttle);
    getInput("duration", duration);

    motor_ = motor; // Store for use in onRunning logs

    // MAV_CMD_DO_MOTOR_TEST = 209
    auto request = std::make_shared<mavros_msgs::srv::CommandLong::Request>();
    request->command = 209;
    request->confirmation = 0;
    request->param1 = static_cast<float>(motor); // Motor number
    request->param2 = 0.0f; // MOTOR_TEST_THROTTLE_PERCENT (0 for percent type)
    request->param3 = throttle; // Value (0-100)
    request->param4 = duration; // Timeout (sec)
    request->param5 = 0;        // Motor count (0 means just one)
    request->param6 = 0;        // Test type (0 is standard)

    RCLCPP_INFO(node_->get_logger(),
                "Testing Motor %d at %.1f%% for %.1f seconds...", motor,
                throttle, duration);

    future_ = client_->async_send_request(request).future.share();

    // We also need to wait for the duration to complete in the behavior tree
    // However, MAVROS command service returns ACK immediately, not when the
    // duration is over. So we'll store the start time and block SUCCESS until
    // duration elapses.
    start_time_ = node_->now();
    wait_duration_ =
        rclcpp::Duration::from_seconds(duration + 0.5); // Add 0.5s padding
    cmd_acked_ = false;

    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override {
    // 1. Check if the command was ACKed by the flight controller
    if (!cmd_acked_) {
      if (future_.wait_for(0s) == std::future_status::ready) {
        try {
          auto response = future_.get();
          if (response->success) {
            RCLCPP_INFO(node_->get_logger(),
                        "Motor %d test command ACKed (result: %d)", motor_,
                        response->result);
            cmd_acked_ = true;
          } else {
            RCLCPP_ERROR(node_->get_logger(), "Motor test command failed: %d",
                         response->result);
            return BT::NodeStatus::FAILURE;
          }
        } catch (const std::exception &e) {
          RCLCPP_ERROR(node_->get_logger(), "Service exception: %s", e.what());
          return BT::NodeStatus::FAILURE;
        }
      } else {
        return BT::NodeStatus::RUNNING; // Waiting for ACK
      }
    }

    // 2. We have the ACK. Now wait for the duration to physically finish
    if (node_->now() - start_time_ >= wait_duration_) {
      RCLCPP_INFO(node_->get_logger(), "Motor test duration elapsed.");
      return BT::NodeStatus::SUCCESS;
    }

    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {
    RCLCPP_INFO(node_->get_logger(), "MotorTest Node halted.");
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<mavros_msgs::srv::CommandLong>::SharedPtr client_;
  rclcpp::Client<mavros_msgs::srv::CommandLong>::SharedFuture future_;
  rclcpp::Time start_time_;
  rclcpp::Duration wait_duration_{0, 0};
  bool cmd_acked_ = false;
  int motor_ = 0;
};
