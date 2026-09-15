#pragma once

#include "DesktopWorld.h"

namespace VirtualPet {

/// Configuration constants for 2D desktop physics simulation.
struct PhysicsConfig {
    float gravity{980.0f};          ///< Downward gravitational acceleration (px/s^2).
    int32_t groundMargin{10};       ///< Distance in pixels above taskbar/bottom edge.
    float edgeBounce{0.25f};        ///< Restitution coefficient on boundary collision.
    float dragSmoothing{0.25f};     ///< Lerp interpolation factor for dragging responsiveness.
    float groundFriction{0.88f};    ///< Friction multiplier applied to horizontal speed when on ground.
    float airResistance{0.99f};     ///< Velocity retention in the air.
    float terminalVelocity{1500.0f};///< Maximum fall speed in px/s.
};

/// Interactive toy or treat item simulated in desktop space.
struct ToyItem {
    float x{0.0f};
    float y{0.0f};
    float vx{0.0f};
    float vy{0.0f};
    int32_t radius{12};
    bool active{false};
    bool isTreat{false};
};

/**
 * @brief 2D kinematic physics engine handling movement, gravity, dragging, and desktop collisions.
 */
class Physics {
public:
    explicit Physics(const PhysicsConfig& config = PhysicsConfig{});
    ~Physics() = default;

    /// Advances physics simulation by deltaTime seconds.
    void Update(float deltaTime, const DesktopWorld& world);

    /// Instantly relocates the pet to specified desktop coordinates.
    void SetPosition(float x, float y) noexcept;

    /// Sets linear velocity in pixels/second.
    void SetVelocity(float vx, float vy) noexcept;

    /// Applies an instantaneous velocity impulse.
    void ApplyImpulse(float ix, float iy) noexcept;

    /// Applies an acceleration force for the next update step.
    void AddForce(float fx, float fy) noexcept;

    /// Sets entity collision dimensions.
    void SetDimensions(int32_t width, int32_t height) noexcept;

    // --- Drag Interaction ---

    /// Begins user mouse drag interaction.
    void StartDragging(const Point& cursorScreenPos, const Point& dragOffset) noexcept;

    /// Updates drag target position while mouse button is held.
    void UpdateDragging(const Point& cursorScreenPos) noexcept;

    /// Concludes drag interaction and transfers release velocity.
    void StopDragging() noexcept;

    // --- Desktop Toys & Treats ---

    void SpawnToy(float x, float y, float vx, float vy, bool isTreat = false) noexcept;
    void ClearToy() noexcept { m_toy.active = false; }
    [[nodiscard]] const ToyItem& GetToy() const noexcept { return m_toy; }
    [[nodiscard]] ToyItem& GetToy() noexcept { return m_toy; }

    // --- Queries ---

    [[nodiscard]] Point GetPosition() const noexcept {
        return Point(static_cast<int32_t>(m_x), static_cast<int32_t>(m_y));
    }
    [[nodiscard]] float GetX() const noexcept { return m_x; }
    [[nodiscard]] float GetY() const noexcept { return m_y; }
    [[nodiscard]] float GetVelocityX() const noexcept { return m_vx; }
    [[nodiscard]] float GetVelocityY() const noexcept { return m_vy; }

    [[nodiscard]] int32_t GetWidth() const noexcept { return m_width; }
    [[nodiscard]] int32_t GetHeight() const noexcept { return m_height; }
    [[nodiscard]] Rect GetBounds() const noexcept {
        return Rect(static_cast<int32_t>(m_x), static_cast<int32_t>(m_y), m_width, m_height);
    }

    [[nodiscard]] bool IsGrounded() const noexcept { return m_isGrounded; }
    [[nodiscard]] bool IsDragged() const noexcept { return m_isDragged; }

    [[nodiscard]] const PhysicsConfig& GetConfig() const noexcept { return m_config; }
    void SetConfig(const PhysicsConfig& config) noexcept { m_config = config; }

private:
    PhysicsConfig m_config;

    float m_x{100.0f};
    float m_y{100.0f};
    float m_vx{0.0f};
    float m_vy{0.0f};
    float m_ax{0.0f};
    float m_ay{0.0f};

    int32_t m_width{128};
    int32_t m_height{128};

    bool m_isGrounded{false};
    bool m_isDragged{false};

    Point m_dragOffset{0, 0};
    Point m_targetDragPos{0, 0};
    float m_recentDragVx{0.0f};
    float m_recentDragVy{0.0f};

    ToyItem m_toy;
};

} // namespace VirtualPet
