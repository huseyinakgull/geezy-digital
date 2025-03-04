#include "game_interface.hpp"
#include <iostream>

namespace game {

    // Helper structure for EnumWindows callback
    struct FindWindowData {
        DWORD processId;
        HWND result;
    };

    // EnumWindows callback function
    BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
        FindWindowData* data = reinterpret_cast<FindWindowData*>(lParam);
        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId == data->processId) {
            char className[256] = { 0 };
            GetClassNameA(hwnd, className, sizeof(className));

            RECT rect;
            GetWindowRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Only accept windows larger than minimum size
            if (width > 200 && height > 200) {
                std::cout << "[FOUND] Game window (by Process ID): " << hwnd
                    << " Class: " << className
                    << " Size: " << width << "x" << height << std::endl;
                data->result = hwnd;
                return FALSE; // Stop enumeration
            }
        }
        return TRUE; // Continue enumeration
    }

    GameInterface::GameInterface()
        : m_memoryManager(nullptr)
        , m_isConnected(false)
        , m_clientModuleBase(0)
        , m_processName("cs2.exe")
        , m_clientModuleName("client.dll")
    {
        // Common window class names for CS2
        m_windowClassNames = {
            "CS2"
        };
    }

    GameInterface::~GameInterface() {
        if (m_memoryManager) {
            delete m_memoryManager;
            m_memoryManager = nullptr;
        }
    }

    bool GameInterface::Initialize(const std::string& processName) {
        m_processName = processName;

        // Create memory manager
        m_memoryManager = new Memory::MemoryManager(m_processName);

        // Attempt to connect to the game process
        if (m_memoryManager->AttachToProcess()) {
            m_clientModuleBase = m_memoryManager->GetModuleBaseAddress(m_clientModuleName.c_str());
            if (m_clientModuleBase != 0) {
                std::cout << "[SUCCESS] " << m_clientModuleName << " module found: 0x"
                    << std::hex << m_clientModuleBase << std::dec << std::endl;
                m_isConnected = true;
                return true;
            }
            else {
                std::cout << "[ERROR] " << m_clientModuleName << " module not found" << std::endl;
            }
        }
        else {
            std::cout << "[ERROR] Failed to attach to process: " << m_processName << std::endl;
        }

        return false;
    }

    HWND GameInterface::FindGameWindow() {
        HWND hwnd = NULL;

        // Try known class names first
        for (const auto& className : m_windowClassNames) {
            hwnd = FindWindowA(className.c_str(), NULL);
            if (hwnd) {
                std::cout << "[SUCCESS] Game window found with class name '" << className << "': " << hwnd << std::endl;
                return hwnd;
            }
        }

        // If not found by class name, try finding by process ID
        if (m_isConnected) {
            DWORD processId = GetProcessId();
            if (processId != 0) {
                hwnd = FindWindowByProcessId(processId);
                if (hwnd) {
                    return hwnd;
                }
            }
        }

        std::cout << "[ERROR] Game window not found" << std::endl;
        return NULL;
    }

    HWND GameInterface::FindWindowByProcessId(DWORD processId) {
        FindWindowData data = { processId, NULL };

        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

        return data.result;
    }

    DWORD GameInterface::GetProcessId() const {
        if (m_memoryManager && m_memoryManager->GetProcessHandle()) {
            return ::GetProcessId(m_memoryManager->GetProcessHandle());
        }
        return 0;
    }

    bool GameInterface::UpdateConnectionStatus() {
        if (!m_isConnected || !m_memoryManager) {
            return false;
        }

        // Check if process is still running
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(m_memoryManager->GetProcessHandle(), &exitCode) || exitCode != STILL_ACTIVE) {
            std::cout << "[INFO] Game closed, connection lost" << std::endl;
            m_isConnected = false;
            return false;
        }

        return true;
    }

} // namespace game