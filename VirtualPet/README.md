# VirtualPet

A Windows desktop companion pet application written in C++17.

## Directory Structure

```text
VirtualPet/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── src/
│   ├── main.cpp
│   ├── Pet.cpp
│   ├── PetBrain.cpp
│   ├── BehaviorTree.cpp
│   ├── Memory.cpp
│   ├── Animation.cpp
│   ├── Physics.cpp
│   ├── DesktopWorld.cpp
│   ├── MouseSensor.cpp
│   ├── SystemSensor.cpp
│   └── SaveManager.cpp
├── include/
│   ├── Pet.h
│   ├── PetBrain.h
│   ├── BehaviorTree.h
│   ├── Memory.h
│   ├── Animation.h
│   ├── Physics.h
│   ├── DesktopWorld.h
│   ├── MouseSensor.h
│   ├── SystemSensor.h
│   └── SaveManager.h
├── assets/
│   ├── idle/
│   ├── walk/
│   ├── run/
│   ├── sleep/
│   └── reactions/
├── data/
│   └── pet_state.json
└── config/
    └── pet_config.json
```

## Module Overview

- **src/**: Implementation files (`.cpp`) for window lifecycle, pet state orchestration, decision making, behavior trees, physics, and sensors.
- **include/**: Header declarations (`.h`) exposing class interfaces and data structures.
- **assets/**: Sprite sheets and animation frames categorized by action.
- **data/**: Dynamic pet runtime state (e.g. `pet_state.json`).
- **config/**: User and application configuration settings (e.g. `pet_config.json`).

## Building

Requires CMake 3.15+ and a C++17-capable compiler on Windows (Visual Studio / MSVC or Clang/LLVM):

```bash
cmake -B build -S .
cmake --build build --config Release
```
