#include "memory.hpp"
#include "handle_hijack.hpp"
#include <tlhelp32.h>

namespace geezy_digital {

    // ��lem ba�lant�s� i�in ortak ad�mlar� ger�ekle�tirir
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

    // ProcessManager s�n�f�n�n implementasyonu
    bool ProcessManager::GD_AttachToProcessWithHijacking(const char* processName) {
        m_processId = GD_FindProcessIdByName(processName);
        if (!m_processId) return false;

        // Handle Hijacking kullanarak i�leme ba�lan
        m_processHandle = handle_hijack::GD_HijackHandle(m_processId);

        if (!m_processHandle || m_processHandle == INVALID_HANDLE_VALUE) {
            std::cout << "[geezy_digital] Handle Hijacking ba�ar�s�z oldu, OpenProcess y�ntemine ge�iliyor." << std::endl;
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

    // ��lem ad�na g�re i�leme ba�lan�r
    bool ProcessManager::GD_AttachToProcess(const char* processName) {
        m_processId = GD_FindProcessIdByName(processName);
        if (!m_processId) return false;

        return GD_InitializeProcess();
    }

    // Pencere ad�na g�re i�leme ba�lan�r
    bool ProcessManager::GD_AttachToWindow(const char* windowName) {
        m_processId = GD_FindProcessIdByWindowName(windowName);
        if (!m_processId) return false;

        return GD_InitializeProcess();
    }

    // Pencere handle'�n� g�nceller
    bool ProcessManager::GD_UpdateWindowHandle() {
        m_windowHandle = GD_FindWindowHandleByProcessId(m_processId);
        return m_windowHandle != NULL;
    }

    // ��lem handle'�n� kapat�r
    void ProcessManager::GD_CloseProcess() {
        if (m_processHandle && m_processHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_processHandle);
            m_processHandle = NULL;
        }
    }

    // ��lem ID'si ile bir pencerenin handle'�n� bulur
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

    // ��lem ad�yla i�lem ID'sini bulur
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

    // Pencere ad�yla i�lem ID'sini bulur
    DWORD ProcessManager::GD_FindProcessIdByWindowName(const char* windowName) {
        DWORD processId = 0;
        HWND windowHandle = FindWindowA(nullptr, windowName);
        if (windowHandle) {
            GetWindowThreadProcessId(windowHandle, &processId);
        }
        return processId;
    }

    // �mza ile bellek i�inde arama yapar
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

    // ��lem mod�l�n� isimle al�r
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