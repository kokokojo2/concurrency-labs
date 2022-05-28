cmake_minimum_required(VERSION 3.21)
project(pc_lab1)

set(CMAKE_CXX_STANDARD 14)

add_executable(pc_lab1 lab5/server.cpp lab5/socket.cpp lab5/socket.h lab5/Logger.cpp lab5/Logger.h)
