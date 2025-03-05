#include "memory_manager.hpp"
#include <iomanip>
#include <sstream>
#include <regex>

namespace Memory {

    uintptr_t MemoryManager::ParseOffset(const json& offsetValue) {
        // Check if the offset is a string (potentially hex or decimal)
        if (offsetValue.is_string()) {
            std::string offsetStr = offsetValue.get<std::string>();

            // Check if it's a hexadecimal string
            if (offsetStr.substr(0, 2) == "0x") {
                return std::stoull(offsetStr, nullptr, 16);
            }

            // Assume it's a decimal string
            return std::stoull(offsetStr, nullptr, 10);
        }

        // If it's already a number, return it directly
        if (offsetValue.is_number_unsigned()) {
            return offsetValue.get<uintptr_t>();
        }

        // If a signed number, convert safely
        if (offsetValue.is_number_integer()) {
            return static_cast<uintptr_t>(offsetValue.get<intptr_t>());
        }

        LogError("Invalid offset type: cannot convert to uintptr_t");
        return 0;
    }

    MemoryManager::MemoryManager(const std::string& targetProcess) : processName(targetProcess) {
        LogInfo("Initializing memory manager...");
    }

    MemoryManager::~MemoryManager() {
        if (processHandle && processHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(processHandle);
            LogInfo("Memory manager shutting down, process handle closed.");
        }
    }

    bool MemoryManager::AttachToProcess() {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Process32First(snapshot, &entry)) {
            do {
                if (_stricmp(entry.szExeFile, processName.c_str()) == 0) {
                    processId = entry.th32ProcessID;
                    CloseHandle(snapshot);

                    // Get process handle with appropriate access rights for memory operations
                    processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, processId);

                    if (processHandle) {
                        LogSuccess("Process found and connected: " + processName + " (PID: " + std::to_string(processId) + ")");
                        return true;
                    }
                    else {
                        LogError("Process found but access denied. Try running as administrator.");
                        return false;
                    }
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        LogError("Process not found: " + processName);
        return false;
    }



    bool MemoryManager::LoadModule(const std::string& moduleName) {
        moduleBase = GetModuleBaseAddress(moduleName);
        if (moduleBase != 0) {
            LogSuccess("Module loaded: " + moduleName + " (Base: " + std::to_string(moduleBase) + ")");
            return true;
        }

        LogError("Failed to load module: " + moduleName);
        return false;
    }

    std::vector<uint8_t> MemoryManager::ReadBytes(uintptr_t address, size_t size) {
        std::vector<uint8_t> buffer(size);

        if (processHandle) {
            SIZE_T bytesRead;
            ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer.data(), size, &bytesRead);

            if (bytesRead != size) {
                buffer.resize(bytesRead);
            }
        }

        return buffer;
    }

    uintptr_t MemoryManager::FindSignature(const char* signature, const char* mask, uintptr_t start, size_t size) {
        std::vector<uint8_t> buffer = ReadBytes(start, size);

        if (buffer.empty()) {
            LogError("Failed to read memory region, cannot scan for signature.");
            return 0;
        }

        for (size_t i = 0; i < buffer.size() - strlen(mask) + 1; i++) {
            bool found = true;

            for (size_t j = 0; j < strlen(mask); j++) {
                if (mask[j] == 'x' && buffer[i + j] != static_cast<uint8_t>(signature[j])) {
                    found = false;
                    break;
                }
            }

            if (found) {
                LogSuccess("Signature found: " + std::to_string(start + i));
                return start + i;
            }
        }

        LogError("Signature not found.");
        return 0;
    }

    bool MemoryManager::LoadOffsets(const std::string& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                LogError("Failed to open offset file: " + filePath);
                return false;
            }

            file >> offsets;
            LogSuccess("Offsets loaded: " + std::to_string(offsets.size()) + " entries");
            return true;
        }
        catch (const std::exception& e) {
            LogError("Offset loading error: " + std::string(e.what()));
            return false;
        }
    }

    uintptr_t MemoryManager::GetOffset(const std::string& offsetName) {
        if (offsets.contains(offsetName)) {
            return ParseOffset(offsets[offsetName]);
        }

        LogError("Offset not found: " + offsetName);
        return 0;
    }

    uintptr_t MemoryManager::GetModuleBaseAddress(const std::string& moduleName) {
        MODULEENTRY32 entry;
        entry.dwSize = sizeof(MODULEENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

        if (Module32First(snapshot, &entry)) {
            do {
                if (_stricmp(entry.szModule, moduleName.c_str()) == 0) {
                    CloseHandle(snapshot);
                    return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                }
            } while (Module32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return 0;
    }

    uintptr_t MemoryManager::GetPointerAddress(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
        uintptr_t address = baseAddress;

        for (size_t i = 0; i < offsets.size(); i++) {
            address = Read<uintptr_t>(address);

            if (address == 0) {
                LogError("Pointer chain failed at step: " + std::to_string(i));
                return 0;
            }

            address += offsets[i];
        }

        return address;
    }

    void MemoryManager::LogInfo(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }

    void MemoryManager::LogError(const std::string& message) {
        std::cout << "[ERROR] " << message << std::endl;
    }

    void MemoryManager::LogSuccess(const std::string& message) {
        std::cout << "[SUCCESS] " << message << std::endl;
    }

} // namespace Memory