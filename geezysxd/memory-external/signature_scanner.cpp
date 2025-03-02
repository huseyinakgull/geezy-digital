#include "signature_scanner.hpp"
#include <iostream>

namespace Scanner {
    SignatureScanner::SignatureScanner(HANDLE process, uintptr_t base, size_t size)
        : processHandle(process), moduleBase(base), moduleSize(size) {
        std::cout << "[BILGI] Imza tarayici baslatildi" << std::endl;
    }

    // Constructor implementation for use with MemoryManager
    SignatureScanner::SignatureScanner(const Memory::MemoryManager& memManager, uintptr_t base, size_t size)
        : processHandle(memManager.GetProcessHandle()), moduleBase(base), moduleSize(size) {
        std::cout << "[BILGI] Imza tarayici MemoryManager ile baslatildi" << std::endl;
    }

    std::vector<uint8_t> SignatureScanner::PatternToBytes(const std::string& pattern) {
        std::vector<uint8_t> bytes;
        auto start = pattern.c_str();
        auto end = pattern.c_str() + pattern.length();

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (current < end && *current == '?')
                    ++current;
                bytes.push_back(0);
            }
            else {
                bytes.push_back(static_cast<uint8_t>(strtoul(std::string(current, 2).c_str(), nullptr, 16)));
                ++current;
            }
        }

        return bytes;
    }

    uintptr_t SignatureScanner::FindSignature(const std::string& pattern, const std::string& mask) {
        std::vector<uint8_t> buffer(moduleSize);
        SIZE_T bytesRead;

        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(moduleBase), buffer.data(), moduleSize, &bytesRead)) {
            std::cout << "[HATA] Modul bellegi okunamadi" << std::endl;
            return 0;
        }

        std::vector<uint8_t> signatureBytes = PatternToBytes(pattern);

        for (size_t i = 0; i < bytesRead - mask.length(); ++i) {
            bool found = true;

            for (size_t j = 0; j < mask.length(); ++j) {
                if (mask[j] == 'x' && buffer[i + j] != signatureBytes[j]) {
                    found = false;
                    break;
                }
            }

            if (found) {
                return moduleBase + i;
            }
        }

        return 0;
    }

    uintptr_t SignatureScanner::FindSignatureByName(const std::string& name) {
        for (const auto& sig : customSignatures) {
            if (sig.name == name) {
                uintptr_t result = FindSignature(sig.pattern, sig.mask);
                if (result) {
                    std::cout << "[BASARI] " << name << " imzasi bulundu: 0x" << std::hex << result << std::dec << std::endl;
                    return result;
                }
                else {
                    std::cout << "[HATA] " << name << " imzasi bulunamadi" << std::endl;
                    return 0;
                }
            }
        }

        std::cout << "[HATA] Tanimlanmamis imza ismi: " << name << std::endl;
        return 0;
    }

    void SignatureScanner::AddCustomSignature(const std::string& name, const std::string& pattern, const std::string& mask) {
        customSignatures.push_back({ name, pattern, mask });
        std::cout << "[BILGI] Yeni imza eklendi: " << name << std::endl;
    }

    std::vector<std::pair<std::string, uintptr_t>> SignatureScanner::ScanForAllSignatures() {
        std::vector<std::pair<std::string, uintptr_t>> results;

        for (const auto& sig : customSignatures) {
            uintptr_t address = FindSignature(sig.pattern, sig.mask);
            results.push_back({ sig.name, address });

            if (address) {
                std::cout << "[BASARI] " << sig.name << " imzasi bulundu: 0x" << std::hex << address << std::dec << std::endl;
            }
            else {
                std::cout << "[HATA] " << sig.name << " imzasi bulunamadi" << std::endl;
            }
        }

        return results;
    }

    uintptr_t SignatureScanner::ResolveRelativeAddress(uintptr_t address, int offsetToRelative, int instructionSize) {
        int32_t relativeOffset = Read<int32_t>(address + offsetToRelative);
        return address + instructionSize + relativeOffset;
    }
}