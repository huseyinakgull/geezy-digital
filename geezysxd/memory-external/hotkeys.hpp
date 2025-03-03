#pragma once

#include <Windows.h>
#include <functional>
#include <map>

namespace core {

    // Hotkey IDs for the application
    enum class HotkeyId {
        MENU_TOGGLE = 1,
        EXIT = 2,
        // Add more hotkeys as needed
    };

    class HotkeyManager {
    public:
        HotkeyManager();
        ~HotkeyManager();

        // Register a global hotkey
        bool RegisterHotkey(HWND hWnd, HotkeyId id, UINT modifiers, UINT vk);

        // Unregister a global hotkey
        bool UnregisterHotkey(HWND hWnd, HotkeyId id);

        // Unregister all global hotkeys
        void UnregisterAllHotkeys(HWND hWnd);

        // Handle WM_HOTKEY message, return true if handled
        bool HandleHotkeyMessage(WPARAM wParam, LPARAM lParam);

        // Set callback for a specific hotkey
        void SetHotkeyCallback(HotkeyId id, std::function<void()> callback);

    private:
        // Map of hotkey IDs to callback functions
        std::map<HotkeyId, std::function<void()>> m_hotkeyCallbacks;
    };

} // namespace core