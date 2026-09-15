#pragma once

#include "MouseSensor.h"

#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace VirtualPet {

/// Information about a connected display monitor.
struct MonitorInfo {
    Rect fullArea{0, 0, 0, 0};        ///< Full display resolution rectangle.
    Rect workArea{0, 0, 0, 0};        ///< Desktop work area excluding taskbars and docks.
    bool isPrimary{false};            ///< True if this is the Windows primary display.
    std::string deviceName;           ///< Adapter/monitor name.
};

/// Edge where the primary desktop taskbar is docked.
enum class TaskbarEdge {
    Bottom,
    Top,
    Left,
    Right,
    Unknown
};

/// Information regarding the Windows taskbar.
struct TaskbarInfo {
    Rect rect{0, 0, 0, 0};
    TaskbarEdge edge{TaskbarEdge::Bottom};
    bool isVisible{true};
};

/// Surface representation of an open application window on the desktop.
struct WindowSurface {
#ifdef _WIN32
    HWND hwnd{nullptr};
#endif
    Rect bounds{0, 0, 0, 0};
    std::string title;
};

/**
 * @brief Discovers and tracks desktop geometry, monitor work areas, taskbar, and open window surfaces.
 */
class DesktopWorld {
public:
    DesktopWorld();
    /// Creates a fixed geometry snapshot for simulations and deterministic tests.
    DesktopWorld(Rect workArea, std::vector<WindowSurface> surfaces);
    ~DesktopWorld() = default;

    /// Refreshes monitor geometry, taskbar positions, and visible window surfaces.
    void Refresh();

    /// Refreshes open application window bounds on the desktop.
    void RefreshWindowSurfaces(void* ignoreHwnd = nullptr);

    /// Returns the primary monitor's work area rectangle.
    [[nodiscard]] Rect GetPrimaryWorkArea() const noexcept { return m_primaryWorkArea; }

    /// Returns the bounding rectangle enclosing all connected monitors.
    [[nodiscard]] Rect GetVirtualScreenBounds() const noexcept { return m_virtualScreenBounds; }

    /// Returns the list of all detected monitors.
    [[nodiscard]] const std::vector<MonitorInfo>& GetMonitors() const noexcept { return m_monitors; }

    /// Finds the monitor that contains the specified point.
    [[nodiscard]] const MonitorInfo* GetMonitorAt(const Point& pt) const;

    /// Returns information about the desktop taskbar.
    [[nodiscard]] const TaskbarInfo& GetTaskbarInfo() const noexcept { return m_taskbar; }

    /// Returns detected open window surfaces.
    [[nodiscard]] const std::vector<WindowSurface>& GetWindowSurfaces() const noexcept { return m_windowSurfaces; }

    /// Clamps an entity's bounding box so it remains inside the desktop work area.
    [[nodiscard]] Point ClampToWorkArea(const Point& position, int32_t width, int32_t height) const noexcept;

    /// Calculates the desktop baseline ground Y-coordinate for an entity at the given X position.
    [[nodiscard]] int32_t GetGroundY(int32_t x, int32_t height, int32_t groundMargin = 0) const noexcept;

    /// Calculates the highest supporting surface Y beneath the pet (either a window top edge or desktop ground).
    [[nodiscard]] int32_t GetSupportingSurfaceY(int32_t x, int32_t currentY, int32_t width, int32_t height, int32_t groundMargin = 0) const noexcept;

    /// Returns true if an entity's bottom edge is touching or below a solid surface.
    [[nodiscard]] bool IsOnGround(int32_t x, int32_t y, int32_t height, int32_t groundMargin = 4) const noexcept;

private:
    std::vector<MonitorInfo> m_monitors;
    std::vector<WindowSurface> m_windowSurfaces;
    Rect m_primaryWorkArea{0, 0, 1920, 1080};
    Rect m_virtualScreenBounds{0, 0, 1920, 1080};
    TaskbarInfo m_taskbar;

    void DetectTaskbar();
};

} // namespace VirtualPet
