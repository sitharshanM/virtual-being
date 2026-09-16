#include "Pet.h"
#include "SaveManager.h"

#include <chrono>
#include <memory>
#include <string>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

namespace {

constexpr const wchar_t* WINDOW_CLASS_NAME = L"VirtualPetWindowClass";
constexpr const wchar_t* WINDOW_TITLE = L"Virtual Companion";

// Transparent Color Key (pure magenta)
constexpr COLORREF TRANSPARENT_COLOR_KEY = RGB(255, 0, 255);

// Custom Window Messages
constexpr UINT WM_TRAYICON = WM_APP + 1;

// Context / Tray Menu IDs
constexpr UINT_PTR ID_MENU_TOGGLE_VISIBLE = 1000;
constexpr UINT_PTR ID_MENU_STATUS         = 1001;
constexpr UINT_PTR ID_MENU_HEADPAT        = 1002;
constexpr UINT_PTR ID_MENU_FEED           = 1003;
constexpr UINT_PTR ID_MENU_DROP_TOY       = 1004;
constexpr UINT_PTR ID_MENU_STUDY          = 1005;
constexpr UINT_PTR ID_MENU_AUTOSTART      = 1007;
constexpr UINT_PTR ID_MENU_EXIT           = 1008;
constexpr int ID_CHAT_HISTORY = 2001;
constexpr int ID_CHAT_INPUT = 2002;
constexpr int ID_CHAT_SEND = 2003;
constexpr int ID_CHAT_DIARY = 2004;

std::unique_ptr<VirtualPet::Pet> g_pet = nullptr;
NOTIFYICONDATAW g_nid{};
bool g_isPetVisible = true;
HWND g_toyWindow = nullptr;
HWND g_bubbleWindow = nullptr;
std::wstring g_bubbleText;
HWND g_chatWindow = nullptr;
HWND g_chatHistory = nullptr;
HWND g_chatInput = nullptr;

void ShowChatPanel();

std::wstring Utf8ToWide(const std::string& value) {
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) return L"Virtual Companion";
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

LRESULT CALLBACK ToyWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rect; GetClientRect(hwnd, &rect);
        HBRUSH background = CreateSolidBrush(TRANSPARENT_COLOR_KEY);
        FillRect(dc, &rect, background); DeleteObject(background);
        if (g_pet) {
            const auto& toy = g_pet->GetPhysics().GetToy();
            if (toy.isTreat) {
                // Draw warm Coffee / Boba cup
                HBRUSH cupBrush = CreateSolidBrush(RGB(210, 160, 110));
                HBRUSH lidBrush = CreateSolidBrush(RGB(245, 240, 230));
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(140, 90, 50));
                auto oldB = SelectObject(dc, cupBrush);
                auto oldP = SelectObject(dc, pen);
                // Cup body
                RoundRect(dc, 4, 8, rect.right - 4, rect.bottom - 2, 6, 6);
                // Lid
                SelectObject(dc, lidBrush);
                RoundRect(dc, 2, 4, rect.right - 2, 10, 3, 3);
                // Straw
                HPEN strawPen = CreatePen(PS_SOLID, 2, RGB(100, 200, 120));
                SelectObject(dc, strawPen);
                MoveToEx(dc, rect.right / 2 + 3, 4, nullptr);
                LineTo(dc, rect.right / 2 + 6, 0);
                SelectObject(dc, oldB); SelectObject(dc, oldP);
                DeleteObject(cupBrush); DeleteObject(lidBrush); DeleteObject(pen); DeleteObject(strawPen);
            } else {
                // Draw cute pink Plushie Heart
                HBRUSH heartBrush = CreateSolidBrush(RGB(255, 110, 165));
                HPEN heartPen = CreatePen(PS_SOLID, 1, RGB(220, 60, 120));
                auto oldB = SelectObject(dc, heartBrush);
                auto oldP = SelectObject(dc, heartPen);
                int r = (rect.right - rect.left) / 2;
                Ellipse(dc, 2, 2, r + 2, r + 2);
                Ellipse(dc, r - 2, 2, rect.right - 2, r + 2);
                POINT triangle[3] = {
                    {2, r / 2 + 3},
                    {rect.right - 2, r / 2 + 3},
                    {rect.right / 2, rect.bottom - 2}
                };
                Polygon(dc, triangle, 3);
                SelectObject(dc, oldB); SelectObject(dc, oldP);
                DeleteObject(heartBrush); DeleteObject(heartPen);
            }
        }
        EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK BubbleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_LBUTTONUP) { ShowChatPanel(); return 0; }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT client{}; GetClientRect(hwnd, &client);
        HBRUSH clear = CreateSolidBrush(TRANSPARENT_COLOR_KEY); FillRect(dc, &client, clear); DeleteObject(clear);
        HBRUSH bubble = CreateSolidBrush(RGB(255, 249, 252));
        HPEN outline = CreatePen(PS_SOLID, 2, RGB(116, 72, 91));
        auto oldBrush = SelectObject(dc, bubble); auto oldPen = SelectObject(dc, outline);
        RoundRect(dc, 3, 3, client.right - 4, client.bottom - 14, 18, 18);
        POINT tail[3]{{client.right - 55, client.bottom - 15},{client.right - 30, client.bottom - 2},{client.right - 35, client.bottom - 17}};
        Polygon(dc, tail, 3);
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(bubble); DeleteObject(outline);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(42, 29, 36));
        RECT textRect{16, 12, client.right - 16, client.bottom - 23};
        DrawTextW(dc, g_bubbleText.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
        EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void RefreshChatHistory() {
    if (!g_pet || !g_chatHistory) return;
    std::wstring text = Utf8ToWide(g_pet->GetConversationText());
    if (g_pet->IsAiTyping()) text += L"Astra is thinking locally...";
    SetWindowTextW(g_chatHistory, text.c_str());
    SendMessageW(g_chatHistory, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
    SendMessageW(g_chatHistory, EM_SCROLLCARET, 0, 0);
}

LRESULT CALLBACK ChatWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            g_chatHistory = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_VSCROLL,
                12,12,456,300,hwnd,reinterpret_cast<HMENU>(ID_CHAT_HISTORY),nullptr,nullptr);
            g_chatInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                12,326,354,30,hwnd,reinterpret_cast<HMENU>(ID_CHAT_INPUT),nullptr,nullptr);
            CreateWindowW(L"BUTTON",L"Send",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,376,326,92,30,hwnd,reinterpret_cast<HMENU>(ID_CHAT_SEND),nullptr,nullptr);
            CreateWindowW(L"BUTTON",L"Diary",WS_CHILD|WS_VISIBLE,376,364,92,28,hwnd,reinterpret_cast<HMENU>(ID_CHAT_DIARY),nullptr,nullptr);
            SetTimer(hwnd,1,500,nullptr); RefreshChatHistory(); return 0;
        case WM_COMMAND:
            if (LOWORD(wp)==ID_CHAT_SEND && g_pet) {
                wchar_t buffer[1024]{}; GetWindowTextW(g_chatInput,buffer,1024);
                std::string message; int size=WideCharToMultiByte(CP_UTF8,0,buffer,-1,nullptr,0,nullptr,nullptr);
                if(size>1){message.resize(size);WideCharToMultiByte(CP_UTF8,0,buffer,-1,message.data(),size,nullptr,nullptr);message.pop_back();}
                if(g_pet->SendChatMessage(message)){SetWindowTextW(g_chatInput,L"");RefreshChatHistory();}
                else if(g_pet->IsAiTyping()) MessageBoxW(hwnd,L"Astra is still thinking. Give her a moment.",L"Astra",MB_OK);
                return 0;
            }
            if (LOWORD(wp)==ID_CHAT_DIARY && g_pet) MessageBoxW(hwnd,Utf8ToWide(g_pet->GetDiaryText()).c_str(),L"Astra's Diary",MB_OK);
            break;
        case WM_TIMER: RefreshChatHistory(); return 0;
        case WM_CLOSE: ShowWindow(hwnd,SW_HIDE); return 0;
        case WM_DESTROY: g_chatWindow=nullptr; g_chatHistory=nullptr; g_chatInput=nullptr; break;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

