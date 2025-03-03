#include "overlay.hpp"
#include <dwmapi.h>
#include <iostream>

#pragma comment(lib, "dwmapi.lib")

namespace core {

    // Initialize static member
    WNDPROC Overlay::m_wndProc = nullptr;

    Overlay::Overlay()
        : m_overlayWindow(NULL)
        , m_windowClassName(L"GeezyDigitalOverlay")
        , m_windowTitle(L"GeezyDigital CS2 Menu")
    {
    }

    Overlay::~Overlay() {
        Shutdown();
    }

    ATOM Overlay::RegisterOverlayClass(HINSTANCE hInstance) {
        WNDCLASSEX wc = {
            sizeof(WNDCLASSEX),
            CS_CLASSDC,
            StaticWndProc,
            0L, 0L,
            hInstance,
            NULL, NULL, NULL, NULL,
            L"GeezyDigitalOverlay",
            NULL
        };

        return RegisterClassEx(&wc);
    }

    bool Overlay::Initialize(HWND gameWindow) {
        if (!gameWindow) {
            std::cout << "[ERROR] Cannot initialize overlay: No game window provided" << std::endl;
            return false;
        }

        // Register window class if not already registered
        RegisterOverlayClass(GetModuleHandle(NULL));

        // Create the overlay window
        return CreateOverlayWindow(gameWindow);
    }

    bool Overlay::CreateOverlayWindow(HWND gameWindow) {
        RECT gameRect;
        if (!GetWindowRect(gameWindow, &gameRect)) {
            std::cout << "[ERROR] Failed to get game window rectangle" << std::endl;
            return false;
        }

        // Create overlay window with appropriate styles for overlay functionality
        m_overlayWindow = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
            m_windowClassName.c_str(),
            m_windowTitle.c_str(),
            WS_POPUP,
            gameRect.left, gameRect.top,
            gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        if (!m_overlayWindow) {
            DWORD error = GetLastError();
            std::cout << "[ERROR] Failed to create overlay window. Error code: " << error << std::endl;
            return false;
        }

        // Make black background transparent (color keying)
        SetLayeredWindowAttributes(m_overlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);

        // Position overlay on top of game window
        SetWindowPos(m_overlayWindow, HWND_TOPMOST,
            gameRect.left, gameRect.top,
            gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
            SWP_SHOWWINDOW);

        // Optional: disable DWM blur effect
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = FALSE;
        DwmEnableBlurBehindWindow(m_overlayWindow, &bb);

        // Show window
        ShowWindow(m_overlayWindow, SW_SHOWDEFAULT);
        UpdateWindow(m_overlayWindow);

        return true;
    }

    void Overlay::Shutdown() {
        if (m_overlayWindow) {
            DestroyWindow(m_overlayWindow);
            m_overlayWindow = NULL;
        }
    }

    void Overlay::UpdatePosition(HWND gameWindow) {
        if (!IsWindow(gameWindow) || !IsWindow(m_overlayWindow))
            return;

        RECT gameRect;
        if (!GetWindowRect(gameWindow, &gameRect)) {
            // Window is no longer valid
            return;
        }

        // Match overlay size and position to game window
        SetWindowPos(
            m_overlayWindow, HWND_TOPMOST,
            gameRect.left, gameRect.top,
            gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
            SWP_NOACTIVATE
        );
    }

    void Overlay::SetVisible(bool visible) {
        // Toggle transparency based on visibility
        LONG ex_style = GetWindowLong(m_overlayWindow, GWL_EXSTYLE);
        if (visible) {
            // When menu is visible, allow mouse input
            ex_style &= ~WS_EX_TRANSPARENT;
        }
        else {
            // When menu is hidden, make window click-through
            ex_style |= WS_EX_TRANSPARENT;
        }
        SetWindowLong(m_overlayWindow, GWL_EXSTYLE, ex_style);
    }

    LRESULT CALLBACK Overlay::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // If custom window procedure is set, forward messages to it
        if (m_wndProc) {
            return m_wndProc(hWnd, msg, wParam, lParam);
        }

        // Default behavior
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace core