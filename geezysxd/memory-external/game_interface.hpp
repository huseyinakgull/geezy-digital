#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "memory_manager.hpp"

namespace game {

    class GameInterface {
    public:
        GameInterface();
        ~GameInterface();

        // Initialize and connect to the game
        bool Initialize(const std::string& processName);

        // Find the game window
        HWND FindGameWindow();

        // Get game memory manager
        Memory::MemoryManager* GetMemoryManager() const { return m_memoryManager; }

        // Check if connected to game
        bool IsConnected() const { return m_isConnected; }

        // Get client module base address
        uintptr_t GetClientModuleBase() const { return m_clientModuleBase; }

        // Get game process ID
        DWORD GetProcessId() const;

        // Update connection status (check if game is still running)
        bool UpdateConnectionStatus();

    private:
        Memory::MemoryManager* m_memoryManager;
        bool m_isConnected;
        uintptr_t m_clientModuleBase;
        std::string m_processName;
        std::string m_clientModuleName;
        std::vector<std::string> m_windowClassNames;

        // Find window by process ID
        HWND FindWindowByProcessId(DWORD processId);
    };

} // namespace game