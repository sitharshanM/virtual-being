#include "DesktopWorld.h"

#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <gdk/gdk.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <nlohmann/json.hpp>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#undef Status
#undef Success
#undef None
#undef Bool
#endif
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
                wchar_t classBuf[128] = {0};
                ::GetClassNameW(hwnd, classBuf, 127);
                char classUtf8[128] = {0};
                ::WideCharToMultiByte(CP_UTF8, 0, classBuf, -1, classUtf8, sizeof(classUtf8) - 1, nullptr, nullptr);
                WindowSurface ws;
                ws.hwnd = hwnd;
                ws.bounds = Rect(r.left, r.top, w, h);
                ws.title = titleUtf8;
                ws.appClass = classUtf8;
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
    : m_windowSurfaces(std::move(surfaces)), m_primaryWorkArea(workArea), m_virtualScreenBounds(workArea), m_isFixedGeometry(true) {
    MonitorInfo monitor;
    monitor.workArea = monitor.fullArea = workArea;
    monitor.isPrimary = true;
    m_monitors.push_back(monitor);
}

void DesktopWorld::Refresh() {
    if (m_isFixedGeometry) return;
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
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
        int nMonitors = gdk_display_get_n_monitors(display);
        for (int i = 0; i < nMonitors; ++i) {
            GdkMonitor* mon = gdk_display_get_monitor(display, i);
            if (!mon) continue;
            GdkRectangle full{}, work{};
            gdk_monitor_get_geometry(mon, &full);
            gdk_monitor_get_workarea(mon, &work);
            MonitorInfo info;
            info.fullArea = Rect(full.x, full.y, full.width, full.height);
            info.workArea = Rect(work.x, work.y, work.width, work.height);
            info.isPrimary = gdk_monitor_is_primary(mon);
            const char* model = gdk_monitor_get_model(mon);
            info.deviceName = model ? model : ("Monitor " + std::to_string(i));
            m_monitors.push_back(info);
            if (info.isPrimary || i == 0) {
                m_primaryWorkArea = info.workArea;
                m_virtualScreenBounds = info.fullArea;
            }
        }
    }
    if (m_monitors.empty()) {
        m_primaryWorkArea = Rect(0, 0, 1920, 1040);
        m_virtualScreenBounds = Rect(0, 0, 1920, 1080);
        MonitorInfo def;
        def.fullArea = m_virtualScreenBounds;
        def.workArea = m_primaryWorkArea;
        def.isPrimary = true;
        def.deviceName = "Default";
        m_monitors.push_back(def);
    }
    DetectTaskbar();
    RefreshWindowSurfaces();
#endif
}

#ifndef _WIN32
namespace {

bool QueryHyprlandSurfaces(std::vector<WindowSurface>& surfaces, WindowSurface& activeWindow) {
    const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!sig || !*sig) return false;

    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    std::string xdgDir = (xdg && *xdg) ? xdg : ("/run/user/" + std::to_string(getuid()));
    std::string sockPath = xdgDir + "/hypr/" + sig + "/.socket.sock";

