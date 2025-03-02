#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <memory>
#include "../memory-external/memory/memory.hpp"

namespace Scanner {
    class SignatureScanner {
    private:
        HANDLE processHandle;
        uintptr_t moduleBase;
        size_t moduleSize;

        // Custom signature patterns for CS2
        struct SignatureDefinition {
            std::string name;
            std::string pattern;
            std::string mask;
        };

        // Custom signature collection
        std::vector<SignatureDefinition> customSignatures = {
            {"EntityList", "\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x7C\x24\x30\x41\xBB", "xxx????xxxxxxx"},
            {"LocalPlayer", "\x48\x8D\x05\x00\x00\x00\x00\x48\x8B\x18\x48\x85\xDB\x74\x2A", "xxx????xxxxxxxx"},
            {"ViewMatrix", "\x48\x8D\x0D\x00\x00\x00\x00\x48\xC1\xE0\x06\x48", "xxx????xxxxx"},
            {"PlantedC4", "\x48\x8B\x15\x00\x00\x00\x00\x48\x85\xD2\x74\x38", "xxx????xxxxx"}
        };

        // Convert pattern string to bytes
        static std::vector<uint8_t> PatternToBytes(const std::string& pattern);

    public:
        SignatureScanner(HANDLE process, uintptr_t base, size_t size);
        // Constructor overload for using with MemoryManager
        SignatureScanner(const Memory::MemoryManager& memManager, uintptr_t base, size_t size);
        ~SignatureScanner() = default;

        // Find a signature in memory
        uintptr_t FindSignature(const std::string& pattern, const std::string& mask);

        // Find a signature by predefined name
        uintptr_t FindSignatureByName(const std::string& name);

        // Add a custom signature
        void AddCustomSignature(const std::string& name, const std::string& pattern, const std::string& mask);

        // Scan for all predefined signatures
        std::vector<std::pair<std::string, uintptr_t>> ScanForAllSignatures();

        // Read memory at specified address
        template<typename T>
        T Read(uintptr_t address) {
            T value{};
            ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
            return value;
        }

        // Resolve relative address (used for following relative jumps in x64)
        uintptr_t ResolveRelativeAddress(uintptr_t address, int offsetToRelative, int instructionSize);
    };
}