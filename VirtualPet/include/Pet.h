#pragma once

#include "Animation.h"
#include "CompanionLife.h"
#include "DesktopWorld.h"
#include "Memory.h"
#include "LocalLLM.h"
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
    bool Init(const std::string& configPath = "",
              const std::string& statePath = "");

    /// Advances the pet simulation frame by deltaTime seconds.
    void Update(float deltaTime);

    /// Renders the current frame onto the window device context.
#ifdef _WIN32
    void Render(HDC hdc);
#endif
    void RefreshDesktop() { m_desktopWorld.Refresh(); }
    void CancelInteraction() { m_mouseSensor.CancelInteraction(); if (m_physics.IsDragged()) m_physics.StopDragging(); }

    /// Explicitly persists current companion state to disk.
    void SaveState();

    // --- Interactive Companion Actions ---

    void GiveHeadpat(float amount = 0.04f);
    void SendCoffee(float amount = 0.35f);
    void TossPlushieHeart();
    void StartStudyMode(float duration = 120.0f);

    // Legacy / Convenience wrappers
    void Feed(float amount = 0.35f) { SendCoffee(amount); }
    void Play(float enjoyment = 0.2f) { (void)enjoyment; TossPlushieHeart(); }
    void InteractPet() { GiveHeadpat(); }
    void DropToy(bool isTreat = false);
    void PlaySpecialAnimation(std::string name, float duration = 1.2f);
    bool SendChatMessage(const std::string& message);
    [[nodiscard]] std::string GetConversationText() const { return m_life.ConversationText(); }
    [[nodiscard]] std::string GetDiaryText() const { return m_life.DiaryText(); }
    [[nodiscard]] bool IsAiTyping() const noexcept { return m_localLlm.IsBusy(); }

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
    [[nodiscard]] Memory& GetMemory() noexcept { return m_memory; }
    [[nodiscard]] const PetBrain& GetBrain() const noexcept { return m_brain; }
    [[nodiscard]] PetBrain& GetBrain() noexcept { return m_brain; }
    [[nodiscard]] std::string GetSpeechText() const { return m_localLlm.HasReply() ? m_localLlm.GetReply() : m_brain.GetThought(); }
    [[nodiscard]] std::string GetLocalLlmStatus() const { return m_localLlm.GetStatus(); }

private:
    void UpdateStep(float deltaTime);
    AppConfigData m_config;
    Animation m_animation;
    Physics m_physics;
    MouseSensor m_mouseSensor;
    SystemSensor m_systemSensor;
    DesktopWorld m_desktopWorld;
    Memory m_memory;
    LocalLLM m_localLlm;
    CompanionLife m_life;
    SaveManager m_saveManager;
    PetBrain m_brain;

    float m_worldTimer{0.0f};
    float m_autoSaveTimer{0.0f};
    float m_autoSaveInterval{30.0f};
    uint64_t m_seenReplyVersion{0};
    float m_contextTimer{0.0f};
    std::string m_lastApplication;
    int32_t m_seenBoundaries{0};
    std::string m_specialAnimation;
    float m_specialAnimationTimer{0.0f};
    PetAction m_prevAction{PetAction::Idle};   ///< Action from last frame, used to detect transitions.
    bool m_studyAnimActive{false};              ///< True while the reading loop special-anim is playing.
};

} // namespace VirtualPet
