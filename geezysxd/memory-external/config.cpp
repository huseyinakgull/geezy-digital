#include "config.hpp"
#include <iostream>

namespace Config {
    ConfigManager::ConfigManager(const std::string& filePath) : configFilePath(filePath) {
        LogInfo("Konfigurasyon yoneticisi baslatiliyor...");
        LoadConfig();
    }

    ConfigManager::~ConfigManager() {
        if (isLoaded) {
            SaveConfig();
        }
    }

    bool ConfigManager::LoadConfig() {
        try {
            std::ifstream file(configFilePath);
            if (!file.is_open()) {
                LogInfo("Konfigurasyon dosyasi bulunamadi, yeni bir tane olusturuluyor: " + configFilePath);
                isLoaded = true;
                return true; // Not an error, just create a new config
            }

            file >> configData;
            isLoaded = true;

            LogInfo("Konfigurasyon yuklendi");
            return true;
        }
        catch (const std::exception& e) {
            LogError("Konfigurasyon yukleme hatasi: " + std::string(e.what()));
            return false;
        }
    }

    bool ConfigManager::SaveConfig() {
        try {
            std::ofstream file(configFilePath);
            if (!file.is_open()) {
                LogError("Konfigurasyon dosyasi kaydedilemedi: " + configFilePath);
                return false;
            }

            file << configData.dump(4);
            LogInfo("Konfigurasyon kaydedildi");
            return true;
        }
        catch (const std::exception& e) {
            LogError("Konfigurasyon kaydetme hatasi: " + std::string(e.what()));
            return false;
        }
    }

    std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
        if (!isLoaded || !configData.contains(key)) {
            return defaultValue;
        }

        try {
            return configData.at(key).get<std::string>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    int ConfigManager::GetInt(const std::string& key, int defaultValue) const {
        if (!isLoaded || !configData.contains(key)) {
            return defaultValue;
        }

        try {
            return configData.at(key).get<int>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    bool ConfigManager::GetBool(const std::string& key, bool defaultValue) const {
        if (!isLoaded || !configData.contains(key)) {
            return defaultValue;
        }

        try {
            return configData.at(key).get<bool>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    float ConfigManager::GetFloat(const std::string& key, float defaultValue) const {
        if (!isLoaded || !configData.contains(key)) {
            return defaultValue;
        }

        try {
            return configData.at(key).get<float>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    uintptr_t ConfigManager::GetOffset(const std::string& name, uintptr_t defaultValue) const {
        if (!isLoaded || !configData.contains("Offsets") || !configData["Offsets"].contains(name)) {
            return defaultValue;
        }

        try {
            return configData["Offsets"][name].get<uintptr_t>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    uintptr_t ConfigManager::GetWeaponOffset(const std::string& name, uintptr_t defaultValue) const {
        if (!isLoaded || !configData.contains("WeaponOffsets") || !configData["WeaponOffsets"].contains(name)) {
            return defaultValue;
        }

        try {
            return configData["WeaponOffsets"][name].get<uintptr_t>();
        }
        catch (...) {
            return defaultValue;
        }
    }

    json ConfigManager::GetSection(const std::string& sectionName) const {
        if (!isLoaded || !configData.contains(sectionName)) {
            return json::object();
        }

        return configData.at(sectionName);
    }

    bool ConfigManager::HasKey(const std::string& key) const {
        return isLoaded && configData.contains(key);
    }

    bool ConfigManager::HasSection(const std::string& section) const {
        return isLoaded && configData.contains(section);
    }

    void ConfigManager::LogInfo(const std::string& message) {
        std::cout << "[BILGI] " << message << std::endl;
    }

    void ConfigManager::LogError(const std::string& message) {
        std::cout << "[HATA] " << message << std::endl;
    }

    void ConfigManager::LogSuccess(const std::string& message) {
        std::cout << "[BASARI] " << message << std::endl;
    }
}