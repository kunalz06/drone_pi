#!/usr/bin/env python3

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    pkg_dir = FindPackageShare('drone_tests')

    tree_file_arg = DeclareLaunchArgument(
        'tree_xml_file',
        default_value=PathJoinSubstitution([pkg_dir, 'trees', 'motor_test_tree.xml']),
        description='Path to the BehaviorTree XML file'
    )

    fcu_url_arg = DeclareLaunchArgument(
        'fcu_url',
        default_value='/dev/ttyfcu:921600',
        description='MAVROS FCU URL (e.g. /dev/ttyfcu:921600 or udp://127.0.0.1:14550)'
    )

    mavros_node = Node(
        package='mavros',
        executable='mavros_node',
        name='mavros',
        output='screen',
        parameters=[{
            'fcu_url': LaunchConfiguration('fcu_url'),
            'gcs_url': '',
            'target_system_id': 1,
            'target_component_id': 1
        }]
    )

    bt_executor_node = Node(
        package='drone_behavior_tree',
        executable='bt_executor',
        name='bt_executor',
        output='screen',
        parameters=[{
            'tree_xml_file': LaunchConfiguration('tree_xml_file')
        }]
    )

    return LaunchDescription([
        tree_file_arg,
        fcu_url_arg,
        mavros_node,
        bt_executor_node
    ])
