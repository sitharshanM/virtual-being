#include "DesktopWatcher.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace VirtualPet {

DesktopWatcher::DesktopWatcher() {
#ifndef _WIN32
    const char* disp = std::getenv("DISPLAY");
    const char* hypr = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if ((!disp || !*disp) && (!hypr || !*hypr) && (wayland && *wayland)) {
        m_isFloorOnly = true;
    }
#endif
}

void DesktopWatcher::Update(float deltaTime, const Rect& petBounds, DesktopWorld& world) {
    m_timer += deltaTime;
    if (!m_initialized || m_timer >= m_refreshInterval) {
        m_initialized = true;
        m_timer = 0.0f;
        world.RefreshWindowSurfaces();
        CheckSurfaces(petBounds, world);
    } else {
        const auto& surfaces = world.GetWindowSurfaces();
        m_currentSurface = nullptr;
        int32_t petFootY = petBounds.y + petBounds.height;
        int32_t petCenterX = petBounds.x + petBounds.width / 2;

        for (const auto& ws : surfaces) {
            if (std::abs(petFootY - ws.bounds.y) <= 6 &&
                petCenterX >= ws.bounds.x && petCenterX <= ws.bounds.x + ws.bounds.width) {
                m_currentSurface = &ws;
                break;
            }
        }
    }
}

void DesktopWatcher::CheckSurfaces(const Rect& petBounds, DesktopWorld& world) {
    const auto& surfaces = world.GetWindowSurfaces();

#ifndef _WIN32
    const char* disp = std::getenv("DISPLAY");
    const char* hypr = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if ((!disp || !*disp) && (!hypr || !*hypr)) {
        m_isFloorOnly = surfaces.empty();
    }
#endif

    int32_t petFootY = petBounds.y + petBounds.height;
    int32_t petCenterX = petBounds.x + petBounds.width / 2;

    const WindowSurface* matchedSurface = nullptr;
    for (const auto& ws : surfaces) {
        if (std::abs(petFootY - ws.bounds.y) <= 6 &&
            petCenterX >= ws.bounds.x && petCenterX <= ws.bounds.x + ws.bounds.width) {
            matchedSurface = &ws;
            break;
        }
    }

    if (m_hadTrackedSurface) {
        const WindowSurface* stillExists = nullptr;
        for (const auto& ws : surfaces) {
#ifdef _WIN32
            if (ws.hwnd != nullptr && ws.hwnd == m_trackedSurface.hwnd) {
                stillExists = &ws;
                break;
            }
#else
            if (ws.xwin != 0 && ws.xwin == m_trackedSurface.xwin) {
                stillExists = &ws;
                break;
            }
#endif
        }

        if (!stillExists) {
            m_surfaceJustGone = true;
            m_surfaceJustMoved = false;
            m_surfaceDelta = {0, 0};
            m_hadTrackedSurface = false;
            m_currentSurface = nullptr;
        } else {
            int32_t dx = stillExists->bounds.x - m_trackedSurface.bounds.x;
            int32_t dy = stillExists->bounds.y - m_trackedSurface.bounds.y;

            if (dx != 0 || dy != 0) {
                m_surfaceJustMoved = true;
                m_surfaceDelta = {dx, dy};
            }

            m_trackedSurface = *stillExists;
            m_currentSurface = stillExists;
        }
    } else {
        if (matchedSurface) {
            m_hadTrackedSurface = true;
            m_trackedSurface = *matchedSurface;
            m_currentSurface = matchedSurface;
        } else {
            m_currentSurface = nullptr;
        }
    }
}

const WindowSurface* DesktopWatcher::GetNearestSurface(const Point& point, const DesktopWorld& world, int32_t petHeight) const {
    const auto& surfaces = world.GetWindowSurfaces();
    if (surfaces.empty()) return nullptr;

    const Rect workArea = world.GetPrimaryWorkArea();
    const WindowSurface* closest = nullptr;
    float minDistanceSq = 1e12f;

    for (const auto& ws : surfaces) {
        // Edge Case Fix: Do not target windows that have no headroom above them (e.g. maximized or touching ceiling)
        if (ws.bounds.y - petHeight < workArea.y + 10) {
            continue;
        }

        int32_t minX = ws.bounds.x + 15;
        int32_t maxX = ws.bounds.x + (std::max)(15, ws.bounds.width - 15);
        float targetX = static_cast<float>(std::clamp(point.x, minX, maxX));
        float targetY = static_cast<float>(ws.bounds.y);

        float dx = targetX - static_cast<float>(point.x);
        float dy = targetY - static_cast<float>(point.y);
        float distSq = dx * dx + dy * dy;

        if (distSq < minDistanceSq) {
            minDistanceSq = distSq;
            closest = &ws;
        }
    }

    return closest;
}

} // namespace VirtualPet
