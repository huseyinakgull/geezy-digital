#include "memory.hpp"
#include "handle_hijack.hpp"
#include <tlhelp32.h>

namespace geezy_digital {

    // Ýþlem baðlantýsý için ortak adýmlarý gerçekleþtirir
    bool ProcessManager::GD_InitializeProcess() {
        if (!m_processId) return false;

        m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
        if (!m_processHandle || m_processHandle == INVALID_HANDLE_VALUE) return false;

        HMODULE modules[0xFF];
        DWORD cbNeeded;
        if (EnumProcessModulesEx(m_processHandle, modules, sizeof(modules), &cbNeeded, LIST_MODULES_64BIT)) {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(m_processHandle, modules[0], &moduleInfo, sizeof(moduleInfo))) {
                m_baseModule.base = (uintptr_t)modules[0];
                m_baseModule.size = moduleInfo.SizeOfImage;
            }
        }

        m_windowHandle = GD_FindWindowHandleByProcessId(m_processId);
        return m_baseModule.IsValid();
    }

    // ProcessManager sýnýfýnýn implementasyonu
    bool ProcessManager::GD_AttachToProcessWithHijacking(const char* processName) {
        m_processId = GD_FindProcessIdByName(processName);
        if (!m_processId) return false;

        // Handle Hijacking kullanarak iþleme baðlan
        m_processHandle = handle_hijack::GD_HijackHandle(m_processId);

        if (!m_processHandle || m_processHandle == INVALID_HANDLE_VALUE) {
            std::cout << "[geezy_digital] Handle Hijacking baþarýsýz oldu, OpenProcess yöntemine geçiliyor." << std::endl;
            return GD_AttachToProcess(processName);
        }

        HMODULE modules[0xFF];
        DWORD cbNeeded;
        if (EnumProcessModulesEx(m_processHandle, modules, sizeof(modules), &cbNeeded, LIST_MODULES_64BIT)) {
            MODULEINFO moduleInfo;
            if (GetModuleInformation(m_processHandle, modules[0], &moduleInfo, sizeof(moduleInfo))) {
                m_baseModule.base = (uintptr_t)modules[0];
                m_baseModule.size = moduleInfo.SizeOfImage;
            }
        }

        m_windowHandle = GD_FindWindowHandleByProcessId(m_processId);
        return m_baseModule.IsValid();
    }

    // Ýþlem adýna göre iþleme baðlanýr
    bool ProcessManager::GD_AttachToProcess(const char* processName) {
        m_processId = GD_FindProcessIdByName(processName);
        if (!m_processId) return false;

        return GD_InitializeProcess();
    }

    // Pencere adýna göre iþleme baðlanýr
    bool ProcessManager::GD_AttachToWindow(const char* windowName) {
        m_processId = GD_FindProcessIdByWindowName(windowName);
        if (!m_processId) return false;

        return GD_InitializeProcess();
    }

    // Pencere handle'ýný günceller
    bool ProcessManager::GD_UpdateWindowHandle() {
        m_windowHandle = GD_FindWindowHandleByProcessId(m_processId);
        return m_windowHandle != NULL;
    }

    // Ýþlem handle'ýný kapatýr
    void ProcessManager::GD_CloseProcess() {
        if (m_processHandle && m_processHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_processHandle);
            m_processHandle = NULL;
        }
    }

    // Ýþlem ID'si ile bir pencerenin handle'ýný bulur
    HWND ProcessManager::GD_FindWindowHandleByProcessId(DWORD processId) {
        HWND hwnd = NULL;
        do {
            hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == processId) {
                TCHAR windowTitle[MAX_PATH];
                GetWindowText(hwnd, windowTitle, MAX_PATH);
                if (IsWindowVisible(hwnd) && windowTitle[0] != '\0') {
                    return hwnd;
                }
            }
        } while (hwnd != NULL);
        return NULL;
    }

    // Ýþlem adýyla iþlem ID'sini bulur
    DWORD ProcessManager::GD_FindProcessIdByName(const char* processName) {
        std::wstring wideProcessName;
        int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, processName, -1, nullptr, 0);
        if (wideCharLength > 0) {
            wideProcessName.resize(wideCharLength);
            MultiByteToWideChar(CP_UTF8, 0, processName, -1, &wideProcessName[0], wideCharLength);
        }

        HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        PROCESSENTRY32W processEntry{};
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        DWORD pid = 0;
        if (Process32FirstW(hPID, &processEntry)) {
            do {
                if (!wcscmp(processEntry.szExeFile, wideProcessName.c_str())) {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hPID, &processEntry));
        }
        CloseHandle(hPID);
        return pid;
    }

    // Pencere adýyla iþlem ID'sini bulur
    DWORD ProcessManager::GD_FindProcessIdByWindowName(const char* windowName) {
        DWORD processId = 0;
        HWND windowHandle = FindWindowA(nullptr, windowName);
        if (windowHandle) {
            GetWindowThreadProcessId(windowHandle, &processId);
        }
        return processId;
    }

    // Ýmza ile bellek içinde arama yapar
    uintptr_t ProcessManager::GD_FindPatternInMemory(const std::vector<uint8_t>& signature, const ModuleInfo& moduleInfo) {
        const ModuleInfo& searchModule = moduleInfo.IsValid() ? moduleInfo : m_baseModule;
        if (!searchModule.IsValid()) return 0;

        std::unique_ptr<uint8_t[]> moduleData = std::make_unique<uint8_t[]>(searchModule.size);
        if (!m_memoryAccess.GD_ReadRaw(m_processHandle, searchModule.base, moduleData.get(), searchModule.size)) {
            return 0;
        }

        for (uintptr_t i = 0; i < searchModule.size; i++) {
            bool found = true;
            for (size_t j = 0; j < signature.size(); j++) {
                if (signature[j] != 0x00 && moduleData[i + j] != signature[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return searchModule.base + i;
            }
        }
        return 0;
    }

    // Ýþlem modülünü isimle alýr
    ModuleInfo ProcessManager::GD_GetModuleByName(const char* moduleName) {
        std::wstring wideModule;
        int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, moduleName, -1, nullptr, 0);
        if (wideCharLength > 0) {
            wideModule.resize(wideCharLength);
            MultiByteToWideChar(CP_UTF8, 0, moduleName, -1, &wideModule[0], wideCharLength);
        }

        HANDLE handleModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_processId);
        MODULEENTRY32W moduleEntry{};
        moduleEntry.dwSize = sizeof(MODULEENTRY32W);

        ModuleInfo result;
        if (Module32FirstW(handleModule, &moduleEntry)) {
            do {
                if (!wcscmp(moduleEntry.szModule, wideModule.c_str())) {
                    result.base = (uintptr_t)moduleEntry.modBaseAddr;
                    result.size = moduleEntry.dwSize;
                    break;
                }
            } while (Module32NextW(handleModule, &moduleEntry));
        }

        CloseHandle(handleModule);
        return result;
    }

} // namespace geezy_digital