    auto sendCmd = [&](const std::string& cmd) -> std::string {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) return "";
        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);
        if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);
            return "";
        }
        if (write(fd, cmd.data(), cmd.size()) < 0) {
            close(fd);
            return "";
        }
        std::string response;
        char buf[4096];
        ssize_t n = 0;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            response.append(buf, static_cast<size_t>(n));
        }
        close(fd);
        return response;
    };

    // Query active / focused window
    std::string activeJsonStr = sendCmd("j/activewindow");
    if (!activeJsonStr.empty()) {
        try {
            auto a = nlohmann::json::parse(activeJsonStr);
            if (a.is_object() && a.contains("title")) {
                activeWindow.title = a.value("title", "");
                activeWindow.appClass = a.value("class", "");
                if (a.contains("at") && a["at"].is_array() && a["at"].size() >= 2) {
                    activeWindow.bounds.x = a["at"][0].get<int>();
                    activeWindow.bounds.y = a["at"][1].get<int>();
                }
                if (a.contains("size") && a["size"].is_array() && a["size"].size() >= 2) {
                    activeWindow.bounds.width = a["size"][0].get<int>();
                    activeWindow.bounds.height = a["size"][1].get<int>();
                }
                std::string addrStr = a.value("address", "");
                if (!addrStr.empty()) {
                    activeWindow.xwin = std::strtoul(addrStr.c_str(), nullptr, 16);
                }
            }
        } catch (...) {}
    }

    std::string wsJsonStr = sendCmd("j/activeworkspace");
    std::string clientsJsonStr = sendCmd("j/clients");
    if (clientsJsonStr.empty()) return false;

    try {
        int activeWsId = -1;
        if (!wsJsonStr.empty()) {
            auto wsJson = nlohmann::json::parse(wsJsonStr);
            if (wsJson.contains("id") && wsJson["id"].is_number()) {
                activeWsId = wsJson["id"].get<int>();
            }
        }

        auto clients = nlohmann::json::parse(clientsJsonStr);
        if (!clients.is_array()) return false;

        bool foundAny = false;
        for (const auto& c : clients) {
            bool mapped = c.value("mapped", false);
            bool hidden = c.value("hidden", false);
            bool pinned = c.value("pinned", false);
            int wsId = -1;
            if (c.contains("workspace") && c["workspace"].contains("id")) {
                wsId = c["workspace"]["id"].get<int>();
            }

            if (!mapped || hidden) continue;
            if (activeWsId != -1 && wsId != activeWsId && !pinned) continue;

            if (!c.contains("at") || !c.contains("size") || !c["at"].is_array() || !c["size"].is_array()) continue;
            int x = c["at"][0].get<int>();
            int y = c["at"][1].get<int>();
            int w = c["size"][0].get<int>();
            int h = c["size"][1].get<int>();

            if (w < 200 || h < 100) continue;

            std::string title = c.value("title", "");
            if (title.empty() || title == "Virtual Pet" || title == "VirtualPet") continue;

            std::string addrStr = c.value("address", "");
            unsigned long xwin = 0;
            if (!addrStr.empty()) {
                xwin = std::strtoul(addrStr.c_str(), nullptr, 16);
            }

            WindowSurface ws;
            ws.xwin = xwin;
            ws.bounds = Rect(x, y, w, h);
            ws.title = title;
            ws.appClass = c.value("class", "");
            surfaces.push_back(ws);
            foundAny = true;
        }
        return foundAny;
    } catch (...) {
        return false;
    }
}

#ifdef HAVE_X11
bool QueryX11Surfaces(std::vector<WindowSurface>& surfaces, unsigned long ignoreXwin, WindowSurface& activeWindow) {
    Display* disp = XOpenDisplay(nullptr);
    if (!disp) return false;

    Window root = DefaultRootWindow(disp);
    Window parent = 0;
    Window* children = nullptr;
    unsigned int nchildren = 0;

    if (!XQueryTree(disp, root, &root, &parent, &children, &nchildren) || !children) {
        XCloseDisplay(disp);
        return false;
    }

    Atom netWmState = XInternAtom(disp, "_NET_WM_STATE", True);
    Atom netWmStateHidden = XInternAtom(disp, "_NET_WM_STATE_HIDDEN", True);
    Atom utf8String = XInternAtom(disp, "UTF8_STRING", True);
    Atom netWmName = XInternAtom(disp, "_NET_WM_NAME", True);
    Atom netActiveWin = XInternAtom(disp, "_NET_ACTIVE_WINDOW", True);

    Window activeXwin = 0;
    if (netActiveWin) {
        Atom actualType;
        int actualFormat;
        unsigned long nitems, bytesAfter;
        unsigned char* prop = nullptr;
        if (XGetWindowProperty(disp, root, netActiveWin, 0, 1, False, XA_WINDOW,
                               &actualType, &actualFormat, &nitems, &bytesAfter, &prop) == 0 && prop) {
            if (nitems > 0) {
                activeXwin = *reinterpret_cast<Window*>(prop);
            }
            XFree(prop);
        }
    }

    bool foundAny = false;
    for (unsigned int i = 0; i < nchildren; ++i) {
        Window win = children[i];
        if (win == ignoreXwin) continue;

        XWindowAttributes attrs;
        if (!XGetWindowAttributes(disp, win, &attrs)) continue;
        if (attrs.map_state != IsViewable) continue;
        if (attrs.override_redirect) continue;

        if (netWmState && netWmStateHidden) {
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* prop = nullptr;
            if (XGetWindowProperty(disp, win, netWmState, 0, 1024, False, XA_ATOM,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &prop) == 0 && prop) {
                Atom* atoms = reinterpret_cast<Atom*>(prop);
                bool isHidden = false;
                for (unsigned long a = 0; a < nitems; ++a) {
                    if (atoms[a] == netWmStateHidden) {
                        isHidden = true;
                        break;
                    }
                }
                XFree(prop);
                if (isHidden) continue;
            }
        }

        std::string title;
        if (netWmName && utf8String) {
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* prop = nullptr;
            if (XGetWindowProperty(disp, win, netWmName, 0, 1024, False, utf8String,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &prop) == 0 && prop) {
                title = reinterpret_cast<char*>(prop);
                XFree(prop);
            }
        }
        if (title.empty()) {
            char* name = nullptr;
            if (XFetchName(disp, win, &name) > 0 && name) {
                title = name;
                XFree(name);
            }
        }

        if (title.empty() || title == "Virtual Pet" || title == "VirtualPet") continue;

        std::string appClass;
        XClassHint classHint;
        if (XGetClassHint(disp, win, &classHint)) {
            if (classHint.res_class) {
                appClass = classHint.res_class;
                XFree(classHint.res_class);
            }
            if (classHint.res_name) {
                XFree(classHint.res_name);
            }
        }

        int destX = 0, destY = 0;
        Window childReturn = 0;
        if (XTranslateCoordinates(disp, win, root, 0, 0, &destX, &destY, &childReturn)) {
            if (attrs.width >= 200 && attrs.height >= 100) {
                WindowSurface ws;
                ws.xwin = static_cast<unsigned long>(win);
                ws.bounds = Rect(destX, destY, attrs.width, attrs.height);
                ws.title = title;
                ws.appClass = appClass;
                surfaces.push_back(ws);
                foundAny = true;

                if (win == activeXwin) {
                    activeWindow = ws;
                }
            }
        }
    }

    if (children) XFree(children);
    XCloseDisplay(disp);
    return foundAny;
}
#endif

} // namespace
#endif

