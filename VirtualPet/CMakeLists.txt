cmake_minimum_required(VERSION 3.15)
project(VirtualPet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Directory variables
set(PET_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Gather source and header files
file(GLOB_RECURSE PET_SOURCES "${PET_SRC_DIR}/*.cpp")
file(GLOB_RECURSE PET_HEADERS "${PET_INCLUDE_DIR}/*.h")

# Define the VirtualPet executable target
add_executable(VirtualPet ${PET_SOURCES} ${PET_HEADERS})

# Include headers from the include directory
target_include_directories(VirtualPet PRIVATE ${PET_INCLUDE_DIR})

# Link Windows libraries needed for desktop windowing, graphics, and sensors
if(WIN32)
    target_link_libraries(VirtualPet PRIVATE
        user32
        gdi32
        dwmapi
        shell32
    )
endif()
