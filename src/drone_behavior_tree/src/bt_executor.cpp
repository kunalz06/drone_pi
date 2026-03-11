#include <chrono>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/bt_cout_logger.h"
#include "rclcpp/rclcpp.hpp"

// Custom nodes
#include "drone_behavior_tree/nodes/arm_drone.hpp"
#include "drone_behavior_tree/nodes/motor_test.hpp"

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("bt_executor");

  node->declare_parameter("tree_xml_file", "");

  std::string xml_file;
  node->get_parameter("tree_xml_file", xml_file);

  if (xml_file.empty()) {
    RCLCPP_ERROR(node->get_logger(), "Parameter 'tree_xml_file' is empty");
    return 1;
  }

  RCLCPP_INFO(node->get_logger(), "Loading behavior tree from: %s",
              xml_file.c_str());

  BT::BehaviorTreeFactory factory;

  // Register custom nodes
  factory.registerNodeType<ArmDroneNode>("ArmDrone", node);
  factory.registerNodeType<MotorTestNode>("MotorTest", node);

  // Parse the XML
  auto tree = factory.createTreeFromFile(xml_file);

  // Add a logger to print state transitions to the console (which gets saved to
  // our log file)
  BT::StdCoutLogger logger_cout(tree);

  RCLCPP_INFO(node->get_logger(), "--- Behavior Tree Execution Started ---");

  // Spin ROS in a background thread so callbacks function during BT tick
  std::thread spin_thread([node]() { rclcpp::spin(node); });

  // Tick the tree continuously until it finishes
  BT::NodeStatus status = BT::NodeStatus::RUNNING;
  while (rclcpp::ok() && status == BT::NodeStatus::RUNNING) {
    status = tree.tickExactlyOnce();

    // Sleep to prevent maxing out the CPU loop
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (status == BT::NodeStatus::SUCCESS) {
    RCLCPP_INFO(node->get_logger(), "--- Behavior Tree Execution SUCCESS ---");
  } else if (status == BT::NodeStatus::FAILURE) {
    RCLCPP_ERROR(node->get_logger(), "--- Behavior Tree Execution FAILED ---");
  }

  rclcpp::shutdown();
  spin_thread.join();
  return 0;
}
