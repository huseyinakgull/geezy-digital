#ifndef _GEEZY_DIGITAL_CONFIG_HPP_
#define _GEEZY_DIGITAL_CONFIG_HPP_

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include "../memory-external/classes/json.hpp"

namespace geezy_digital {

    // Renk tan�mlar�
    struct GD_Color {
        int r = 255, g = 255, b = 255, a = 255;

        GD_Color() = default;
        GD_Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return { {"r", r}, {"g", g}, {"b", b}, {"a", a} };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_Color FromJson(const nlohmann::json& j) {
            GD_Color color;
            if (j.contains("r") && j["r"].is_number()) color.r = j["r"];
            if (j.contains("g") && j["g"].is_number()) color.g = j["g"];
            if (j.contains("b") && j["b"].is_number()) color.b = j["b"];
            if (j.contains("a") && j["a"].is_number()) color.a = j["a"];
            return color;
        }
    };

    // ESP Ayarlar�
    struct GD_ESPConfig {
        bool enabled = true;
        bool showBox = true;
        bool showHealth = true;
        bool showName = true;
        bool showDistance = false;
        bool showTeammates = true;
        bool showEnemies = true;
        bool showWeapons = false;

        // Farkl� ko�ullar i�in renkler
        GD_Color enemyColor = { 255, 0, 0, 255 };
        GD_Color teammateColor = { 0, 255, 0, 255 };
        GD_Color defusingColor = { 0, 0, 255, 255 };
        GD_Color lowHealthColor = { 255, 165, 0, 255 };

        int boxThickness = 2;
        int textSize = 12;
        float maxRenderDistance = 1000.0f;

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"enabled", enabled},
                {"showBox", showBox},
                {"showHealth", showHealth},
                {"showName", showName},
                {"showDistance", showDistance},
                {"showTeammates", showTeammates},
                {"showEnemies", showEnemies},
                {"showWeapons", showWeapons},
                {"enemyColor", enemyColor.ToJson()},
                {"teammateColor", teammateColor.ToJson()},
                {"defusingColor", defusingColor.ToJson()},
                {"lowHealthColor", lowHealthColor.ToJson()},
                {"boxThickness", boxThickness},
                {"textSize", textSize},
                {"maxRenderDistance", maxRenderDistance}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_ESPConfig FromJson(const nlohmann::json& j) {
            GD_ESPConfig config;

            // Boolean de�erlerini al
            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("showBox") && j["showBox"].is_boolean()) config.showBox = j["showBox"];
            if (j.contains("showHealth") && j["showHealth"].is_boolean()) config.showHealth = j["showHealth"];
            if (j.contains("showName") && j["showName"].is_boolean()) config.showName = j["showName"];
            if (j.contains("showDistance") && j["showDistance"].is_boolean()) config.showDistance = j["showDistance"];
            if (j.contains("showTeammates") && j["showTeammates"].is_boolean()) config.showTeammates = j["showTeammates"];
            if (j.contains("showEnemies") && j["showEnemies"].is_boolean()) config.showEnemies = j["showEnemies"];
            if (j.contains("showWeapons") && j["showWeapons"].is_boolean()) config.showWeapons = j["showWeapons"];

            // Renk de�erlerini al
            if (j.contains("enemyColor") && j["enemyColor"].is_object()) config.enemyColor = GD_Color::FromJson(j["enemyColor"]);
            if (j.contains("teammateColor") && j["teammateColor"].is_object()) config.teammateColor = GD_Color::FromJson(j["teammateColor"]);
            if (j.contains("defusingColor") && j["defusingColor"].is_object()) config.defusingColor = GD_Color::FromJson(j["defusingColor"]);
            if (j.contains("lowHealthColor") && j["lowHealthColor"].is_object()) config.lowHealthColor = GD_Color::FromJson(j["lowHealthColor"]);

            // Say�sal de�erleri al
            if (j.contains("boxThickness") && j["boxThickness"].is_number()) config.boxThickness = j["boxThickness"];
            if (j.contains("textSize") && j["textSize"].is_number()) config.textSize = j["textSize"];
            if (j.contains("maxRenderDistance") && j["maxRenderDistance"].is_number()) config.maxRenderDistance = j["maxRenderDistance"];