void ShowChatPanel() {
    if (!g_chatWindow) return;
    ShowWindow(g_chatWindow,SW_SHOW); SetForegroundWindow(g_chatWindow); SetFocus(g_chatInput); RefreshChatHistory();
}

void ShowPetStatusDialog(HWND hwnd) {
    if (!g_pet) return;
    const auto& needs = g_pet->GetMemory().GetNeeds();
    const auto& name = g_pet->GetMemory().GetName();
    const auto& history = g_pet->GetMemory().GetHistory();
    const auto& identity = g_pet->GetMemory().GetIdentity();
    const auto& thought = g_pet->GetBrain().GetThought();
    std::string tierTitle = g_pet->GetMemory().GetRelationshipTitle();
    std::string moodTitle = g_pet->GetMemory().GetMoodTitle();
    std::string llmStatus = g_pet->GetLocalLlmStatus();

    wchar_t statusMsg[2048] = {0};
    swprintf_s(statusMsg, 2048,
               L"🌸 Companion: %hs\n"
               L"💕 Relationship: %hs (Affection: %d%%)\n\n"
               L"✨ Mood: %hs (%d%%)\n"
               L"🫧 Comfort: %d%%\n"
               L"💤 Beauty Rest: %d%%\n"
               L"☕ Coffee / Boba Craving: %d%%\n\n"
               L"🎧 Her own world:\n"
               L"  • Loves: %hs\n"
               L"  • Usually listening to: %hs\n"
               L"  • Imperfection: %hs\n"
               L"  • Goal: %hs (%d%%)\n\n"
               L"📜 Our Diary:\n"
               L"  • Headpats: %d\n"
               L"  • Coffee Dates: %d\n"
               L"  • Study Sessions: %d\n"
               L"  • Honest boundaries: %d\n"
               L"  • Days Together: %d\n\n"
               L"🧠 Local AI: %hs\n\n"
               L"💭 Current Thought:\n"
               L"\"%hs\"",
               name.c_str(),
               tierTitle.c_str(),
               static_cast<int>(needs.GetAffection() * 100.0f),
               moodTitle.c_str(),
               static_cast<int>(needs.mood * 100.0f),
               static_cast<int>(needs.comfort * 100.0f),
               static_cast<int>(needs.energy * 100.0f),
               static_cast<int>(needs.GetCoffeeCraving() * 100.0f),
               identity.favoriteActivity.c_str(),
               identity.favoriteMusic.c_str(),
               identity.imperfection.c_str(),
               identity.personalGoal.c_str(),
               static_cast<int>(identity.goalProgress * 100.0f),
               history.timesPetted,
               history.timesFed,
               history.studySessionsTogether,
               history.boundariesExpressed,
               history.daysTogether,
               llmStatus.c_str(),
               thought.c_str());

    ::MessageBoxW(hwnd, statusMsg, L"Relationship Diary", MB_OK | MB_ICONINFORMATION);
}

void ShowPetContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = ::CreatePopupMenu();
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_TOGGLE_VISIBLE, g_isPetVisible ? L"&Hide Companion" : L"&Show Companion");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_STATUS, L"&Relationship Diary & Status... 💌");
    ::AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_HEADPAT, L"&Give Headpat ❤️");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_FEED, L"Send &Coffee / Boba Break ☕");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_DROP_TOY, L"Toss &Plushie Heart 🧸");
    ::AppendMenuW(hMenu, MF_STRING, ID_MENU_STUDY, L"&Study Together (Focus Mode) 📖");
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
            // Register Windows System Tray Icon
            ZeroMemory(&g_nid, sizeof(NOTIFYICONDATAW));
            g_nid.cbSize = sizeof(NOTIFYICONDATAW);
            g_nid.hWnd = hwnd;
            g_nid.uID = 1;
            g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            g_nid.uCallbackMessage = WM_TRAYICON;
            g_nid.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
            wcscpy_s(g_nid.szTip, 128, L"Virtual Companion - Astra ❤️");
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

            if (g_pet && g_pet->GetConfig().transparentBackground && width > 0 && height > 0) {
                // Preserve PNG alpha all the way to the desktop compositor.
                BITMAPINFO info{};
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biWidth = width;
                info.bmiHeader.biHeight = -height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biCompression = BI_RGB;
                void* pixels = nullptr;
                HDC memory = CreateCompatibleDC(hdc);
                HBITMAP dib = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
                auto oldBmp = SelectObject(memory, dib);

                // Render into a cleared premultiplied-alpha surface. Drawing with
                // ordinary SourceOver onto a fresh DIB can retain undefined pixels
                // and make successive poses appear overlaid.
                if (!g_pet->GetAnimation().RenderAlpha(static_cast<BYTE*>(pixels), width, height)) {
                    std::memset(pixels, 0, static_cast<size_t>(width) * height * 4);
                    g_pet->Render(memory);
                }

                VirtualPet::Point petPos = g_pet->GetPosition();
                POINT dstPoint = { petPos.x, petPos.y };
                SIZE windowSize = { width, height };
                POINT srcPoint = { 0, 0 };
                BLENDFUNCTION blend{};
                blend.BlendOp = AC_SRC_OVER;
                blend.BlendFlags = 0;
                blend.SourceConstantAlpha = 255;
                blend.AlphaFormat = AC_SRC_ALPHA;

                // Submit 32-bit per-pixel alpha window update to Desktop Window Manager (DWM)
                UpdateLayeredWindow(hwnd, hdc, &dstPoint, &windowSize, memory, &srcPoint, 0, &blend, ULW_ALPHA);

                SelectObject(memory, oldBmp);
                DeleteObject(dib);
                DeleteDC(memory);
                ::EndPaint(hwnd, &ps);
                return 0;
            }

            // Fallback: Double-buffered color-key blitting
            HDC memDC = ::CreateCompatibleDC(hdc);
            HBITMAP memBmp = ::CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ oldBmp = ::SelectObject(memDC, memBmp);

            // Fill background with transparent color key
            HBRUSH bgBrush = ::CreateSolidBrush(TRANSPARENT_COLOR_KEY);
            RECT fillRect{0, 0, width, height};
            ::FillRect(memDC, &fillRect, bgBrush);
            ::DeleteObject(bgBrush);

            if (g_pet) {
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

        case WM_LBUTTONDBLCLK:
            ShowChatPanel(); return 0;

        case WM_MOUSEMOVE: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            if (g_pet) {
                g_pet->OnMouseMove(VirtualPet::Point(pt.x, pt.y));
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ClientToScreen(hwnd, &pt);
            if (g_pet) {
                g_pet->OnLButtonUp(VirtualPet::Point(pt.x, pt.y));
            }
            ::ReleaseCapture();
            return 0;
        }

        case WM_CANCELMODE:
            if (g_pet) g_pet->CancelInteraction();
            ::ReleaseCapture();
            return 0;

        case WM_CAPTURECHANGED:
            if (g_pet && g_pet->GetMouseSensor().IsLeftButtonDown()) g_pet->CancelInteraction();
            return 0;

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
                case ID_MENU_HEADPAT:
                    if (g_pet) g_pet->GiveHeadpat();
                    break;
                case ID_MENU_FEED:
                    if (g_pet) g_pet->SendCoffee();
                    break;
                case ID_MENU_DROP_TOY:
                    if (g_pet) g_pet->TossPlushieHeart();
                    break;
                case ID_MENU_STUDY:
                    if (g_pet) g_pet->StartStudyMode();
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

        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            if (g_pet) g_pet->RefreshDesktop();
            return 0;

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
    // SetProcessDpiAwarenessContext requires Windows 10 (1607) SDK headers.
    // MinGW may not expose it unless _WIN32_WINNT >= 0x0605.
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0605
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#else
    ::SetProcessDPIAware(); // Older SDK / MinGW fallback
#endif
    // 1. Initialize Companion system
    g_pet = std::make_unique<VirtualPet::Pet>();
    if (!g_pet->Init()) return 1;
    const auto& config = g_pet->GetConfig();

    // 2. Register Window Class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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
        (config.transparentBackground ? WS_EX_LAYERED : 0) | (config.alwaysOnTop ? WS_EX_TOPMOST : 0) | WS_EX_TOOLWINDOW,
        WINDOW_CLASS_NAME,
        Utf8ToWide(config.title).c_str(),
        WS_POPUP,
        initialPos.x, initialPos.y,
        width, height,
        nullptr, nullptr,
        hInstance, nullptr
    );

    if (!hwnd) {
        return 1;
    }

    WNDCLASSW toyClass{};
    toyClass.lpfnWndProc = ToyWndProc;
    toyClass.hInstance = hInstance;
    toyClass.lpszClassName = L"VirtualPetToy";
    RegisterClassW(&toyClass);
    g_toyWindow = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW |
        (config.alwaysOnTop ? WS_EX_TOPMOST : 0), L"VirtualPetToy", L"", WS_POPUP,
        0, 0, 28, 28, hwnd, nullptr, hInstance, nullptr);
    if (g_toyWindow) SetLayeredWindowAttributes(g_toyWindow, TRANSPARENT_COLOR_KEY, 0, LWA_COLORKEY);
    WNDCLASSW bubbleClass{};
    bubbleClass.lpfnWndProc = BubbleWndProc;
    bubbleClass.hInstance = hInstance;
    bubbleClass.lpszClassName = L"VirtualPetSpeechBubble";
    RegisterClassW(&bubbleClass);
    g_bubbleWindow = CreateWindowExW(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW |
        (config.alwaysOnTop ? WS_EX_TOPMOST : 0), L"VirtualPetSpeechBubble", L"", WS_POPUP,
        0, 0, 300, 92, hwnd, nullptr, hInstance, nullptr);
    if (g_bubbleWindow) SetLayeredWindowAttributes(g_bubbleWindow, TRANSPARENT_COLOR_KEY, 0, LWA_COLORKEY);
    WNDCLASSW chatClass{}; chatClass.lpfnWndProc=ChatWndProc; chatClass.hInstance=hInstance;
    chatClass.hCursor=LoadCursor(nullptr,IDC_ARROW); chatClass.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    chatClass.lpszClassName=L"VirtualPetChatPanel"; RegisterClassW(&chatClass);
    g_chatWindow=CreateWindowExW(WS_EX_TOOLWINDOW,L"VirtualPetChatPanel",L"Chat with Astra",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,100,100,500,445,nullptr,nullptr,hInstance,nullptr);
    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    // 4. Main simulation loop at target ~60 FPS
    using Clock = std::chrono::steady_clock;
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
            const auto& toy = g_pet->GetPhysics().GetToy();
            if (g_toyWindow) {
                if (toy.active && g_isPetVisible) {
                    SetWindowPos(g_toyWindow, nullptr, static_cast<int>(toy.x)-toy.radius, static_cast<int>(toy.y)-toy.radius,
                        toy.radius*2, toy.radius*2, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
                    InvalidateRect(g_toyWindow, nullptr, FALSE);
                } else ShowWindow(g_toyWindow, SW_HIDE);
            }
            if (g_bubbleWindow) {
                const std::wstring nextText = Utf8ToWide(g_pet->GetSpeechText());
                if (nextText != g_bubbleText) { g_bubbleText = nextText; InvalidateRect(g_bubbleWindow, nullptr, FALSE); }
                if (g_isPetVisible && !g_bubbleText.empty()) {
                    const auto bubblePos = g_pet->GetPosition();
                    const int bubbleX = bubblePos.x + width / 2 - 260;
                    const int bubbleY = (bubblePos.y >= 96) ? bubblePos.y - 88 : bubblePos.y + 8;
                    SetWindowPos(g_bubbleWindow, nullptr, bubbleX, bubbleY, 300, 92,
                                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
                } else ShowWindow(g_bubbleWindow, SW_HIDE);
            }

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
        const auto elapsed = Clock::now() - currentTime;
        int effectiveFps = config.targetFps;
        if (!g_isPetVisible) effectiveFps = (std::min)(effectiveFps, 10);
        if (g_pet && g_pet->GetSystemSensor().GetState().isBatterySaverOn)
            effectiveFps = (std::min)(effectiveFps, 20);
        const auto budget = std::chrono::duration<double>(1.0 / (std::max)(1, effectiveFps));
        if (elapsed < budget) {
            DWORD wait = static_cast<DWORD>(std::chrono::duration<double, std::milli>(budget - elapsed).count());
            ::Sleep((std::max)(DWORD(1), wait));
        }
    }

    g_pet.reset();
    return static_cast<int>(msg.wParam);
}

#endif

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
