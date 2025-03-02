#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "classes/json.hpp"

using json = nlohmann::json;

namespace Config {
    class ConfigManager {
    private:
        json configData;
        std::string configFilePath;
        bool isLoaded = false;

    public:
        ConfigManager(const std::string& filePath = "config.json");
        ~ConfigManager();

        // Load config from file
        bool LoadConfig();

        // Save config to file
        bool SaveConfig();

        // Get values from config with different data types
        std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
        int GetInt(const std::string& key, int defaultValue = 0) const;
        bool GetBool(const std::string& key, bool defaultValue = false) const;
        float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

        // Get offset values
        uintptr_t GetOffset(const std::string& name, uintptr_t defaultValue = 0) const;
        uintptr_t GetWeaponOffset(const std::string& name, uintptr_t defaultValue = 0) const;

        // Get json subsection
        json GetSection(const std::string& sectionName) const;

        // Set a value (templated)
        template<typename T>
        void Set(const std::string& key, const T& value) {
            configData[key] = value;
        }

        // Set a nested value
        template<typename T>
        void SetNested(const std::string& section, const std::string& key, const T& value) {
            configData[section][key] = value;
        }

        // Check if key exists
        bool HasKey(const std::string& key) const;

        // Check if section exists
        bool HasSection(const std::string& section) const;

        // Output functions
        static void LogInfo(const std::string& message);
        static void LogError(const std::string& message);
        static void LogSuccess(const std::string& message);
    };
}