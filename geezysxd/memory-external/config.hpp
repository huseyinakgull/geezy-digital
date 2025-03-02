#ifndef _GEEZY_DIGITAL_CONFIG_HPP_
#define _GEEZY_DIGITAL_CONFIG_HPP_

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include "../memory-external/classes/json.hpp"

namespace geezy_digital {

    // Renk tanýmlarý
    struct GD_Color {
        int r = 255, g = 255, b = 255, a = 255;

        GD_Color() = default;
        GD_Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}

        // JSON'a dönüþtürme operatörü
        nlohmann::json ToJson() const {
            return { {"r", r}, {"g", g}, {"b", b}, {"a", a} };
        }

        // JSON'dan dönüþtürme fonksiyonu
        static GD_Color FromJson(const nlohmann::json& j) {
            GD_Color color;
            if (j.contains("r") && j["r"].is_number()) color.r = j["r"];
            if (j.contains("g") && j["g"].is_number()) color.g = j["g"];
            if (j.contains("b") && j["b"].is_number()) color.b = j["b"];
            if (j.contains("a") && j["a"].is_number()) color.a = j["a"];
            return color;
        }
    };

    // ESP Ayarlarý
    struct GD_ESPConfig {
        bool enabled = true;
        bool showBox = true;
        bool showHealth = true;
        bool showName = true;
        bool showDistance = false;
        bool showTeammates = true;
        bool showEnemies = true;
        bool showWeapons = false;

        // Farklý koþullar için renkler
        GD_Color enemyColor = { 255, 0, 0, 255 };
        GD_Color teammateColor = { 0, 255, 0, 255 };
        GD_Color defusingColor = { 0, 0, 255, 255 };
        GD_Color lowHealthColor = { 255, 165, 0, 255 };

        int boxThickness = 2;
        int textSize = 12;
        float maxRenderDistance = 1000.0f;

        // JSON'a dönüþtürme operatörü
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

        // JSON'dan dönüþtürme fonksiyonu
        static GD_ESPConfig FromJson(const nlohmann::json& j) {
            GD_ESPConfig config;

            // Boolean deðerlerini al
            if (j.contains("enabled") && j["enabled"].is_boolean()) config.enabled = j["enabled"];
            if (j.contains("showBox") && j["showBox"].is_boolean()) config.showBox = j["showBox"];
            if (j.contains("showHealth") && j["showHealth"].is_boolean()) config.showHealth = j["showHealth"];
            if (j.contains("showName") && j["showName"].is_boolean()) config.showName = j["showName"];
            if (j.contains("showDistance") && j["showDistance"].is_boolean()) config.showDistance = j["showDistance"];
            if (j.contains("showTeammates") && j["showTeammates"].is_boolean()) config.showTeammates = j["showTeammates"];
            if (j.contains("showEnemies") && j["showEnemies"].is_boolean()) config.showEnemies = j["showEnemies"];
            if (j.contains("showWeapons") && j["showWeapons"].is_boolean()) config.showWeapons = j["showWeapons"];

            // Renk deðerlerini al
            if (j.contains("enemyColor") && j["enemyColor"].is_object()) config.enemyColor = GD_Color::FromJson(j["enemyColor"]);
            if (j.contains("teammateColor") && j["teammateColor"].is_object()) config.teammateColor = GD_Color::FromJson(j["teammateColor"]);
            if (j.contains("defusingColor") && j["defusingColor"].is_object()) config.defusingColor = GD_Color::FromJson(j["defusingColor"]);
            if (j.contains("lowHealthColor") && j["lowHealthColor"].is_object()) config.lowHealthColor = GD_Color::FromJson(j["lowHealthColor"]);

            // Sayýsal deðerleri al
            if (j.contains("boxThickness") && j["boxThickness"].is_number()) config.boxThickness = j["boxThickness"];
            if (j.contains("textSize") && j["textSize"].is_number()) config.textSize = j["textSize"];
            if (j.contains("maxRenderDistance") && j["maxRenderDistance"].is_number()) config.maxRenderDistance = j["maxRenderDistance"];

            return config;
        }
    };

    // Tuþ ayarlarý
    struct GD_KeyBindings {
        int toggleEspKey = VK_F1;
        int toggleMenuKey = VK_INSERT;
        int exitKey = VK_END;

        // JSON'a dönüþtürme operatörü
        nlohmann::json ToJson() const {
            return {
                {"toggleEspKey", toggleEspKey},
                {"toggleMenuKey", toggleMenuKey},
                {"exitKey", exitKey}
            };
        }

        // JSON'dan dönüþtürme fonksiyonu
        static GD_KeyBindings FromJson(const nlohmann::json& j) {
            GD_KeyBindings bindings;

            if (j.contains("toggleEspKey") && j["toggleEspKey"].is_number()) bindings.toggleEspKey = j["toggleEspKey"];
            if (j.contains("toggleMenuKey") && j["toggleMenuKey"].is_number()) bindings.toggleMenuKey = j["toggleMenuKey"];
            if (j.contains("exitKey") && j["exitKey"].is_number()) bindings.exitKey = j["exitKey"];

            return bindings;
        }
    };

    // Offset ayarlarý
    struct GD_Offsets {
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

        // Versiyon bilgisi
        uint32_t build_number = 0;

        // JSON'a dönüþtürme operatörü
        nlohmann::json ToJson() const {
            return {
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
                {"m_szName", m_szName}
            };
        }

        // JSON'dan dönüþtürme fonksiyonu
        static GD_Offsets FromJson(const nlohmann::json& j) {
            GD_Offsets offsets;

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

            return offsets;
        }
    };

    // Ana yapýlandýrma sýnýfý
    class ConfigManager {
    private:
        GD_ESPConfig m_espConfig;
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

        // Yapýlandýrmayý dosyadan yükle
        bool GD_LoadConfig(const std::string& filePath = "") {
            if (!filePath.empty()) m_configFilePath = filePath;

            try {
                // Dosya var mý kontrol et
                if (!std::filesystem::exists(m_configFilePath)) {
                    std::cout << "[geezy_digital] Yapýlandýrma dosyasý bulunamadý, varsayýlan deðerler kullanýlýyor." << std::endl;
                    m_configLoaded = false;
                    return false;
                }

                // Dosyayý aç ve oku
                std::ifstream file(m_configFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Yapýlandýrma dosyasý açýlamadý, varsayýlan deðerler kullanýlýyor." << std::endl;
                    m_configLoaded = false;
                    return false;
                }

                // JSON'ý parse et
                nlohmann::json j;
                file >> j;
                file.close();

                // Yapýlandýrma deðerlerini oku
                if (j.contains("espConfig") && j["espConfig"].is_object()) {
                    m_espConfig = GD_ESPConfig::FromJson(j["espConfig"]);
                }

                if (j.contains("keyBindings") && j["keyBindings"].is_object()) {
                    m_keyBindings = GD_KeyBindings::FromJson(j["keyBindings"]);
                }

                m_configLoaded = true;
                std::cout << "[geezy_digital] Yapýlandýrma baþarýyla yüklendi: " << m_configFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Yapýlandýrma yüklenirken hata oluþtu: " << e.what() << std::endl;
                m_configLoaded = false;
                return false;
            }
        }

        // Yapýlandýrmayý dosyaya kaydet
        bool GD_SaveConfig(const std::string& filePath = "") {
            if (!filePath.empty()) m_configFilePath = filePath;

            try {
                // JSON oluþtur
                nlohmann::json j;
                j["espConfig"] = m_espConfig.ToJson();
                j["keyBindings"] = m_keyBindings.ToJson();

                // Dosyayý aç
                std::ofstream file(m_configFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Yapýlandýrma dosyasý oluþturulamadý!" << std::endl;
                    return false;
                }

                // JSON'ý yaz
                file << j.dump(4); // 4 karakter girinti ile
                file.close();

                std::cout << "[geezy_digital] Yapýlandýrma baþarýyla kaydedildi: " << m_configFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Yapýlandýrma kaydedilirken hata oluþtu: " << e.what() << std::endl;
                return false;
            }
        }

        // Offset'leri dosyadan yükle
        bool GD_LoadOffsets(const std::string& filePath = "") {
            if (!filePath.empty()) m_offsetsFilePath = filePath;

            try {
                // Dosya var mý kontrol et
                if (!std::filesystem::exists(m_offsetsFilePath)) {
                    std::cout << "[geezy_digital] Offset dosyasý bulunamadý!" << std::endl;
                    m_offsetsLoaded = false;
                    return false;
                }

                // Dosyayý aç ve oku
                std::ifstream file(m_offsetsFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Offset dosyasý açýlamadý!" << std::endl;
                    m_offsetsLoaded = false;
                    return false;
                }

                // JSON'ý parse et
                nlohmann::json j;
                file >> j;
                file.close();

                // Offset deðerlerini oku
                m_offsets = GD_Offsets::FromJson(j);

                m_offsetsLoaded = true;
                std::cout << "[geezy_digital] Offset'ler baþarýyla yüklendi. Build numarasý: " << m_offsets.build_number << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Offset'ler yüklenirken hata oluþtu: " << e.what() << std::endl;
                m_offsetsLoaded = false;
                return false;
            }
        }

        // Offset'leri dosyaya kaydet
        bool GD_SaveOffsets(const std::string& filePath = "") {
            if (!filePath.empty()) m_offsetsFilePath = filePath;

            try {
                // JSON oluþtur
                nlohmann::json j = m_offsets.ToJson();

                // Dosyayý aç
                std::ofstream file(m_offsetsFilePath);
                if (!file.is_open()) {
                    std::cout << "[geezy_digital] Offset dosyasý oluþturulamadý!" << std::endl;
                    return false;
                }

                // JSON'ý yaz
                file << j.dump(4); // 4 karakter girinti ile
                file.close();

                std::cout << "[geezy_digital] Offset'ler baþarýyla kaydedildi: " << m_offsetsFilePath << std::endl;
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[geezy_digital] Offset'ler kaydedilirken hata oluþtu: " << e.what() << std::endl;
                return false;
            }
        }

        // Getter metodlarý
        GD_ESPConfig& GetESPConfig() { return m_espConfig; }
        GD_KeyBindings& GetKeyBindings() { return m_keyBindings; }
        GD_Offsets& GetOffsets() { return m_offsets; }

        bool IsConfigLoaded() const { return m_configLoaded; }
        bool AreOffsetsLoaded() const { return m_offsetsLoaded; }
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_CONFIG_HPP_