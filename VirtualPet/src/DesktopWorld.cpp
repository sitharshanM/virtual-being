#include "DesktopWorld.h"

#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace VirtualPet {

#ifdef _WIN32
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    (void)hdcMonitor;
    (void)lprcMonitor;

    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEXW mi;
    mi.cbSize = sizeof(MONITORINFOEXW);
    if (::GetMonitorInfoW(hMonitor, &mi)) {
        MonitorInfo info;
        info.fullArea = Rect(mi.rcMonitor.left, mi.rcMonitor.top,
                             mi.rcMonitor.right - mi.rcMonitor.left,
                             mi.rcMonitor.bottom - mi.rcMonitor.top);
        info.workArea = Rect(mi.rcWork.left, mi.rcWork.top,
                             mi.rcWork.right - mi.rcWork.left,
                             mi.rcWork.bottom - mi.rcWork.top);
        info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

        char nameBuffer[64] = {0};
        ::WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, nameBuffer, sizeof(nameBuffer) - 1, nullptr, nullptr);
        info.deviceName = nameBuffer;

        monitors->push_back(info);
    }
    return TRUE;
}

struct WindowEnumParams {
    std::vector<WindowSurface>* surfaces;
    HWND ignoreHwnd;
};

static BOOL CALLBACK WindowEnumProc(HWND hwnd, LPARAM lParam) {
    auto* params = reinterpret_cast<WindowEnumParams*>(lParam);
    if (hwnd == params->ignoreHwnd) return TRUE;
    if (!::IsWindowVisible(hwnd) || ::IsIconic(hwnd)) return TRUE;

    LONG exStyle = ::GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    RECT r;
    if (::GetWindowRect(hwnd, &r)) {
        int32_t w = r.right - r.left;
        int32_t h = r.bottom - r.top;
        if (w >= 200 && h >= 150) {
            wchar_t titleBuf[128] = {0};
            ::GetWindowTextW(hwnd, titleBuf, 127);
            if (wcslen(titleBuf) > 0 && wcscmp(titleBuf, L"Virtual Pet") != 0) {
                char titleUtf8[256] = {0};
                ::WideCharToMultiByte(CP_UTF8, 0, titleBuf, -1, titleUtf8, sizeof(titleUtf8) - 1, nullptr, nullptr);
                WindowSurface ws;
                ws.hwnd = hwnd;
                ws.bounds = Rect(r.left, r.top, w, h);
                ws.title = titleUtf8;
                params->surfaces->push_back(ws);
            }
        }
    }
    return TRUE;
}
#endif

DesktopWorld::DesktopWorld() {
    Refresh();
}

DesktopWorld::DesktopWorld(Rect workArea, std::vector<WindowSurface> surfaces)
    : m_windowSurfaces(std::move(surfaces)), m_primaryWorkArea(workArea), m_virtualScreenBounds(workArea) {
    MonitorInfo monitor;
    monitor.workArea = monitor.fullArea = workArea;
    monitor.isPrimary = true;
    m_monitors.push_back(monitor);
}

void DesktopWorld::Refresh() {
    m_monitors.clear();

#ifdef _WIN32
    ::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&m_monitors));

    RECT primaryWork;
    if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &primaryWork, 0)) {
        m_primaryWorkArea = Rect(primaryWork.left, primaryWork.top,
                                 primaryWork.right - primaryWork.left,
                                 primaryWork.bottom - primaryWork.top);
    }

    int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    m_virtualScreenBounds = Rect(vx, vy, vw, vh);

    DetectTaskbar();
    RefreshWindowSurfaces();
#else
    m_primaryWorkArea = Rect(0, 0, 1920, 1040);
    m_virtualScreenBounds = Rect(0, 0, 1920, 1080);
#endif
}

void DesktopWorld::RefreshWindowSurfaces(void* ignoreHwnd) {
    m_windowSurfaces.clear();
#ifdef _WIN32
    WindowEnumParams params;
    params.surfaces = &m_windowSurfaces;
    params.ignoreHwnd = static_cast<HWND>(ignoreHwnd);
    ::EnumWindows(WindowEnumProc, reinterpret_cast<LPARAM>(&params));
#endif
}

