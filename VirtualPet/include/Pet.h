#pragma once

#include "Animation.h"
#include "DesktopWorld.h"
#include "Memory.h"
#include "MouseSensor.h"
#include "PetBrain.h"
#include "Physics.h"
#include "SaveManager.h"
#include "SystemSensor.h"

#include <string>

namespace VirtualPet {

/**
 * @brief Top-level orchestrator connecting animation, physics, perception, memory, and cognition.
 */
class Pet {
public:
    Pet();
    ~Pet();

    /// Initializes all sub-modules, loads configuration and saved state.
    bool Init(const std::string& configPath = "config/pet_config.json",
              const std::string& statePath = "data/pet_state.json");

    /// Advances the pet simulation frame by deltaTime seconds.
    void Update(float deltaTime);

    /// Renders the current frame onto the window device context.
    void Render(HDC hdc);

    /// Explicitly persists current pet state to disk.
    void SaveState();

    // --- Interactive Mini-Games & Actions ---

    void Feed(float amount = 0.35f);
    void Play(float enjoyment = 0.2f);
    void Sleep();
    void InteractPet();
    void DropToy(bool isTreat = false);

    // --- Window / OS Event Callbacks ---

    void OnLButtonDown(const Point& screenPos);
    void OnLButtonUp(const Point& screenPos);
    void OnMouseMove(const Point& screenPos);
    void OnRButtonUp(const Point& screenPos);

    // --- Transform & Bounding Queries ---

    [[nodiscard]] Point GetPosition() const noexcept { return m_physics.GetPosition(); }
    [[nodiscard]] Rect GetBounds() const noexcept { return m_physics.GetBounds(); }
    [[nodiscard]] int32_t GetWidth() const noexcept { return m_physics.GetWidth(); }
    [[nodiscard]] int32_t GetHeight() const noexcept { return m_physics.GetHeight(); }

    // --- Subsystem Accessors ---

    [[nodiscard]] const AppConfigData& GetConfig() const noexcept { return m_config; }
    [[nodiscard]] const Animation& GetAnimation() const noexcept { return m_animation; }
    [[nodiscard]] const Physics& GetPhysics() const noexcept { return m_physics; }
    [[nodiscard]] Physics& GetPhysics() noexcept { return m_physics; }
    [[nodiscard]] const MouseSensor& GetMouseSensor() const noexcept { return m_mouseSensor; }
    [[nodiscard]] const SystemSensor& GetSystemSensor() const noexcept { return m_systemSensor; }
    [[nodiscard]] const DesktopWorld& GetDesktopWorld() const noexcept { return m_desktopWorld; }
    [[nodiscard]] const Memory& GetMemory() const noexcept { return m_memory; }
    [[nodiscard]] const PetBrain& GetBrain() const noexcept { return m_brain; }

private:
    AppConfigData m_config;
    Animation m_animation;
    Physics m_physics;
    MouseSensor m_mouseSensor;
    SystemSensor m_systemSensor;
    DesktopWorld m_desktopWorld;
    Memory m_memory;
    SaveManager m_saveManager;
    PetBrain m_brain;

    float m_autoSaveTimer{0.0f};
    float m_autoSaveInterval{30.0f};
};

} // namespace VirtualPet
