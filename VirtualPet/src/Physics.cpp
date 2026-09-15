#include "Physics.h"

#include <algorithm>
#include <cmath>

namespace VirtualPet {

Physics::Physics(const PhysicsConfig& config)
    : m_config(config) {}

void Physics::SetPosition(float x, float y) noexcept {
    m_x = x;
    m_y = y;
}

void Physics::SetVelocity(float vx, float vy) noexcept {
    m_vx = vx;
    m_vy = vy;
}

void Physics::ApplyImpulse(float ix, float iy) noexcept {
    m_vx += ix;
    m_vy += iy;
    m_isGrounded = false;
}

void Physics::AddForce(float fx, float fy) noexcept {
    m_ax += fx;
    m_ay += fy;
}

void Physics::SetDimensions(int32_t width, int32_t height) noexcept {
    m_width = (width > 0) ? width : 128;
    m_height = (height > 0) ? height : 128;
}

void Physics::StartDragging(const Point& cursorScreenPos, const Point& dragOffset) noexcept {
    m_isDragged = true;
    m_isGrounded = false;
    m_dragOffset = dragOffset;
    m_targetDragPos = Point(cursorScreenPos.x - dragOffset.x, cursorScreenPos.y - dragOffset.y);
    m_vx = 0.0f;
    m_vy = 0.0f;
    m_recentDragVx = 0.0f;
    m_recentDragVy = 0.0f;
}

void Physics::UpdateDragging(const Point& cursorScreenPos) noexcept {
    m_targetDragPos = Point(cursorScreenPos.x - m_dragOffset.x, cursorScreenPos.y - m_dragOffset.y);
}

void Physics::StopDragging() noexcept {
    m_isDragged = false;
    constexpr float maxThrowSpeed = 1200.0f;
    m_vx = std::clamp(m_recentDragVx, -maxThrowSpeed, maxThrowSpeed);
    m_vy = std::clamp(m_recentDragVy, -maxThrowSpeed, maxThrowSpeed);
}

void Physics::SpawnToy(float x, float y, float vx, float vy, bool isTreat) noexcept {
    m_toy.x = x;
    m_toy.y = y;
    m_toy.vx = vx;
    m_toy.vy = vy;
    m_toy.radius = isTreat ? 8 : 14;
    m_toy.isTreat = isTreat;
    m_toy.active = true;
}

void Physics::Update(float deltaTime, const DesktopWorld& world) {
    if (deltaTime <= 0.0f) return;
    const float dt = (std::min)(deltaTime, 0.05f);

    // 1. Step Toy/Treat Physics if active
    if (m_toy.active) {
        m_toy.vy += m_config.gravity * dt;
        m_toy.x += m_toy.vx * dt;
        m_toy.y += m_toy.vy * dt;

        int32_t toyX = static_cast<int32_t>(m_toy.x);
        int32_t toyGroundY = world.GetSupportingSurfaceY(toyX, static_cast<int32_t>(m_toy.y),
                                                         m_toy.radius * 2, m_toy.radius * 2, 0);

        if (m_toy.y >= static_cast<float>(toyGroundY)) {
            m_toy.y = static_cast<float>(toyGroundY);
            if (m_toy.vy > 80.0f) {
                m_toy.vy = -m_toy.vy * 0.5f; // Toy bounce
            } else {
                m_toy.vy = 0.0f;
            }
            m_toy.vx *= 0.90f;
        }

        // Toy screen bounds check
        const MonitorInfo* mon = world.GetMonitorAt(Point(toyX, static_cast<int32_t>(m_toy.y)));
        Rect area = mon ? mon->workArea : world.GetPrimaryWorkArea();
        if (m_toy.x < static_cast<float>(area.x)) {
            m_toy.x = static_cast<float>(area.x);
            m_toy.vx = -m_toy.vx * 0.5f;
        } else if (m_toy.x > static_cast<float>(area.x + area.width - m_toy.radius * 2)) {
            m_toy.x = static_cast<float>(area.x + area.width - m_toy.radius * 2);
            m_toy.vx = -m_toy.vx * 0.5f;
        }
    }

    // 2. Step Pet Dragging
    if (m_isDragged) {
        float prevX = m_x;
        float prevY = m_y;

        float lerpFactor = std::clamp(dt / (std::max)(m_config.dragSmoothing, 0.01f), 0.0f, 1.0f);
        m_x += (static_cast<float>(m_targetDragPos.x) - m_x) * lerpFactor;
        m_y += (static_cast<float>(m_targetDragPos.y) - m_y) * lerpFactor;

        m_recentDragVx = (m_x - prevX) / dt;
        m_recentDragVy = (m_y - prevY) / dt;
        return;
    }

    // 3. Step Pet Kinematics
    m_vy += m_config.gravity * dt;
    if (m_vy > m_config.terminalVelocity) {
        m_vy = m_config.terminalVelocity;
    }

    m_vx += m_ax * dt;
    m_vy += m_ay * dt;
    m_ax = 0.0f;
    m_ay = 0.0f;

    m_x += m_vx * dt;
    m_y += m_vy * dt;

    // 4. Desktop Ground & Window Surfaces Collision
    int32_t currentX = static_cast<int32_t>(m_x);
    int32_t currentY = static_cast<int32_t>(m_y);
    int32_t supportingY = world.GetSupportingSurfaceY(currentX, currentY, m_width, m_height, m_config.groundMargin);

    if (m_y >= static_cast<float>(supportingY)) {
        m_y = static_cast<float>(supportingY);

        if (m_vy > 180.0f) {
            m_vy = -m_vy * m_config.edgeBounce;
            m_isGrounded = false;
        } else {
            m_vy = 0.0f;
            m_isGrounded = true;
        }

        m_vx *= std::pow(m_config.groundFriction, dt * 60.0f);
        if (std::abs(m_vx) < 1.0f) {
            m_vx = 0.0f;
        }
    } else {
        m_isGrounded = false;
        m_vx *= std::pow(m_config.airResistance, dt * 60.0f);
    }

    // 5. Lateral Monitor Boundaries
    const MonitorInfo* mon = world.GetMonitorAt(Point(currentX, currentY));
    Rect workArea = mon ? mon->workArea : world.GetPrimaryWorkArea();

    float minX = static_cast<float>(workArea.x);
    float maxX = static_cast<float>(workArea.x + workArea.width - m_width);

    if (m_x < minX) {
        m_x = minX;
        m_vx = -m_vx * m_config.edgeBounce;
    } else if (m_x > maxX) {
        m_x = maxX;
        m_vx = -m_vx * m_config.edgeBounce;
    }

    // Ceiling boundary
    float minY = static_cast<float>(workArea.y);
    if (m_y < minY) {
        m_y = minY;
        if (m_vy < 0.0f) {
            m_vy = -m_vy * m_config.edgeBounce;
        }
    }
}

} // namespace VirtualPet
