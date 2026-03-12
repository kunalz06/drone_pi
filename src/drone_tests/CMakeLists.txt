cmake_minimum_required(VERSION 3.8)
project(drone_tests)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY trees launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
