#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "../memory-external/classes/json.hpp"

using json = nlohmann::json;

namespace Offsets {
    class OffsetManager {
    private:
        json offsetData;
        std::string offsetFilePath;
        bool isLoaded = false;

        // Check if a network connection is available
        bool IsNetworkAvailable();

    public:
        OffsetManager(const std::string& filePath = "offsets.json");
        ~OffsetManager() = default;

        // Load offsets from file
        bool LoadOffsets();

        // Save offsets to file
        bool SaveOffsets();

        // Update offsets from an online source
        bool UpdateOffsetsFromRepository(const std::string& repositoryUrl);

        // Get an offset by name
        uintptr_t GetOffset(const std::string& name);

        // Set an offset value
        void SetOffset(const std::string& name, uintptr_t value);

        // Check if build number is current
        bool IsBuildCurrent(int currentBuild);

        // Get build number
        int GetBuildNumber();

        // Output functions (Turkish messages, but English code)
        static void LogInfo(const std::string& message);
        static void LogError(const std::string& message);
        static void LogSuccess(const std::string& message);
    };
}