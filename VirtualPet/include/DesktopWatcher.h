#pragma once

#include "DesktopWorld.h"
#include "MouseSensor.h"

#include <vector>
#include <string>

namespace VirtualPet {

/**
 * @brief Periodically observes desktop window layout, detects window movements,
 * window disappearances, and monitors the pet's current resting surface.
 */
class DesktopWatcher {
public:
    DesktopWatcher();
    ~DesktopWatcher() = default;

    /// Configures the polling interval (in seconds). Default is 0.5s.
    void SetRefreshInterval(float intervalSeconds) noexcept { m_refreshInterval = intervalSeconds; }

    /// Advances watcher timer; triggers diff check and refresh when timer expires.
    /// @param deltaTime Elapsed frame time.
    /// @param petBounds Current world-space bounding box of the companion.
    /// @param world Reference to DesktopWorld for surface list.
    void Update(float deltaTime, const Rect& petBounds, DesktopWorld& world);

    /// Checks if a window the pet was resting on has moved during the last refresh.
    [[nodiscard]] bool SurfaceJustMoved() const noexcept { return m_surfaceJustMoved; }

    /// Checks if a window the pet was resting on has vanished (closed/minimized).
    [[nodiscard]] bool SurfaceJustGone() const noexcept { return m_surfaceJustGone; }

    /// Amount the surface moved (dx, dy) in pixels.
    [[nodiscard]] Point GetSurfaceMoveDelta() const noexcept { return m_surfaceDelta; }

    /// The surface the pet is currently standing on (nullptr if on the desktop floor).
    [[nodiscard]] const WindowSurface* GetCurrentSurface() const noexcept { return m_currentSurface; }

    /// Finds the closest climbable window top surface to a given world point.
    /// Returns nullptr if no windows have sufficient headroom on screen.
    [[nodiscard]] const WindowSurface* GetNearestSurface(const Point& point, const DesktopWorld& world, int32_t petHeight = 120) const;

    /// Checks if there is at least one open window on screen with headroom to climb onto.
    [[nodiscard]] bool HasClimbableSurfaces(const DesktopWorld& world, int32_t petHeight = 120) const {
        return GetNearestSurface(Point(0, 0), world, petHeight) != nullptr;
    }

    /// Returns true if running in floor-only mode (pure Wayland without readable window surfaces).
    [[nodiscard]] bool IsFloorOnly() const noexcept { return m_isFloorOnly; }

    /// Clears single-frame reaction flags.
    void ResetFrameFlags() noexcept {
        m_surfaceJustMoved = false;
        m_surfaceJustGone = false;
        m_surfaceDelta = {0, 0};
    }

private:
    float m_refreshInterval{0.5f};
    float m_timer{0.0f};
    bool m_initialized{false};

    const WindowSurface* m_currentSurface{nullptr};
    WindowSurface m_trackedSurface{};
    bool m_hadTrackedSurface{false};

    bool m_surfaceJustMoved{false};
    bool m_surfaceJustGone{false};
    Point m_surfaceDelta{0, 0};
    bool m_isFloorOnly{false};

    void CheckSurfaces(const Rect& petBounds, DesktopWorld& world);
};

} // namespace VirtualPet