            return config;
        }
    };

    // FOV Ayarlar�
    struct GD_FOVConfig {
        bool enabled = false;
        float defaultFOV = 90.0f;    // Varsay�lan FOV de�eri
        float customFOV = 110.0f;    // Kullan�c� tan�ml� FOV de�eri
        bool dynamicFOV = false;     // Dinamik FOV (ko�arken, zoomlarken vs)
        float zoomFactor = 0.8f;     // Zoom fakt�r�

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"enabled", enabled},
                {"defaultFOV", defaultFOV},
                {"customFOV", customFOV},
                {"dynamicFOV", dynamicFOV},
                {"zoomFactor", zoomFactor}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_FOVConfig FromJson(const nlohmann::json& j) {
            GD_FOVConfig config;

            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("defaultFOV") && j["defaultFOV"].is_number()) config.defaultFOV = j["defaultFOV"];
            if (j.contains("customFOV") && j["customFOV"].is_number()) config.customFOV = j["customFOV"];
            if (j.contains("dynamicFOV") && j["dynamicFOV"].is_boolean()) config.dynamicFOV = j["dynamicFOV"];
            if (j.contains("zoomFactor") && j["zoomFactor"].is_number()) config.zoomFactor = j["zoomFactor"];

            return config;
        }
    };

    // Silah Kaplama Bilgisi
    struct GD_WeaponSkin {
        int weaponID = 0;            // Silah ID'si
        int paintKit = 0;            // Kaplama (skin) ID'si
        float wear = 0.0f;           // A��nma de�eri (0.0-1.0)
        int seed = 0;                // Desen tohumu
        int statTrak = -1;           // StatTrak de�eri (-1 = kapal�)
        std::string nameTag = "";    // �sim etiketi

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"weaponID", weaponID},
                {"paintKit", paintKit},
                {"wear", wear},
                {"seed", seed},
                {"statTrak", statTrak},
                {"nameTag", nameTag}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_WeaponSkin FromJson(const nlohmann::json& j) {
            GD_WeaponSkin skin;

            if (j.contains("weaponID") && j["weaponID"].is_number()) skin.weaponID = j["weaponID"];
            if (j.contains("paintKit") && j["paintKit"].is_number()) skin.paintKit = j["paintKit"];
            if (j.contains("wear") && j["wear"].is_number()) skin.wear = j["wear"];
            if (j.contains("seed") && j["seed"].is_number()) skin.seed = j["seed"];
            if (j.contains("statTrak") && j["statTrak"].is_number()) skin.statTrak = j["statTrak"];
            if (j.contains("nameTag") && j["nameTag"].is_string()) skin.nameTag = j["nameTag"];

            return skin;
        }
    };

    // Silah Kaplamalar� Ayarlar�
    struct GD_SkinChangerConfig {
        bool enabled = false;
        std::vector<GD_WeaponSkin> weaponSkins;
        bool updateOnlyOnSpawn = true;   // Sadece spawn olunca g�ncelle
        bool knifeSkinChanger = false;   // B��ak skin de�i�tirici
        int knifeModel = 0;              // B��ak modeli
        int knifeSkin = 0;               // B��ak kaplamas�

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            nlohmann::json j = {
                {"enabled", enabled},
                {"updateOnlyOnSpawn", updateOnlyOnSpawn},
                {"knifeSkinChanger", knifeSkinChanger},
                {"knifeModel", knifeModel},
                {"knifeSkin", knifeSkin},
                {"weaponSkins", nlohmann::json::array()}
            };

            for (const auto& skin : weaponSkins) {
                j["weaponSkins"].push_back(skin.ToJson());
            }

            return j;
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_SkinChangerConfig FromJson(const nlohmann::json& j) {
            GD_SkinChangerConfig config;

            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("updateOnlyOnSpawn") && j["updateOnlyOnSpawn"].is_boolean()) config.updateOnlyOnSpawn = j["updateOnlyOnSpawn"];
            if (j.contains("knifeSkinChanger") && j["knifeSkinChanger"].is_boolean()) config.knifeSkinChanger = j["knifeSkinChanger"];
            if (j.contains("knifeModel") && j["knifeModel"].is_number()) config.knifeModel = j["knifeModel"];
            if (j.contains("knifeSkin") && j["knifeSkin"].is_number()) config.knifeSkin = j["knifeSkin"];

            if (j.contains("weaponSkins") && j["weaponSkins"].is_array()) {
                for (const auto& skin : j["weaponSkins"]) {
                    config.weaponSkins.push_back(GD_WeaponSkin::FromJson(skin));
                }
            }

            return config;
        }
    };

    // Glow (Parlama) Efekti Ayarlar�
    struct GD_GlowConfig {
        bool enabled = false;
        bool showEnemies = true;
        bool showTeammates = false;
        bool showWeapons = false;
        bool showBomb = true;
        GD_Color enemyColor = { 255, 0, 0, 160 };        // K�rm�z�, yar� saydam
        GD_Color teammateColor = { 0, 255, 0, 160 };     // Ye�il, yar� saydam
        GD_Color weaponColor = { 0, 0, 255, 160 };       // Mavi, yar� saydam
        GD_Color bombColor = { 255, 255, 0, 200 };       // Sar�, daha belirgin

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"enabled", enabled},
                {"showEnemies", showEnemies},
                {"showTeammates", showTeammates},
                {"showWeapons", showWeapons},
                {"showBomb", showBomb},
                {"enemyColor", enemyColor.ToJson()},
                {"teammateColor", teammateColor.ToJson()},
                {"weaponColor", weaponColor.ToJson()},
                {"bombColor", bombColor.ToJson()}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_GlowConfig FromJson(const nlohmann::json& j) {
            GD_GlowConfig config;

            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("showEnemies") && j["showEnemies"].is_boolean()) config.showEnemies = j["showEnemies"];
            if (j.contains("showTeammates") && j["showTeammates"].is_boolean()) config.showTeammates = j["showTeammates"];
            if (j.contains("showWeapons") && j["showWeapons"].is_boolean()) config.showWeapons = j["showWeapons"];
            if (j.contains("showBomb") && j["showBomb"].is_boolean()) config.showBomb = j["showBomb"];

            if (j.contains("enemyColor") && j["enemyColor"].is_object()) config.enemyColor = GD_Color::FromJson(j["enemyColor"]);
            if (j.contains("teammateColor") && j["teammateColor"].is_object()) config.teammateColor = GD_Color::FromJson(j["teammateColor"]);
            if (j.contains("weaponColor") && j["weaponColor"].is_object()) config.weaponColor = GD_Color::FromJson(j["weaponColor"]);
            if (j.contains("bombColor") && j["bombColor"].is_object()) config.bombColor = GD_Color::FromJson(j["bombColor"]);

            return config;
        }
    };

    // Radar Hacki Ayarlar�
    struct GD_RadarConfig {
        bool enabled = false;
        bool showEnemies = true;
        bool showTeammates = false;
        bool showBomb = true;

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"enabled", enabled},
                {"showEnemies", showEnemies},
                {"showTeammates", showTeammates},
                {"showBomb", showBomb}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_RadarConfig FromJson(const nlohmann::json& j) {
            GD_RadarConfig config;

            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("showEnemies") && j["showEnemies"].is_boolean()) config.showEnemies = j["showEnemies"];
            if (j.contains("showTeammates") && j["showTeammates"].is_boolean()) config.showTeammates = j["showTeammates"];
            if (j.contains("showBomb") && j["showBomb"].is_boolean()) config.showBomb = j["showBomb"];

            return config;
        }
    };

    // Tu� ayarlar�
    struct GD_KeyBindings {
        int toggleEspKey = VK_F1;
        int toggleMenuKey = VK_INSERT;
        int exitKey = VK_END;

        // JSON'a d�n��t�rme operat�r�
        nlohmann::json ToJson() const {
            return {
                {"toggleEspKey", toggleEspKey},
                {"toggleMenuKey", toggleMenuKey},
                {"exitKey", exitKey}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu
        static GD_KeyBindings FromJson(const nlohmann::json& j) {
            GD_KeyBindings bindings;

            if (j.contains("toggleEspKey") && j["toggleEspKey"].is_number()) bindings.toggleEspKey = j["toggleEspKey"];
            if (j.contains("toggleMenuKey") && j["toggleMenuKey"].is_number()) bindings.toggleMenuKey = j["toggleMenuKey"];
            if (j.contains("exitKey") && j["exitKey"].is_number()) bindings.exitKey = j["exitKey"];

            return bindings;
        }
    };

    // Offset yap�s�n� geni�letelim
    struct GD_Offsets {
        // Mevcut offsetler
        uint32_t dwEntityList = 0;
        uint32_t dwLocalPlayer = 0;
        uint32_t dwViewMatrix = 0;
        uint32_t dwLocalPlayerController = 0;
        uint32_t dwPlantedC4 = 0;

        // Entity offsetleri
        uint32_t m_iHealth = 0;
        uint32_t m_iTeamNum = 0;
        uint32_t m_bIsDefusing = 0;
        uint32_t m_hPlayerPawn = 0;
        uint32_t m_sSanitizedPlayerName = 0;
        uint32_t m_vecAbsOrigin = 0;
        uint32_t m_pGameSceneNode = 0;
        uint32_t m_ArmorValue = 0;
        uint32_t m_pClippingWeapon = 0;
        uint32_t m_flC4Blow = 0;
        uint32_t m_szName = 0;

        // YEN�: FOV ve viewmodel i�in offsetler
        uint32_t m_flFOVRate = 0;
        uint32_t m_iFOV = 0;
        uint32_t m_iDefaultFOV = 0;
        uint32_t m_viewmodelFOV = 0;

        // YEN�: Silah kaplamalar� i�in offsetler
        uint32_t m_hMyWeapons = 0;
        uint32_t m_nFallbackPaintKit = 0;
        uint32_t m_nFallbackSeed = 0;
        uint32_t m_flFallbackWear = 0;
        uint32_t m_nFallbackStatTrak = 0;
        uint32_t m_iItemIDHigh = 0;
        uint32_t m_iItemDefinitionIndex = 0;
        uint32_t m_iAccountID = 0;
        uint32_t m_OriginalOwnerXuidLow = 0;
        uint32_t m_hViewModel = 0;

        // YEN�: Radar ve glow i�in offsetler
        uint32_t m_bSpotted = 0;
        uint32_t m_bSpottedByMask = 0;
        uint32_t m_flDetectedByEnemySensorTime = 0;
        uint32_t m_flFlashDuration = 0;
        uint32_t dwGlowManager = 0;
        uint32_t m_iGlowIndex = 0;

        // YEN�: Oyuncu hareket ve durumu i�in offsetler
        uint32_t m_vecVelocity = 0;
        uint32_t m_fFlags = 0;
        uint32_t m_bIsScoped = 0;
        uint32_t m_bPinPulled = 0;
        uint32_t m_fThrowTime = 0;
        uint32_t m_bHasDefuser = 0;
        uint32_t m_iObserverMode = 0;
        uint32_t m_hObserverTarget = 0;

        // Versiyon bilgisi
        uint32_t build_number = 0;

        // JSON'a d�n��t�rme operat�r� (t�m alanlar� i�erecek �ekilde g�ncelleme)
        nlohmann::json ToJson() const {
            return {
                // Mevcut offsetler
                {"build_number", build_number},
                {"dwEntityList", dwEntityList},
                {"dwLocalPlayer", dwLocalPlayer},
                {"dwViewMatrix", dwViewMatrix},
                {"dwLocalPlayerController", dwLocalPlayerController},
                {"dwPlantedC4", dwPlantedC4},
                {"m_iHealth", m_iHealth},
                {"m_iTeamNum", m_iTeamNum},
                {"m_bIsDefusing", m_bIsDefusing},
                {"m_hPlayerPawn", m_hPlayerPawn},
                {"m_sSanitizedPlayerName", m_sSanitizedPlayerName},
                {"m_vecAbsOrigin", m_vecAbsOrigin},
                {"m_pGameSceneNode", m_pGameSceneNode},
                {"m_ArmorValue", m_ArmorValue},
                {"m_pClippingWeapon", m_pClippingWeapon},
                {"m_flC4Blow", m_flC4Blow},
                {"m_szName", m_szName},

                // YEN�: FOV offsetleri
                {"m_flFOVRate", m_flFOVRate},
                {"m_iFOV", m_iFOV},
                {"m_iDefaultFOV", m_iDefaultFOV},
                {"m_viewmodelFOV", m_viewmodelFOV},

                // YEN�: Silah kaplama offsetleri
                {"m_hMyWeapons", m_hMyWeapons},
                {"m_nFallbackPaintKit", m_nFallbackPaintKit},
                {"m_nFallbackSeed", m_nFallbackSeed},
                {"m_flFallbackWear", m_flFallbackWear},
                {"m_nFallbackStatTrak", m_nFallbackStatTrak},
                {"m_iItemIDHigh", m_iItemIDHigh},
                {"m_iItemDefinitionIndex", m_iItemDefinitionIndex},
                {"m_iAccountID", m_iAccountID},
                {"m_OriginalOwnerXuidLow", m_OriginalOwnerXuidLow},
                {"m_hViewModel", m_hViewModel},

                // YEN�: Radar ve glow offsetleri
                {"m_bSpotted", m_bSpotted},
                {"m_bSpottedByMask", m_bSpottedByMask},
                {"m_flDetectedByEnemySensorTime", m_flDetectedByEnemySensorTime},
                {"m_flFlashDuration", m_flFlashDuration},
                {"dwGlowManager", dwGlowManager},
                {"m_iGlowIndex", m_iGlowIndex},

                // YEN�: Oyuncu hareket ve durum offsetleri
                {"m_vecVelocity", m_vecVelocity},
                {"m_fFlags", m_fFlags},
                {"m_bIsScoped", m_bIsScoped},
                {"m_bPinPulled", m_bPinPulled},
                {"m_fThrowTime", m_fThrowTime},
                {"m_bHasDefuser", m_bHasDefuser},
                {"m_iObserverMode", m_iObserverMode},
                {"m_hObserverTarget", m_hObserverTarget}
            };
        }

        // JSON'dan d�n��t�rme fonksiyonu (t�m alanlar� i�erecek �ekilde g�ncelleme)
        static GD_Offsets FromJson(const nlohmann::json& j) {
            GD_Offsets offsets;

            // Mevcut offsetleri y�kle
            if (j.contains("build_number") && j["build_number"].is_number_unsigned()) offsets.build_number = j["build_number"];
            if (j.contains("dwEntityList") && j["dwEntityList"].is_number_unsigned()) offsets.dwEntityList = j["dwEntityList"];
            if (j.contains("dwLocalPlayer") && j["dwLocalPlayer"].is_number_unsigned()) offsets.dwLocalPlayer = j["dwLocalPlayer"];
            if (j.contains("dwViewMatrix") && j["dwViewMatrix"].is_number_unsigned()) offsets.dwViewMatrix = j["dwViewMatrix"];
            if (j.contains("dwLocalPlayerController") && j["dwLocalPlayerController"].is_number_unsigned()) offsets.dwLocalPlayerController = j["dwLocalPlayerController"];
            if (j.contains("dwPlantedC4") && j["dwPlantedC4"].is_number_unsigned()) offsets.dwPlantedC4 = j["dwPlantedC4"];
            if (j.contains("m_iHealth") && j["m_iHealth"].is_number_unsigned()) offsets.m_iHealth = j["m_iHealth"];
            if (j.contains("m_iTeamNum") && j["m_iTeamNum"].is_number_unsigned()) offsets.m_iTeamNum = j["m_iTeamNum"];
            if (j.contains("m_bIsDefusing") && j["m_bIsDefusing"].is_number_unsigned()) offsets.m_bIsDefusing = j["m_bIsDefusing"];
            if (j.contains("m_hPlayerPawn") && j["m_hPlayerPawn"].is_number_unsigned()) offsets.m_hPlayerPawn = j["m_hPlayerPawn"];
            if (j.contains("m_sSanitizedPlayerName") && j["m_sSanitizedPlayerName"].is_number_unsigned()) offsets.m_sSanitizedPlayerName = j["m_sSanitizedPlayerName"];
            if (j.contains("m_vecAbsOrigin") && j["m_vecAbsOrigin"].is_number_unsigned()) offsets.m_vecAbsOrigin = j["m_vecAbsOrigin"];
            if (j.contains("m_pGameSceneNode") && j["m_pGameSceneNode"].is_number_unsigned()) offsets.m_pGameSceneNode = j["m_pGameSceneNode"];
            if (j.contains("m_ArmorValue") && j["m_ArmorValue"].is_number_unsigned()) offsets.m_ArmorValue = j["m_ArmorValue"];
            if (j.contains("m_pClippingWeapon") && j["m_pClippingWeapon"].is_number_unsigned()) offsets.m_pClippingWeapon = j["m_pClippingWeapon"];
            if (j.contains("m_flC4Blow") && j["m_flC4Blow"].is_number_unsigned()) offsets.m_flC4Blow = j["m_flC4Blow"];
            if (j.contains("m_szName") && j["m_szName"].is_number_unsigned()) offsets.m_szName = j["m_szName"];

            // YEN�: FOV offsetlerini y�kle
            if (j.contains("m_flFOVRate") && j["m_flFOVRate"].is_number_unsigned()) offsets.m_flFOVRate = j["m_flFOVRate"];
            if (j.contains("m_iFOV") && j["m_iFOV"].is_number_unsigned()) offsets.m_iFOV = j["m_iFOV"];
            if (j.contains("m_iDefaultFOV") && j["m_iDefaultFOV"].is_number_unsigned()) offsets.m_iDefaultFOV = j["m_iDefaultFOV"];
            if (j.contains("m_viewmodelFOV") && j["m_viewmodelFOV"].is_number_unsigned()) offsets.m_viewmodelFOV = j["m_viewmodelFOV"];

            // YEN�: Silah kaplama offsetlerini y�kle
            if (j.contains("m_hMyWeapons") && j["m_hMyWeapons"].is_number_unsigned()) offsets.m_hMyWeapons = j["m_hMyWeapons"];
            if (j.contains("m_nFallbackPaintKit") && j["m_nFallbackPaintKit"].is_number_unsigned()) offsets.m_nFallbackPaintKit = j["m_nFallbackPaintKit"];
            if (j.contains("m_nFallbackSeed") && j["m_nFallbackSeed"].is_number_unsigned()) offsets.m_nFallbackSeed = j["m_nFallbackSeed"];
            if (j.contains("m_flFallbackWear") && j["m_flFallbackWear"].is_number_unsigned()) offsets.m_flFallbackWear = j["m_flFallbackWear"];
            if (j.contains("m_nFallbackStatTrak") && j["m_nFallbackStatTrak"].is_number_unsigned()) offsets.m_nFallbackStatTrak = j["m_nFallbackStatTrak"];
            if (j.contains("m_iItemIDHigh") && j["m_iItemIDHigh"].is_number_unsigned()) offsets.m_iItemIDHigh = j["m_iItemIDHigh"];
            if (j.contains("m_iItemDefinitionIndex") && j["m_iItemDefinitionIndex"].is_number_unsigned()) offsets.m_iItemDefinitionIndex = j["m_iItemDefinitionIndex"];
            if (j.contains("m_iAccountID") && j["m_iAccountID"].is_number_unsigned()) offsets.m_iAccountID = j["m_iAccountID"];
            if (j.contains("m_OriginalOwnerXuidLow") && j["m_OriginalOwnerXuidLow"].is_number_unsigned()) offsets.m_OriginalOwnerXuidLow = j["m_OriginalOwnerXuidLow"];
            if (j.contains("m_hViewModel") && j["m_hViewModel"].is_number_unsigned()) offsets.m_hViewModel = j["m_hViewModel"];

            // YEN�: Radar ve glow offsetlerini y�kle
            if (j.contains("m_bSpotted") && j["m_bSpotted"].is_number_unsigned()) offsets.m_bSpotted = j["m_bSpotted"];
            if (j.contains("m_bSpottedByMask") && j["m_bSpottedByMask"].is_number_unsigned()) offsets.m_bSpottedByMask = j["m_bSpottedByMask"];
            if (j.contains("m_flDetectedByEnemySensorTime") && j["m_flDetectedByEnemySensorTime"].is_number_unsigned()) offsets.m_flDetectedByEnemySensorTime = j["m_flDetectedByEnemySensorTime"];
            if (j.contains("m_flFlashDuration") && j["m_flFlashDuration"].is_number_unsigned()) offsets.m_flFlashDuration = j["m_flFlashDuration"];
            if (j.contains("dwGlowManager") && j["dwGlowManager"].is_number_unsigned()) offsets.dwGlowManager = j["dwGlowManager"];
            if (j.contains("m_iGlowIndex") && j["m_iGlowIndex"].is_number_unsigned()) offsets.m_iGlowIndex = j["m_iGlowIndex"];

            // YEN�: Oyuncu hareket ve durum offsetlerini y�kle
            if (j.contains("m_vecVelocity") && j["m_vecVelocity"].is_number_unsigned()) offsets.m_vecVelocity = j["m_vecVelocity"];
            if (j.contains("m_fFlags") && j["m_fFlags"].is_number_unsigned()) offsets.m_fFlags = j["m_fFlags"];
            if (j.contains("m_bIsScoped") && j["m_bIsScoped"].is_number_unsigned()) offsets.m_bIsScoped = j["m_bIsScoped"];
            if (j.contains("m_bPinPulled") && j["m_bPinPulled"].is_number_unsigned()) offsets.m_bPinPulled = j["m_bPinPulled"];
            if (j.contains("m_fThrowTime") && j["m_fThrowTime"].is_number_unsigned()) offsets.m_fThrowTime = j["m_fThrowTime"];
            if (j.contains("m_bHasDefuser") && j["m_bHasDefuser"].is_number_unsigned()) offsets.m_bHasDefuser = j["m_bHasDefuser"];
            if (j.contains("m_iObserverMode") && j["m_iObserverMode"].is_number_unsigned()) offsets.m_iObserverMode = j["m_iObserverMode"];
            if (j.contains("m_hObserverTarget") && j["m_hObserverTarget"].is_number_unsigned()) offsets.m_hObserverTarget = j["m_hObserverTarget"];

            return offsets;
        }
    };

    // Ana yap�land�rma s�n�f� - G�ncellenmi� versiyon
    class ConfigManager {
    private:
        GD_ESPConfig m_espConfig;
        GD_FOVConfig m_fovConfig;                    // YEN�
        GD_SkinChangerConfig m_skinChangerConfig;    // YEN�
        GD_GlowConfig m_glowConfig;                  // YEN�
        GD_RadarConfig m_radarConfig;                // YEN�
        GD_KeyBindings m_keyBindings;
        GD_Offsets m_offsets;
        std::string m_configFilePath = "config.json";
        std::string m_offsetsFilePath = "offsets.json";
        bool m_configLoaded = false;
        bool m_offsetsLoaded = false;

    public:
        ConfigManager() {
            GD_LoadConfig();
            GD_LoadOffsets();
        }

        // Yap�land�rmay� dosyadan y�kle - G�ncellenmi� versiyon
        bool GD_LoadConfig(const std::string& filePath = "") {
            if (!filePath.empty()) m_configFilePath = filePath;

            try {
                // Dosya var m� kontrol et
                if (!std::filesystem::exists(m_configFilePath)) {
                    std::cout << "[geezy_digital] Yap�land�rma dosyas� bulunamad�, varsay�lan de�erler kullan�l�yor." << std::endl;
                    m_configLoaded = false;
                    return false;
                }

                // Dosyay� a� ve oku
                std::ifstream file(m_configFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Yap�land�rma dosyas� a��lamad�, varsay�lan de�erler kullan�l�yor." << std::endl;
                    m_configLoaded = false;
                    return false;
                }

                // JSON'� parse et
                nlohmann::json j;
                file >> j;
                file.close();

                // Yap�land�rma de�erlerini oku
                if (j.contains("espConfig") && j["espConfig"].is_object()) {
                    m_espConfig = GD_ESPConfig::FromJson(j["espConfig"]);
                }

                // Yeni yap�land�rma de�erlerini oku
                if (j.contains("fovConfig") && j["fovConfig"].is_object()) {
                    m_fovConfig = GD_FOVConfig::FromJson(j["fovConfig"]);
                }

                if (j.contains("skinChangerConfig") && j["skinChangerConfig"].is_object()) {
                    m_skinChangerConfig = GD_SkinChangerConfig::FromJson(j["skinChangerConfig"]);
                }

                if (j.contains("glowConfig") && j["glowConfig"].is_object()) {
                    m_glowConfig = GD_GlowConfig::FromJson(j["glowConfig"]);
                }

                if (j.contains("radarConfig") && j["radarConfig"].is_object()) {
                    m_radarConfig = GD_RadarConfig::FromJson(j["radarConfig"]);
                }

                if (j.contains("keyBindings") && j["keyBindings"].is_object()) {
                    m_keyBindings = GD_KeyBindings::FromJson(j["keyBindings"]);
                }

                m_configLoaded = true;
                std::cout << "[geezy_digital] Yap�land�rma ba�ar�yla y�klendi: " << m_configFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Yap�land�rma y�klenirken hata olu�tu: " << e.what() << std::endl;
                m_configLoaded = false;
                return false;
            }
        }

        // Yap�land�rmay� dosyaya kaydet - G�ncellenmi� versiyon
        bool GD_SaveConfig(const std::string& filePath = "") {
            if (!filePath.empty()) m_configFilePath = filePath;

            try {
                // JSON olu�tur
                nlohmann::json j;
                j["espConfig"] = m_espConfig.ToJson();
                j["fovConfig"] = m_fovConfig.ToJson();                     // YEN�
                j["skinChangerConfig"] = m_skinChangerConfig.ToJson();     // YEN�
                j["glowConfig"] = m_glowConfig.ToJson();                   // YEN�
                j["radarConfig"] = m_radarConfig.ToJson();                 // YEN�
                j["keyBindings"] = m_keyBindings.ToJson();

                // Dosyay� a�
                std::ofstream file(m_configFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Yap�land�rma dosyas� olu�turulamad�!" << std::endl;
                    return false;
                }

                // JSON'� yaz
                file << j.dump(4); // 4 karakter girinti ile
                file.close();

                std::cout << "[geezy_digital] Yap�land�rma ba�ar�yla kaydedildi: " << m_configFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Yap�land�rma kaydedilirken hata olu�tu: " << e.what() << std::endl;
                return false;
            }
        }

        // Offset'leri dosyadan y�kle
        bool GD_LoadOffsets(const std::string& filePath = "") {
            if (!filePath.empty()) m_offsetsFilePath = filePath;

            try {
                // Dosya var m� kontrol et
                if (!std::filesystem::exists(m_offsetsFilePath)) {
                    std::cout << "[geezy_digital] Offset dosyas� bulunamad�!" << std::endl;
                    m_offsetsLoaded = false;
                    return false;
                }

                // Dosyay� a� ve oku
                std::ifstream file(m_offsetsFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Offset dosyas� a��lamad�!" << std::endl;
                    m_offsetsLoaded = false;
                    return false;
                }

                // JSON'� parse et
                nlohmann::json j;
                file >> j;
                file.close();

                // Offset de�erlerini oku
                m_offsets = GD_Offsets::FromJson(j);

                m_offsetsLoaded = true;
                std::cout << "[geezy_digital] Offset'ler ba�ar�yla y�klendi. Build numaras�: " << m_offsets.build_number << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Offset'ler y�klenirken hata olu�tu: " << e.what() << std::endl;
                m_offsetsLoaded = false;
                return false;
            }
        }

        // Offset'leri dosyaya kaydet
        bool GD_SaveOffsets(const std::string& filePath = "") {
            if (!filePath.empty()) m_offsetsFilePath = filePath;

            try {
                // JSON olu�tur
                nlohmann::json j = m_offsets.ToJson();

                // Dosyay� a�
                std::ofstream file(m_offsetsFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Offset dosyas� olu�turulamad�!" << std::endl;
                    return false;
                }

                // JSON'� yaz
                file << j.dump(4); // 4 karakter girinti ile
                file.close();

                std::cout << "[geezy_digital] Offset'ler ba�ar�yla kaydedildi: " << m_offsetsFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Offset'ler kaydedilirken hata olu�tu: " << e.what() << std::endl;
                return false;
            }
        }

        // Yeni getter metodlar�
        GD_FOVConfig& GetFOVConfig() { return m_fovConfig; }
        GD_SkinChangerConfig& GetSkinChangerConfig() { return m_skinChangerConfig; }
        GD_GlowConfig& GetGlowConfig() { return m_glowConfig; }
        GD_RadarConfig& GetRadarConfig() { return m_radarConfig; }

        // Mevcut getter metodlar�
        GD_ESPConfig& GetESPConfig() { return m_espConfig; }
        GD_KeyBindings& GetKeyBindings() { return m_keyBindings; }
        GD_Offsets& GetOffsets() { return m_offsets; }

        bool IsConfigLoaded() const { return m_configLoaded; }
        bool AreOffsetsLoaded() const { return m_offsetsLoaded; }
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_CONFIG_HPP_