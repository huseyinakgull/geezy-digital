// esp.hpp
#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include "game_interface.hpp"
#include "memory_manager.hpp"
#include "imgui/imgui.h"

namespace esp {

    struct PlayerBones {
        std::map<std::string, ImVec2> bonePositions;
    };

    struct PlayerInfo {
        int team;
        int health;
        int armor;
        std::string name;
        ImVec2 origin;
        ImVec2 head;
        PlayerBones bones;
        bool isDefusing;
        float flashAlpha;
        std::string weapon;
        int money;
    };

    struct C4Info {
        ImVec2 position;
    };

    class ESP {
    public:
        ESP();
        ~ESP();

        // ESP özelliklerini kontrol eden deðiþkenler
        bool showBoxESP = true;
        bool showSkeletonESP = false;
        bool showHeadTracker = false;
        bool showExtraFlags = true;
        bool teamESP = false; // Takým arkadaþlarýný gösterme
        int renderDistance = 5000;
        int flagRenderDistance = 200;

        // Renk ayarlarý
        ImVec4 boxColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        ImVec4 boxColorTeam = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        ImVec4 skeletonColorEnemy = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
        ImVec4 skeletonColorTeam = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
        ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 distanceColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

        // Kemik baðlantýlarý
        std::vector<std::pair<std::string, std::string>> boneConnections = {
            {"neck_0", "spine_1"},
            {"spine_1", "spine_2"},
            {"spine_2", "pelvis"},
            {"spine_1", "arm_upper_L"},
            {"arm_upper_L", "arm_lower_L"},
            {"arm_lower_L", "hand_L"},
            {"spine_1", "arm_upper_R"},
            {"arm_upper_R", "arm_lower_R"},
            {"arm_lower_R", "hand_R"},
            {"pelvis", "leg_upper_L"},
            {"leg_upper_L", "leg_lower_L"},
            {"leg_lower_L", "ankle_L"},
            {"pelvis", "leg_upper_R"},
            {"leg_upper_R", "leg_lower_R"},
            {"leg_lower_R", "ankle_R"}
        };

        // Kemik haritasý (hangi kemiðin hangi indekste olduðunu belirtir)
        std::map<std::string, int> boneMap = {
            {"head", 6},
            {"neck_0", 5},
            {"spine_1", 4},
            {"spine_2", 2},
            {"pelvis", 0},
            {"arm_upper_L", 8},
            {"arm_lower_L", 9},
            {"hand_L", 10},
            {"arm_upper_R", 13},
            {"arm_lower_R", 14},
            {"hand_R", 15},
            {"leg_upper_L", 22},
            {"leg_lower_L", 23},
            {"ankle_L", 24},
            {"leg_upper_R", 25},
            {"leg_lower_R", 26},
            {"ankle_R", 27}
        };

        // ESP render fonksiyonu
        void Render(game::GameInterface* gameInterface);

    private:
        // Kullanýlacak offsetler için veri yapýlarý
        struct Offsets {
            uintptr_t dwEntityList = 0x0;
            uintptr_t dwLocalPlayerController = 0x0;
            uintptr_t dwViewMatrix = 0x0;
            uintptr_t dwPlantedC4 = 0x0;
            uintptr_t m_hPlayerPawn = 0x0;
            uintptr_t m_iTeamNum = 0x0;
            uintptr_t m_vOldOrigin = 0x0;
            uintptr_t m_iHealth = 0x0;
            uintptr_t m_ArmorValue = 0x0;
            uintptr_t m_pGameSceneNode = 0x0;
            uintptr_t m_sSanitizedPlayerName = 0x0;
            uintptr_t m_bIsDefusing = 0x0;
            uintptr_t m_pInGameMoneyServices = 0x0;
            uintptr_t m_iAccount = 0x0;
            uintptr_t m_flFlashOverlayAlpha = 0x0;
            uintptr_t m_pClippingWeapon = 0x0;
            uintptr_t m_szName = 0x0;
            uintptr_t m_vecAbsOrigin = 0x0;
        };

        Offsets offsets;

        // ESP Verileri
        int localTeam = 0;
        ImVec2 localOrigin = ImVec2(0, 0);
        bool isC4Planted = false;
        C4Info c4Info;
        std::vector<PlayerInfo> players;

        // View matrix
        struct ViewMatrix {
            float matrix[16];
        };

        // Yardýmcý fonksiyonlar
        bool LoadOffsets();
        void UpdatePlayerData(game::GameInterface* gameInterface);
        ImVec2 WorldToScreen(const ViewMatrix& viewMatrix, const float* position, int screenWidth, int screenHeight);
        float CalculateDistance(const ImVec2& a, const ImVec2& b);
    };

} // namespace esp