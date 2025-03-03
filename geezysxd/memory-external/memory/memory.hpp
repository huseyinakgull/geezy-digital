#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include "../classes/json.hpp"

#pragma comment(lib, "Psapi.lib")

using json = nlohmann::json;

namespace Memory {
    class MemoryManager {
    private:
        HANDLE processHandle = nullptr;
        DWORD processId = 0;
        uintptr_t moduleBase = 0;
        std::string processName;
        json offsets;

    public:
        MemoryManager(const std::string& targetProcess);
        ~MemoryManager();
        bool AttachToProcess();
        bool LoadModule(const std::string& moduleName);
        HANDLE GetProcessHandle() const { return processHandle; }
        template<typename T>
        T Read(uintptr_t address) {
            T value{};
            if (processHandle) {
                ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
            }
            return value;
        }
        template<typename T>
        bool Write(uintptr_t address, const T& value) {
            if (processHandle) {
                return WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
            }
            return false;
        }
        std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size);
        uintptr_t FindSignature(const char* signature, const char* mask, uintptr_t start, size_t size);
        bool LoadOffsets(const std::string& filePath);
        uintptr_t GetOffset(const std::string& offsetName);
        uintptr_t GetModuleBaseAddress(const std::string& moduleName);
        uintptr_t GetPointerAddress(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets);
        void LogInfo(const std::string& message);
        void LogError(const std::string& message);
        void LogSuccess(const std::string& message);
    };
}