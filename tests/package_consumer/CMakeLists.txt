cmake_minimum_required(VERSION 3.19)

project(bethutil_package_consumer LANGUAGES CXX)

find_package(bethutil CONFIG REQUIRED)

add_executable(package_consumer main.cpp)
target_link_libraries(package_consumer PRIVATE btu::bethutil)
