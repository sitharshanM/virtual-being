#pragma once

#include <cstdint>

namespace VirtualPet {

/// 2D integer coordinate point.
struct Point {
    int32_t x{0};
    int32_t y{0};

    constexpr Point() = default;
    constexpr Point(int32_t x_, int32_t y_) : x(x_), y(y_) {}

    constexpr bool operator==(const Point& other) const noexcept {
        return x == other.x && y == other.y;
    }
    constexpr bool operator!=(const Point& other) const noexcept {
        return !(*this == other);
    }
};

/// 2D bounding rectangle.
struct Rect {
    int32_t x{0};
    int32_t y{0};
    int32_t width{0};
    int32_t height{0};

    constexpr Rect() = default;
    constexpr Rect(int32_t x_, int32_t y_, int32_t w_, int32_t h_)
        : x(x_), y(y_), width(w_), height(h_) {}

    constexpr bool Contains(const Point& pt) const noexcept {
        return pt.x >= x && pt.x < (x + width) &&
               pt.y >= y && pt.y < (y + height);
    }

    constexpr Point Center() const noexcept {
        return Point(x + width / 2, y + height / 2);
    }
};

/// Configuration parameters for mouse perception and drag interactions.
struct MouseSensorConfig {
    float proximityDistancePx{150.0f}; ///< Proximity radius around pet to trigger "near" awareness.
    bool detectCursorDrag{true};       ///< Whether mouse dragging of the pet window is detected.
    float dragThresholdPx{4.0f};       ///< Minimum movement in pixels before a click becomes a drag.
};

/// Snapshot of mouse interaction state at a given frame.
struct MouseState {
    Point screenPosition{0, 0};        ///< Global cursor position on the desktop.
    Point positionDelta{0, 0};         ///< Change in cursor position since last update.
    Point petRelativePosition{0, 0};   ///< Cursor position relative to pet window top-left.
    float distanceToPet{0.0f};         ///< Euclidean distance from cursor to pet center.

    bool isNear{false};                ///< Cursor is within proximity threshold.
    bool isHovering{false};            ///< Cursor is directly over the pet's bounds.
    bool isDragging{false};            ///< User is clicking and dragging the pet.

    bool leftButtonDown{false};        ///< Current left button state.
    bool rightButtonDown{false};       ///< Current right button state.
    bool leftButtonClicked{false};     ///< True on the single frame left button was released after click.
    bool rightButtonClicked{false};    ///< True on the single frame right button was released after click.

    Point dragOffset{0, 0};            ///< Offset between cursor and pet origin at start of drag.
};

/**
 * @brief Senses desktop mouse activity, cursor proximity to the pet, and dragging interactions.
 * 
 * MouseSensor tracks global desktop cursor position and local window interactions
 * to inform the pet's behavioral brain and physics engine.
 */
class MouseSensor {
public:
    explicit MouseSensor(const MouseSensorConfig& config = MouseSensorConfig{});
    ~MouseSensor() = default;

    /// Update sensor state for the current frame.
    /// @param deltaTime Time elapsed since last frame in seconds.
    /// @param petBounds Current bounding rectangle of the pet window in desktop space.
    void Update(float deltaTime, const Rect& petBounds);

    /// Notify sensor of a mouse button down event (e.g. from WM_LBUTTONDOWN).
    void OnButtonDown(bool leftButton, const Point& screenPos, const Rect& petBounds);

    /// Notify sensor of a mouse button up event (e.g. from WM_LBUTTONUP).
    void OnButtonUp(bool leftButton, const Point& screenPos);

    /// Notify sensor of raw mouse movement (e.g. from WM_MOUSEMOVE).
    void OnMouseMove(const Point& screenPos, const Rect& petBounds);

    /// Reset transient frame states (click flags, deltas).
    void ResetFrameState();

    // --- State Queries ---

    [[nodiscard]] const MouseState& GetState() const noexcept { return m_state; }
    [[nodiscard]] Point GetCursorPosition() const noexcept { return m_state.screenPosition; }
    [[nodiscard]] Point GetCursorDelta() const noexcept { return m_state.positionDelta; }
    [[nodiscard]] Point GetPetRelativePosition() const noexcept { return m_state.petRelativePosition; }
    [[nodiscard]] float GetDistanceToPet() const noexcept { return m_state.distanceToPet; }

    [[nodiscard]] bool IsNear() const noexcept { return m_state.isNear; }
    [[nodiscard]] bool IsHovering() const noexcept { return m_state.isHovering; }
    [[nodiscard]] bool IsDragging() const noexcept { return m_state.isDragging; }
    [[nodiscard]] Point GetDragOffset() const noexcept { return m_state.dragOffset; }

    [[nodiscard]] bool IsLeftButtonDown() const noexcept { return m_state.leftButtonDown; }
    [[nodiscard]] bool IsRightButtonDown() const noexcept { return m_state.rightButtonDown; }
    [[nodiscard]] bool WasLeftButtonClicked() const noexcept { return m_state.leftButtonClicked; }
    [[nodiscard]] bool WasRightButtonClicked() const noexcept { return m_state.rightButtonClicked; }

    // --- Configuration ---

    [[nodiscard]] const MouseSensorConfig& GetConfig() const noexcept { return m_config; }
    void SetConfig(const MouseSensorConfig& config) noexcept { m_config = config; }
    void SetProximityDistance(float distancePx) noexcept { m_config.proximityDistancePx = distancePx; }

private:
    /// Polls system cursor position using OS API (e.g. Win32 GetCursorPos).
    void PollOSCursor();

    MouseSensorConfig m_config;
    MouseState m_state;

    bool m_trackingClick{false};
    Point m_clickStartPos{0, 0};
    bool m_prevLeftButtonDown{false};
    bool m_prevRightButtonDown{false};
    Point m_lastPolledPosition{0, 0};
};

} // namespace VirtualPet
