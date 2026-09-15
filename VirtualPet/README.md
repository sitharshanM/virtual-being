# VirtualPet application

See the [repository README](../README.md) for controls, configuration, persistence,
artwork, and architecture.

This subdirectory also builds independently:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Requires Windows and a C++17 compiler (MSVC or MinGW-w64). Config and asset
folders are copied beside the executable. Runtime saves default to
`%LOCALAPPDATA%/VirtualPet/data/pet_state.json`.
