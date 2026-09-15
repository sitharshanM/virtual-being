

A planned Windows C++ virtual companion whose world is your desktop.

**Status: architecture scaffold only.** This repository contains the project
layout, responsibility notes and sample state. The application is not implemented
or runnable yet. Coding starts in the next development session.

## Project structure

```text
virtual-being/
├── CMakeLists.txt
├── .gitignore
├── LICENSE
├── README.md
└── VirtualPet/
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

Asset folders contain `.gitkeep` files so Git retains the empty directories.

## Module responsibilities

| Module | Responsibility |
| --- | --- |
| main | Windows startup, window lifecycle and event loop |
| Pet | Pet identity, current state and orchestration. |
| PetBrain | Behavior decisions; later utility AI and optional dialogue. |
| BehaviorTree | Conditions, selectors, sequences and action lifecycle. |
| Memory | Persistent needs, personality and interaction history. |
| Animation | Animation clips and rendering. |
| Physics | Movement, gravity and desktop edge collisions. |
| DesktopWorld | Monitor work areas, desktop boundaries and window surfaces. |
| MouseSensor | Cursor position, proximity and drag interactions. |
| SystemSensor | Idle time, battery and opt-in system observations. |
| SaveManager | Versioned JSON persistence, validation and recovery. |

## Planned architecture

Mouse and system observations feed the pet brain. The brain selects actions;
physics updates movement and animation renders the current state. Memory stores
the pet's needs and history, while SaveManager loads and saves them.
DesktopWorld provides the screen geometry used by movement and perception.

## Development roadmap

### Phase 1 — Desktop foundation
- Transparent, borderless, always-on-top Win32 window.
- Basic idle, walk, sleep and reaction animations.
- Mouse interaction and dragging.
- Gravity and desktop edge handling.
- Validated save/load state with recovery from malformed files.
- Optional Windows startup integration with an explicit user setting.

### Phase 2 — Behavior and awareness
- Hunger, energy, mood and personality.
- Utility AI and behavior tree actions.
- Day/night cycles, idle detection and battery reactions.
- Persistent interaction memory.

### Phase 3 — Learning and dialogue
- Bounded local learning and evolving preferences.
- More animations and optional app awareness.
- Optional local LLM dialogue, separate from real-time behavior.
- Multiple pets and interactions.

## Build status

The CMake file is a placeholder; there are no executable targets or build
instructions yet. The planned toolchain is C++17, CMake and the Windows SDK
(for example, Visual Studio with Desktop development with C++).

## Assets and persistence

No artwork is bundled yet. Add original or appropriately licensed sprite frames
to the action folders when implementing animation.

`VirtualPet/data/pet_state.json` is a proposed sample schema, not an active save
file. Runtime save locations, schema migrations and atomic writes remain TODOs.

## Privacy by design

Keep the core behavior local. Future startup, app-awareness and LLM integrations
should be opt-in. Avoid collecting window contents or sending desktop activity
to external services by default.

## License

MIT — see [LICENSE](LICENSE).
