cmake_minimum_required(VERSION 3.24)
project(SimplyMLConsumer LANGUAGES CXX)

find_package(SimplyML 0.1 REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE SimplyML::SimplyML)
