#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include "../memory-external/classes/json.hpp"

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

        // Get a string value
        std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

        // Get an integer value
        int GetInt(const std::string& key, int defaultValue = 0) const;

        // Get a boolean value
        bool GetBool(const std::string& key, bool defaultValue = false) const;

        // Get a float value
        float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

        // Set a value (templated)
        template<typename T>
        void Set(const std::string& key, const T& value) {
            configData[key] = value;
        }

        // Check if key exists
        bool HasKey(const std::string& key) const;

        // Output functions (Turkish messages, but English code)
        static void LogInfo(const std::string& message);
        static void LogError(const std::string& message);
    };
}