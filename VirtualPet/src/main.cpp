#include "Pet.h"
#include "SaveManager.h"

#include <chrono>
#include <memory>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#endif

namespace {

constexpr const wchar_t* WINDOW_CLASS_NAME = L"VirtualPetWindowClass";
constexpr const wchar_t* WINDOW_TITLE = L"Virtual Pet";

// Transparent Color Key (pure magenta)
constexpr COLORREF TRANSPARENT_COLOR_KEY = RGB(255, 0, 255);

// Custom Window Messages
constexpr UINT WM_TRAYICON = WM_APP + 1;

// Context / Tray Menu IDs
constexpr UINT_PTR ID_MENU_TOGGLE_VISIBLE = 1000;
constexpr UINT_PTR ID_MENU_STATUS         = 1001;
constexpr UINT_PTR ID_MENU_FEED           = 1002;
constexpr UINT_PTR ID_MENU_PLAY           = 1003;
constexpr UINT_PTR ID_MENU_DROP_TOY       = 1004;
constexpr UINT_PTR ID_MENU_DROP_TREAT     = 1005;
constexpr UINT_PTR ID_MENU_SLEEP          = 1006;
constexpr UINT_PTR ID_MENU_AUTOSTART      = 1007;
constexpr UINT_PTR ID_MENU_EXIT           = 1008;

std::unique_ptr<VirtualPet::Pet> g_pet = nullptr;
NOTIFYICONDATAW g_nid{};
bool g_isPetVisible = true;

void ShowPetStatusDialog(HWND hwnd) {
    if (!g_pet) return;
    const auto& needs = g_pet->GetMemory().GetNeeds();
    const auto& name = g_pet->GetMemory().GetName();
    const auto& thought = g_pet->GetBrain().GetThought();

    wchar_t statusMsg[512] = {0};
    swprintf_s(statusMsg, 512,
               L"Companion: %hs\n\n"
               L"Energy: %d%%\n"
               L"Mood: %d%%\n"
               L"Hunger: %d%%\n"
               L"Trust: %d%%\n\n"
               L"Inner Thought:\n\"%hs\"",
               name.c_str(),
               static_cast<int>(needs.energy * 100.0f),
               static_cast<int>(needs.mood * 100.0f),
               static_cast<int>(needs.hunger * 100.0f),
               static_cast<int>(needs.trust * 100.0f),
               thought.c_str());

    ::MessageBoxW(hwnd, statusMsg, L"Virtual Pet Status", MB_OK | MB_ICONINFORMATION);
}

void ShowPetContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = ::CreatePopupMenu();
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_TOGGLE_VISIBLE, g_isPetVisible ? L"&Hide Pet" : L"&Show Pet");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_STATUS, L"Pet &Status...");
    ::AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_FEED, L"&Feed Snack");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_PLAY, L"&Play Petting");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_DROP_TOY, L"Drop &Ball Toy");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_DROP_TREAT, L"Drop &Treat");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_SLEEP, L"&Sleep Nap");
    ::AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    bool autostart = VirtualPet::SaveManager::IsStartWithWindowsEnabled();
    UINT autostartFlags = MF_STRING | (autostart ? MF_CHECKED : MF_UNCHECKED);
    ::AppendMenuW(hMenu, autostartFlags, ID_MENU_AUTOSTART, L"Start With &Windows");

    ::AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"E&xit");

    ::SetForegroundWindow(hwnd);
    ::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    ::DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            ::SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR_KEY, 0, LWA_COLORKEY);

            // Register Windows System Tray Icon
            ZeroMemory(&g_nid, sizeof(NOTIFYICONDATAW));
            g_nid.cbSize = sizeof(NOTIFYICONDATAW);
            g_nid.hWnd = hwnd;
            g_nid.uID = 1;
            g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            g_nid.uCallbackMessage = WM_TRAYICON;
            g_nid.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
            wcscpy_s(g_nid.szTip, 128, L"Virtual Pet - Astra");
            ::Shell_NotifyIconW(NIM_ADD, &g_nid);
            return 0;
        }

        case WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                POINT pt;
                ::GetCursorPos(&pt);
                ShowPetContextMenu(hwnd, pt);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);

            RECT clientRect;
            ::GetClientRect(hwnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            // Double-buffered rendering to eliminate flicker
            HDC memDC = ::CreateCompatibleDC(hdc);
            HBITMAP memBmp = ::CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ oldBmp = ::SelectObject(memDC, memBmp);

            // Clear background with magenta transparent color key
            HBRUSH transBrush = ::CreateSolidBrush(TRANSPARENT_COLOR_KEY);
            ::FillRect(memDC, &clientRect, transBrush);
            ::DeleteObject(transBrush);

            // Render pet & toys
            if (g_pet && g_isPetVisible) {
                g_pet->Render(memDC);
            }

            // Blit buffer to window
            ::BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

            ::SelectObject(memDC, oldBmp);
            ::DeleteObject(memBmp);
            ::DeleteDC(memDC);

            ::EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            ::SetCapture(hwnd);
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            if (g_pet) {
                g_pet->OnLButtonDown(VirtualPet::Point(pt.x, pt.y));
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            if (g_pet) {
                g_pet->OnMouseMove(VirtualPet::Point(pt.x, pt.y));
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            ::ReleaseCapture();
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            if (g_pet) {
                g_pet->OnLButtonUp(VirtualPet::Point(pt.x, pt.y));
            }
            return 0;
        }

        case WM_RBUTTONUP: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            ShowPetContextMenu(hwnd, pt);
            if (g_pet) {
                g_pet->OnRButtonUp(VirtualPet::Point(pt.x, pt.y));
            }
            return 0;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_MENU_TOGGLE_VISIBLE:
                    g_isPetVisible = !g_isPetVisible;
                    ::ShowWindow(hwnd, g_isPetVisible ? SW_SHOW : SW_HIDE);
                    break;
                case ID_MENU_STATUS:
                    ShowPetStatusDialog(hwnd);
                    break;
                case ID_MENU_FEED:
                    if (g_pet) g_pet->Feed();
                    break;
                case ID_MENU_PLAY:
                    if (g_pet) g_pet->Play();
                    break;
                case ID_MENU_DROP_TOY:
                    if (g_pet) g_pet->DropToy(false);
                    break;
                case ID_MENU_DROP_TREAT:
                    if (g_pet) g_pet->DropToy(true);
                    break;
                case ID_MENU_SLEEP:
                    if (g_pet) g_pet->Sleep();
                    break;
                case ID_MENU_AUTOSTART: {
                    bool current = VirtualPet::SaveManager::IsStartWithWindowsEnabled();
                    VirtualPet::SaveManager::SetStartWithWindows(!current);
                    break;
                }
                case ID_MENU_EXIT:
                    ::DestroyWindow(hwnd);
                    break;
                default:
                    break;
            }
            return 0;
        }

        case WM_DESTROY: {
            ::Shell_NotifyIconW(NIM_DELETE, &g_nid);
            ::PostQuitMessage(0);
            return 0;
        }

        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

