# Virtual Pet

A Windows desktop companion written in C++17. It has a transparent pet window,
mouse dragging and petting, autonomous movement and sleep, desktop window
collisions, a tray menu, and throwable toys and treats.

## Build and test

Requires CMake 3.15+ and a Windows C++17 compiler with the Windows SDK
(MSVC or MinGW-w64). From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For Ninja/MinGW, run from the toolchain shell or put its `bin` directory on PATH:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The executable is under `build/VirtualPet` (or its `Release` subfolder for
multi-configuration generators). CMake copies `config` and `assets` beside it.
Both repository-root and `VirtualPet`-directory builds support CTest.

## Controls

- Click the pet to pet it; drag to move and throw it.
- Right-click the pet or click its tray icon for feeding, play, sleep, toys,
  visibility, startup registration, status, and exit.
- Sleep requests persist until the pet recovers; petting or dragging interrupts them.
- Toys use a separate transparent window and remain visible away from the pet.

## Configuration and saves

Edit `config/pet_config.json` beside the executable. Configuration is validated;
invalid files fall back to defaults. Changes take effect after restarting.

Settings include frame rate, window size/scale/title/topmost/transparency,
physics, behavior scoring frequency, needs decay, mouse interaction, idle
threshold, resource paths, and initial position (`tray_bottom_right`,
`bottom-right`, `bottom-left`, or `center`).

Relative asset paths resolve from the application resource root. Default runtime
state is `%LOCALAPPDATA%/VirtualPet/data/pet_state.json`, independent of the
working directory. Relative `paths.save_file` values resolve beneath
`%LOCALAPPDATA%/VirtualPet`. If `save_file` is omitted, `data_dir/pet_state.json`
is used. Absolute save paths and explicit `Pet::Init` state paths are supported.
To retain an existing save from an older build, copy it to the new runtime path.
The checked-in `VirtualPet/data/pet_state.json` is sample data.

Saves validate values, preserve all interaction counters and escaped text, and
replace the destination atomically. Invalid or unreadable saves return default
state; failed writes are reported to the application's error stream.

## Artwork

Place original or appropriately licensed PNG, BMP, JPG, or JPEG frames in
`assets/idle`, `walk`, `run`, `sleep`, and `reactions`. Use zero-padded filenames
such as `001.png`, `002.png`. Frames play in filename order at 0.1 seconds per
frame. Invalid images are skipped. Without artwork, a procedural pet is shown.
The window uses a magenta transparency key; fully transparent image regions
show through. Semi-transparent edges may show a color-key fringe.

## Code layout

| Module | Responsibility |
| --- | --- |
| main | Win32 windows, tray menu, input, rendering and frame pacing |
| Pet | Coordinates sensors, brain, physics, animation and persistence |
| PetBrain | Utility scores, manual actions and interaction behavior |
| Physics / DesktopWorld | Movement, monitor bounds and window surfaces |
| Animation | GDI+ decoding, playback and rendering |
| MouseSensor / SystemSensor | Cursor, inactivity, time and power observations |
| Memory / SaveManager | Needs, personality, history, configuration and JSON saves |
| BehaviorTree | Standalone sequence/selector primitives for future behaviors |

Tests cover persistence and failure paths, config validation, clicks, manual
actions, toy contact, physics, animation timing, image decoding and rendering.
Interactive tray behavior and mixed-DPI/multiple-monitor layouts still benefit
from manual testing on the target desktop.

The pinned nlohmann/json 3.11.3 header and MIT license are vendored under
`VirtualPet/third_party/nlohmann`; builds do not download dependencies.

## Privacy and license

Behavior runs locally. Window geometry/titles, cursor, idle time, clock, and
power state are observed locally; no desktop activity is sent to a service.
Startup registration is optional. Project license: MIT (see `LICENSE`).