void DesktopWorld::DetectTaskbar() {
#ifdef _WIN32
    HWND taskbarHwnd = ::FindWindowW(L"Shell_TrayWnd", nullptr);
    if (taskbarHwnd && ::IsWindowVisible(taskbarHwnd)) {
        RECT r;
        if (::GetWindowRect(taskbarHwnd, &r)) {
            m_taskbar.rect = Rect(r.left, r.top, r.right - r.left, r.bottom - r.top);
            m_taskbar.isVisible = true;

            int screenW = ::GetSystemMetrics(SM_CXSCREEN);
            int screenH = ::GetSystemMetrics(SM_CYSCREEN);

            if (r.top > screenH / 2) {
                m_taskbar.edge = TaskbarEdge::Bottom;
            } else if (r.bottom < screenH / 2) {
                m_taskbar.edge = TaskbarEdge::Top;
            } else if (r.left < screenW / 2) {
                m_taskbar.edge = TaskbarEdge::Left;
            } else {
                m_taskbar.edge = TaskbarEdge::Right;
            }
            return;
        }
    }
#endif
    m_taskbar.isVisible = false;
    m_taskbar.edge = TaskbarEdge::Unknown;
}

const MonitorInfo* DesktopWorld::GetMonitorAt(const Point& pt) const {
    for (const auto& monitor : m_monitors) {
        if (monitor.fullArea.Contains(pt)) {
            return &monitor;
        }
    }
    for (const auto& monitor : m_monitors) {
        if (monitor.isPrimary) {
            return &monitor;
        }
    }
    return m_monitors.empty() ? nullptr : &m_monitors[0];
}

Point DesktopWorld::ClampToWorkArea(const Point& position, int32_t width, int32_t height) const noexcept {
    const MonitorInfo* mon = GetMonitorAt(position);
    Rect area = mon ? mon->workArea : m_primaryWorkArea;

    int32_t minX = area.x;
    int32_t maxX = area.x + area.width - width;
    int32_t minY = area.y;
    int32_t maxY = area.y + area.height - height;

    Point clamped;
    clamped.x = std::clamp(position.x, minX, (std::max)(minX, maxX));
    clamped.y = std::clamp(position.y, minY, (std::max)(minY, maxY));
    return clamped;
}

int32_t DesktopWorld::GetGroundY(int32_t x, int32_t height, int32_t groundMargin) const noexcept {
    const MonitorInfo* mon = GetMonitorAt(Point(x, m_primaryWorkArea.y));
    Rect area = mon ? mon->workArea : m_primaryWorkArea;
    return area.y + area.height - height - groundMargin;
}

int32_t DesktopWorld::GetSupportingSurfaceY(int32_t x, int32_t currentY, int32_t width, int32_t height, int32_t groundMargin) const noexcept {
    const auto* monitor = GetMonitorAt(Point(x, currentY));
    const Rect area = monitor ? monitor->workArea : m_primaryWorkArea;
    int32_t bestGroundY = area.y + area.height - height - groundMargin;

    // Check if pet can land on top of any open application window
    for (const auto& ws : m_windowSurfaces) {
        // Horizontal check: does pet overlap the window width?
        bool xOverlap = (x + width > ws.bounds.x) && (x < ws.bounds.x + ws.bounds.width);
        if (xOverlap) {
            int32_t windowTopY = ws.bounds.y - height;
            // If the surface is beneath the pet or within landing reach
            if (windowTopY >= (currentY - 10) && windowTopY < bestGroundY) {
                bestGroundY = windowTopY;
            }
        }
    }

    return bestGroundY;
}

bool DesktopWorld::IsOnGround(int32_t x, int32_t y, int32_t height, int32_t groundMargin) const noexcept {
    int32_t groundY = GetGroundY(x, height, groundMargin);
    return y >= groundY;
}

} // namespace VirtualPet