void DesktopWorld::RefreshWindowSurfaces(void* ignoreHwnd) {
    if (m_isFixedGeometry) return;
    m_windowSurfaces.clear();
    m_activeWindow = WindowSurface{};
#ifdef _WIN32
    HWND fg = ::GetForegroundWindow();
    if (fg) {
        RECT r;
        if (::GetWindowRect(fg, &r)) {
            wchar_t titleBuf[256] = {0};
            ::GetWindowTextW(fg, titleBuf, 255);
            wchar_t classBuf[128] = {0};
            ::GetClassNameW(fg, classBuf, 127);
            char titleUtf8[256] = {0};
            char classUtf8[128] = {0};
            ::WideCharToMultiByte(CP_UTF8, 0, titleBuf, -1, titleUtf8, sizeof(titleUtf8) - 1, nullptr, nullptr);
            ::WideCharToMultiByte(CP_UTF8, 0, classBuf, -1, classUtf8, sizeof(classUtf8) - 1, nullptr, nullptr);
            m_activeWindow.hwnd = fg;
            m_activeWindow.bounds = Rect(r.left, r.top, r.right - r.left, r.bottom - r.top);
            m_activeWindow.title = titleUtf8;
            m_activeWindow.appClass = classUtf8;
        }
    }
    WindowEnumParams params;
    params.surfaces = &m_windowSurfaces;
    params.ignoreHwnd = static_cast<HWND>(ignoreHwnd);
    ::EnumWindows(WindowEnumProc, reinterpret_cast<LPARAM>(&params));
#else
    if (QueryHyprlandSurfaces(m_windowSurfaces, m_activeWindow)) {
        return;
    }
#ifdef HAVE_X11
    if (std::getenv("DISPLAY")) {
        unsigned long ignoreWin = ignoreHwnd ? reinterpret_cast<uintptr_t>(ignoreHwnd) : 0;
        QueryX11Surfaces(m_windowSurfaces, ignoreWin, m_activeWindow);
    }
#else
    (void)ignoreHwnd;
#endif
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
#else
    if (m_virtualScreenBounds.height > m_primaryWorkArea.height) {
        m_taskbar.isVisible = true;
        m_taskbar.edge = TaskbarEdge::Bottom;
        m_taskbar.rect = Rect(m_primaryWorkArea.x,
                              m_primaryWorkArea.y + m_primaryWorkArea.height,
                              m_primaryWorkArea.width,
                              m_virtualScreenBounds.height - m_primaryWorkArea.height);
        return;
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
        // Edge Case: Window top must have clearance on screen to physically support the pet!
        // If window is at the top edge of the screen, the pet cannot stand on top of it.
        if (ws.bounds.y - height < area.y) {
            continue;
        }

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
