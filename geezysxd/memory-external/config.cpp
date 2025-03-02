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

    bool ConfigManager::HasKey(const std::string& key) const {
        return isLoaded && configData.contains(key);
    }

    void ConfigManager::LogInfo(const std::string& message) {
        std::cout << "[BILGI] " << message << std::endl;
    }

    void ConfigManager::LogError(const std::string& message) {
        std::cout << "[HATA] " << message << std::endl;
    }
}