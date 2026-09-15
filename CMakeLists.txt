cmake_minimum_required(VERSION 3.15)
project(VirtualBeing LANGUAGES CXX)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the VirtualPet application subdirectory
add_subdirectory(VirtualPet)
