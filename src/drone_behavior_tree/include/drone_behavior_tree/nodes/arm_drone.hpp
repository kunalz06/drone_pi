#include <memory>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "mavros_msgs/srv/command_bool.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class ArmDroneNode : public BT::StatefulActionNode {
public:
  ArmDroneNode(const std::string &name, const BT::NodeConfig &config,
               rclcpp::Node::SharedPtr node)
      : BT::StatefulActionNode(name, config), node_(node) {
    client_ = node_->create_client<mavros_msgs::srv::CommandBool>(
        "/mavros/cmd/arming");
  }

  static BT::PortsList providedPorts() {
    return {BT::InputPort<bool>(
        "value", true, "True to arm, false to disarm (default: true)")};
  }

  BT::NodeStatus onStart() override {
    if (!client_->wait_for_service(3s)) {
      RCLCPP_ERROR(node_->get_logger(), "Arming service not available");
      return BT::NodeStatus::FAILURE;
    }

    auto request = std::make_shared<mavros_msgs::srv::CommandBool::Request>();

    bool arm_value = true;
    getInput("value", arm_value);
    request->value = arm_value;

    RCLCPP_INFO(node_->get_logger(), "Requesting %s...",
                arm_value ? "ARM" : "DISARM");

    future_ = client_->async_send_request(request).future.share();
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override {
    if (future_.wait_for(0s) == std::future_status::ready) {
      try {
        auto response = future_.get();
        if (response->success) {
          RCLCPP_INFO(node_->get_logger(), "Arming command successful: %d",
                      response->result);
          return BT::NodeStatus::SUCCESS;
        } else {
          RCLCPP_ERROR(node_->get_logger(), "Arming command failed: %d",
                       response->result);
          // Sometimes Pixhawk rejects if safety switch is on or pre-arm fails.
          return BT::NodeStatus::FAILURE;
        }
      } catch (const std::exception &e) {
        RCLCPP_ERROR(node_->get_logger(), "Service call exception: %s",
                     e.what());
        return BT::NodeStatus::FAILURE;
      }
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {
    RCLCPP_INFO(node_->get_logger(), "Arming Node halted");
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedPtr client_;
  rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedFuture future_;
};
