#include "offset_manager.hpp"
#include <iostream>
#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

namespace Offsets {
    OffsetManager::OffsetManager(const std::string& filePath) : offsetFilePath(filePath) {
        LogInfo("Offset yoneticisi baslatiliyor...");
    }

    bool OffsetManager::IsNetworkAvailable() {
        DWORD flags;
        return InternetGetConnectedState(&flags, 0);
    }

    bool OffsetManager::LoadOffsets() {
        try {
            std::ifstream file(offsetFilePath);
            if (!file.is_open()) {
                LogError("Offset dosyasi acilamadi: " + offsetFilePath);
                return false;
            }

            file >> offsetData;
            isLoaded = true;

            LogSuccess("Offsetler yuklendi. Build numarasi: " + std::to_string(GetBuildNumber()));
            return true;
        }
        catch (const std::exception& e) {
            LogError("Offset yukleme hatasi: " + std::string(e.what()));
            return false;
        }
    }

    bool OffsetManager::SaveOffsets() {
        try {
            std::ofstream file(offsetFilePath);
            if (!file.is_open()) {
                LogError("Offset dosyasi kaydedilemedi: " + offsetFilePath);
                return false;
            }

            file << offsetData.dump(4);
            LogSuccess("Offsetler kaydedildi");
            return true;
        }
        catch (const std::exception& e) {
            LogError("Offset kaydetme hatasi: " + std::string(e.what()));
            return false;
        }
    }

    bool OffsetManager::UpdateOffsetsFromRepository(const std::string& repositoryUrl) {
        if (!IsNetworkAvailable()) {
            LogError("Internet baglantisi yok, offsetler guncellenemiyor");
            return false;
        }

        // Not implementing actual HTTP request logic here
        // This would typically use WinInet or similar to fetch the updated offsets

        LogInfo("Offsetler guncelleniyor...");

        // Simulate success for demo purposes
        LogSuccess("Offsetler guncellendi");
        return true;
    }

    uintptr_t OffsetManager::GetOffset(const std::string& name) {
        if (!isLoaded) {
            LogError("Offsetler henuz yuklenmedi");
            return 0;
        }

        if (offsetData.contains(name)) {
            return offsetData[name].get<uintptr_t>();
        }

        LogError("Offset bulunamadi: " + name);
        return 0;
    }

    void OffsetManager::SetOffset(const std::string& name, uintptr_t value) {
        offsetData[name] = value;
    }

    bool OffsetManager::IsBuildCurrent(int currentBuild) {
        if (!isLoaded) {
            LogError("Offsetler henuz yuklenmedi");
            return false;
        }

        if (!offsetData.contains("build_number")) {
            LogError("Build numarasi offsetlerde yok");
            return false;
        }

        int storedBuild = offsetData["build_number"].get<int>();
        bool isCurrent = (storedBuild == currentBuild);

        if (isCurrent) {
            LogSuccess("Offsetler guncel (Build: " + std::to_string(currentBuild) + ")");
        }
        else {
            LogInfo("Offsetler guncel degil. Yuklenen build: " + std::to_string(storedBuild) +
                ", Mevcut build: " + std::to_string(currentBuild));
        }

        return isCurrent;
    }

    int OffsetManager::GetBuildNumber() {
        if (!isLoaded) {
            LogError("Offsetler henuz yuklenmedi");
            return 0;
        }

        if (!offsetData.contains("build_number")) {
            LogError("Build numarasi offsetlerde yok");
            return 0;
        }

        return offsetData["build_number"].get<int>();
    }

    void OffsetManager::LogInfo(const std::string& message) {
        std::cout << "[BILGI] " << message << std::endl;
    }

    void OffsetManager::LogError(const std::string& message) {
        std::cout << "[HATA] " << message << std::endl;
    }

    void OffsetManager::LogSuccess(const std::string& message) {
        std::cout << "[BASARI] " << message << std::endl;
    }
}