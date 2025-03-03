#pragma once

#include <Windows.h>
#include <string>
#include <functional>

namespace core {

    class Overlay {
    public:
        Overlay();
        ~Overlay();

        // Initialize overlay window
        bool Initialize(HWND gameWindow);

        // Shutdown and cleanup
        void Shutdown();

        // Update overlay position to match game window
        void UpdatePosition(HWND gameWindow);

        // Set visibility of overlay (transparent for input or not)
        void SetVisible(bool visible);

        // Get overlay window handle
        HWND GetWindowHandle() const { return m_overlayWindow; }

        // Register window class for overlay
        static ATOM RegisterOverlayClass(HINSTANCE hInstance);

        // Set custom window procedure
        void SetWindowProcedure(WNDPROC wndProc) { m_wndProc = wndProc; }

        // Static window procedure (forwards to custom procedure if set)
        static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        HWND m_overlayWindow;          // Handle to overlay window
        static WNDPROC m_wndProc;      // Custom window procedure
        std::wstring m_windowClassName; // Window class name
        std::wstring m_windowTitle;     // Window title

        // Create the actual window
        bool CreateOverlayWindow(HWND gameWindow);
    };

} // namespace core