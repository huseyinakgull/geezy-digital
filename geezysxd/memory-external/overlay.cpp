#include "overlay.hpp"
#include <dwmapi.h>
#include <iostream>
#include "logger.hpp"

#pragma comment(lib, "dwmapi.lib")

namespace core {

    // Initialize static member
    WNDPROC Overlay::m_wndProc = nullptr;

    Overlay::Overlay()
        : m_overlayWindow(NULL)
        , m_windowClassName("GeezyDigitalOverlay")
        , m_windowTitle("GeezyDigital CS2 Menu")
    {
    }

    Overlay::~Overlay() {
        Shutdown();
    }

    ATOM Overlay::RegisterOverlayClass(HINSTANCE hInstance) {
        const char* className = "GeezyDigitalOverlay";  // Sabit string kullan
        WNDCLASSEX wc = {
            sizeof(WNDCLASSEX),
            CS_CLASSDC,
            StaticWndProc,
            0L, 0L,
            hInstance,
            NULL, NULL, NULL, NULL,
            className,
            NULL
        };

        return RegisterClassEx(&wc);
    }

    bool Overlay::Initialize(HWND gameWindow) {
        if (!gameWindow) {
            utils::LogError("Cannot initialize overlay: No game window provided");
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
            utils::LogError("Failed to get game window rectangle");
            return false;
        }

        int width = gameRect.right - gameRect.left;
        int height = gameRect.bottom - gameRect.top;

        utils::LogInfo("Game window size: " + std::to_string(width) + "x" + std::to_string(height));

        // Create overlay window with appropriate styles for overlay functionality
        m_overlayWindow = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_LAYERED, // Removed WS_EX_TRANSPARENT initially to allow input
            m_windowClassName.c_str(),
            m_windowTitle.c_str(),
            WS_POPUP,
            gameRect.left, gameRect.top,
            width, height,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        if (!m_overlayWindow) {
            DWORD error = GetLastError();
            utils::LogError("Failed to create overlay window. Error code: " + std::to_string(error));
            return false;
        }

        // Make fully transparent initially, will be updated based on menu visibility
        SetLayeredWindowAttributes(m_overlayWindow, RGB(0, 0, 0), 255, LWA_ALPHA);

        // Position overlay on top of game window
        SetWindowPos(m_overlayWindow, HWND_TOPMOST,
            gameRect.left, gameRect.top,
            width, height,
            SWP_SHOWWINDOW);

        // Disable DWM composition effects for better performance
        BOOL enable = FALSE;
        DwmSetWindowAttribute(m_overlayWindow, DWMWA_TRANSITIONS_FORCEDISABLED, &enable, sizeof(enable));

        // Optional: disable DWM blur effect
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = FALSE;
        DwmEnableBlurBehindWindow(m_overlayWindow, &bb);

        // Show window
        ShowWindow(m_overlayWindow, SW_SHOWDEFAULT);
        UpdateWindow(m_overlayWindow);

        utils::LogSuccess("Overlay window created successfully");
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

        int width = gameRect.right - gameRect.left;
        int height = gameRect.bottom - gameRect.top;

        // Match overlay size and position to game window
        SetWindowPos(
            m_overlayWindow, HWND_TOPMOST,
            gameRect.left, gameRect.top,
            width, height,
            SWP_NOACTIVATE
        );
    }

    void Overlay::SetVisible(bool visible) {
        if (!IsWindow(m_overlayWindow))
            return;

        // Toggle transparency based on visibility
        LONG ex_style = GetWindowLong(m_overlayWindow, GWL_EXSTYLE);
        if (visible) {
            // When menu is visible, allow mouse input and make window visible
            ex_style &= ~WS_EX_TRANSPARENT;
            SetWindowLong(m_overlayWindow, GWL_EXSTYLE, ex_style);
            // Make window more visible when menu is showing
            SetLayeredWindowAttributes(m_overlayWindow, RGB(0, 0, 0), 230, LWA_ALPHA);
        }
        else {
            // When menu is hidden, make window click-through and mostly transparent
            ex_style |= WS_EX_TRANSPARENT;
            SetWindowLong(m_overlayWindow, GWL_EXSTYLE, ex_style);
            // Make window nearly invisible when menu is hidden
            SetLayeredWindowAttributes(m_overlayWindow, RGB(0, 0, 0), 1, LWA_ALPHA);
        }
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