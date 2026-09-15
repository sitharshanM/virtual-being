#include "Pet.h"

namespace VirtualPet {

Pet::Pet() = default;

Pet::~Pet() {
    SaveState();
}

bool Pet::Init(const std::string& configPath, const std::string& statePath) {
    m_saveManager.SetFilePath(statePath);

    // 1. Load application configuration
    m_saveManager.LoadConfig(configPath, m_config);

    // Apply configuration settings to subsystems
    PhysicsConfig pc;
    pc.gravity = m_config.gravity;
    pc.groundMargin = m_config.groundMargin;
    pc.edgeBounce = m_config.edgeBounce;
    pc.dragSmoothing = m_config.dragSmoothing;
    m_physics.SetConfig(pc);

    int32_t scaledW = static_cast<int32_t>(m_config.windowWidth * m_config.windowScale);
    int32_t scaledH = static_cast<int32_t>(m_config.windowHeight * m_config.windowScale);
    m_physics.SetDimensions(scaledW, scaledH);

    m_memory.SetDecayRates(m_config.energyDepletionRate, m_config.hungerIncreaseRate);
    m_mouseSensor.SetProximityDistance(m_config.mouseProximityDistancePx);

    if (m_config.startWithWindows && !SaveManager::IsStartWithWindowsEnabled()) {
        SaveManager::SetStartWithWindows(true);
    }

    // 2. Load saved state or fallback defaults
    PetStateData loadedData;
    m_saveManager.Load(loadedData);
    m_memory.SetData(loadedData);

    // 3. Discover and load sprite animations
    m_animation.LoadFromDirectory(m_config.assetsDir);

    // 4. Initialize desktop world
    m_desktopWorld.Refresh();

    // 5. Place pet on desktop
    Rect work = m_desktopWorld.GetPrimaryWorkArea();
    int32_t startX = work.x + work.width - m_physics.GetWidth() - 80;
    int32_t startY = m_desktopWorld.GetGroundY(startX, m_physics.GetHeight(), m_physics.GetConfig().groundMargin);
    m_physics.SetPosition(static_cast<float>(startX), static_cast<float>(startY));

    return true;
}

void Pet::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    Rect bounds = m_physics.GetBounds();

    // 1. Update sensors
    m_mouseSensor.Update(deltaTime, bounds);
    m_systemSensor.Update(deltaTime);

    // 2. Process drag state between mouse sensor and physics engine
    if (m_mouseSensor.IsDragging() && !m_physics.IsDragged()) {
        m_physics.StartDragging(m_mouseSensor.GetCursorPosition(), m_mouseSensor.GetDragOffset());
    } else if (m_mouseSensor.IsDragging() && m_physics.IsDragged()) {
        m_physics.UpdateDragging(m_mouseSensor.GetCursorPosition());
    } else if (!m_mouseSensor.IsDragging() && m_physics.IsDragged()) {
        m_physics.StopDragging();
    }

    // 3. Update memory needs decay
    m_memory.Update(deltaTime);

    // 4. Update brain cognition and utility-based decisions
    BrainDecision decision = m_brain.Update(deltaTime,
                                            m_mouseSensor,
                                            m_systemSensor,
                                            m_memory,
                                            m_desktopWorld,
                                            m_physics);

    // 5. Apply movement intent to physics
    if (!m_physics.IsDragged()) {
        if (m_physics.IsGrounded()) {
            m_physics.SetVelocity(decision.targetHorizontalSpeed, m_physics.GetVelocityY());
            if (decision.wantJump) {
                m_physics.ApplyImpulse(0.0f, -420.0f);
            }
        }
    }

    // 6. Step physics simulation (includes window surface collisions and toy movement)
    m_physics.Update(deltaTime, m_desktopWorld);

    // 7. Step animation controller
    m_animation.Play(decision.animState);
    m_animation.SetFacingLeft(decision.facingLeft);
    m_animation.Update(deltaTime);

    // 8. Auto-save state periodically
    m_autoSaveTimer += deltaTime;
    if (m_autoSaveTimer >= m_autoSaveInterval) {
        m_autoSaveTimer = 0.0f;
        SaveState();
    }
}

void Pet::Render(HDC hdc) {
    // 1. Render pet sprite / silhouette
    m_animation.Render(hdc, 0, 0, m_physics.GetWidth(), m_physics.GetHeight());

    // 2. Render toy or treat if positioned near the pet
    const ToyItem& toy = m_physics.GetToy();
    if (toy.active) {
        Point pos = m_physics.GetPosition();
        int32_t localX = static_cast<int32_t>(toy.x) - pos.x;
        int32_t localY = static_cast<int32_t>(toy.y) - pos.y;

        if (localX >= -30 && localX < m_physics.GetWidth() + 30 &&
            localY >= -30 && localY < m_physics.GetHeight() + 30) {
            HBRUSH brush = toy.isTreat ? ::CreateSolidBrush(RGB(180, 100, 40))
                                       : ::CreateSolidBrush(RGB(220, 50, 50));
            HGDIOBJ oldBrush = ::SelectObject(hdc, brush);
            ::Ellipse(hdc, localX - toy.radius, localY - toy.radius,
                           localX + toy.radius, localY + toy.radius);
            ::SelectObject(hdc, oldBrush);
            ::DeleteObject(brush);
        }
    }
}

void Pet::SaveState() {
    m_saveManager.Save(m_memory.GetData());
}

void Pet::Feed(float amount) {
    m_memory.Feed(amount);
    m_animation.Play(AnimationState::Reaction, true);
}

void Pet::Play(float enjoyment) {
    m_memory.Play(enjoyment);
    m_animation.Play(AnimationState::Reaction, true);
}

void Pet::Sleep() {
    m_animation.Play(AnimationState::Sleep, true);
}

void Pet::InteractPet() {
    m_memory.Pet(0.1f);
    m_animation.Play(AnimationState::Reaction, true);
}

void Pet::DropToy(bool isTreat) {
    Point petPos = m_physics.GetPosition();
    float throwVx = m_animation.IsFacingLeft() ? -260.0f : 260.0f;
    m_physics.SpawnToy(static_cast<float>(petPos.x + m_physics.GetWidth() / 2),
                       static_cast<float>(petPos.y - 30),
                       throwVx, -220.0f, isTreat);
}

void Pet::OnLButtonDown(const Point& screenPos) {
    m_mouseSensor.OnButtonDown(true, screenPos, m_physics.GetBounds());
}

void Pet::OnLButtonUp(const Point& screenPos) {
    m_mouseSensor.OnButtonUp(true, screenPos);
}

void Pet::OnMouseMove(const Point& screenPos) {
    m_mouseSensor.OnMouseMove(screenPos, m_physics.GetBounds());
}

void Pet::OnRButtonUp(const Point& screenPos) {
    m_mouseSensor.OnButtonUp(false, screenPos);
}

} // namespace VirtualPet
