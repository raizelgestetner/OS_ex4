cmake_minimum_required(VERSION 3.16)
project(OS_ex4)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(OS_ex4
        MemoryConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        VirtualMemory.h
        VirtualMemory.cpp)
