#include "../memory/memory.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>

namespace Memory {
    MemoryManager::MemoryManager(const std::string& targetProcess) : processName(targetProcess) {
        LogInfo("Bellek yoneticisi baslatiliyor...");
    }

    MemoryManager::~MemoryManager() {
        if (processHandle && processHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(processHandle);
            LogInfo("Bellek yoneticisi kapaniyor, islem handle kapatildi.");
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
                        LogSuccess("Islem bulundu ve baglanti kuruldu: " + processName + " (PID: " + std::to_string(processId) + ")");
                        return true;
                    }
                    else {
                        LogError("Islem bulunamadi veya erisim reddedildi. Lutfen yonetici olarak calistirin.");
                        return false;
                    }
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        LogError("Islem bulunamadi: " + processName);
        return false;
    }

    bool MemoryManager::LoadModule(const std::string& moduleName) {
        moduleBase = GetModuleBaseAddress(moduleName);
        if (moduleBase != 0) {
            LogSuccess("Modul yuklendi: " + moduleName + " (Base: 0x" + std::to_string(moduleBase) + ")");
            return true;
        }

        LogError("Modul yuklenemedi: " + moduleName);
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
            LogError("Bellek bolumu okunamadi, imza taramasi yapilamiyor.");
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
                LogSuccess("Imza bulundu: 0x" + std::to_string(start + i));
                return start + i;
            }
        }

        LogError("Imza bulunamadi.");
        return 0;
    }

    bool MemoryManager::LoadOffsets(const std::string& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                LogError("Offset dosyasi acilamadi: " + filePath);
                return false;
            }

            file >> offsets;
            LogSuccess("Offsetler yuklendi: " + std::to_string(offsets.size()) + " adet");
            return true;
        }
        catch (const std::exception& e) {
            LogError("Offset yukleme hatasi: " + std::string(e.what()));
            return false;
        }
    }

    uintptr_t MemoryManager::GetOffset(const std::string& offsetName) {
        if (offsets.contains(offsetName)) {
            return offsets[offsetName].get<uintptr_t>();
        }

        LogError("Offset bulunamadi: " + offsetName);
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
                LogError("Pointer zinciri basarisiz oldu adimda: " + std::to_string(i));
                return 0;
            }

            address += offsets[i];
        }

        return address;
    }

    void MemoryManager::LogInfo(const std::string& message) {
        std::cout << "[BILGI] " << message << std::endl;
    }

    void MemoryManager::LogError(const std::string& message) {
        std::cout << "[HATA] " << message << std::endl;
    }

    void MemoryManager::LogSuccess(const std::string& message) {
        std::cout << "[BASARI] " << message << std::endl;
    }
}