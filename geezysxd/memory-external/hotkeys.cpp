#include "hotkeys.hpp"
#include <iostream>

namespace core {

    HotkeyManager::HotkeyManager() {
    }

    HotkeyManager::~HotkeyManager() {
    }

    bool HotkeyManager::RegisterHotkey(HWND hWnd, HotkeyId id, UINT modifiers, UINT vk) {
        if (!RegisterHotKey(hWnd, static_cast<int>(id), modifiers, vk)) {
            DWORD error = GetLastError();
            std::cout << "[ERROR] Failed to register hotkey (ID: " << static_cast<int>(id)
                << ", VK: " << vk << "). Error: " << error << std::endl;
            return false;
        }

        return true;
    }

    bool HotkeyManager::UnregisterHotkey(HWND hWnd, HotkeyId id) {
        return UnregisterHotKey(hWnd, static_cast<int>(id));
    }

    void HotkeyManager::UnregisterAllHotkeys(HWND hWnd) {
        // Unregister all known hotkeys
        UnregisterHotkey(hWnd, HotkeyId::MENU_TOGGLE);
        UnregisterHotkey(hWnd, HotkeyId::EXIT);
        // Add more as needed
    }

    bool HotkeyManager::HandleHotkeyMessage(WPARAM wParam, LPARAM lParam) {
        HotkeyId id = static_cast<HotkeyId>(wParam);

        // Check if we have a callback for this hotkey
        auto it = m_hotkeyCallbacks.find(id);
        if (it != m_hotkeyCallbacks.end() && it->second) {
            // Execute the callback
            it->second();
            return true;
        }

        return false;
    }

    void HotkeyManager::SetHotkeyCallback(HotkeyId id, std::function<void()> callback) {
        m_hotkeyCallbacks[id] = callback;
    }

} // namespace core