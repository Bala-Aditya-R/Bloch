cmake_minimum_required(VERSION 3.20)
project(BlochSphere LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib CONFIG REQUIRED)

add_executable(bloch main.cpp)
target_link_libraries(bloch PRIVATE raylib)