int RunApplication(HINSTANCE hInstance) {
    // 1. Initialize Pet system
    g_pet = std::make_unique<VirtualPet::Pet>();
    g_pet->Init();

    // 2. Register Window Class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!::RegisterClassExW(&wc)) {
        return 1;
    }

    // 3. Create transparent, borderless, topmost popup window
    VirtualPet::Point initialPos = g_pet->GetPosition();
    int width = g_pet->GetWidth();
    int height = g_pet->GetHeight();

    HWND hwnd = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_POPUP,
        initialPos.x, initialPos.y,
        width, height,
        nullptr, nullptr,
        hInstance, nullptr
    );

    if (!hwnd) {
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    // 4. Main simulation loop at target ~60 FPS
    using Clock = std::chrono::high_resolution_clock;
    auto prevTime = Clock::now();
    bool running = true;
    MSG msg{};

    while (running) {
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        if (!running) break;

        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
        prevTime = currentTime;

        // Step pet simulation
        if (g_pet) {
            g_pet->Update(deltaTime);

            if (g_isPetVisible) {
                // Synchronize window location on the desktop with physics position
                VirtualPet::Point pos = g_pet->GetPosition();
                ::SetWindowPos(hwnd, HWND_TOPMOST, pos.x, pos.y, width, height,
                               SWP_NOACTIVATE | SWP_NOZORDER);

                // Request frame repaint
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        // Frame rate limiter (~60 FPS -> 16 ms)
        ::Sleep(16);
    }

    g_pet.reset();
    return static_cast<int>(msg.wParam);
}

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    return RunApplication(hInstance);
}

int main(int, char*[]) {
    return RunApplication(::GetModuleHandle(nullptr));
}
#else
int main() {
    return 0;
}
#endif
