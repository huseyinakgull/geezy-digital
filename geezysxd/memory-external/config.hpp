#ifndef _GEEZY_DIGITAL_CONFIG_HPP_
#define _GEEZY_DIGITAL_CONFIG_HPP_

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include "../memory-external/classes/json.hpp"

namespace geezy_digital {

    // Uygulamanýn temel ayarlarý
    struct GD_AppConfig {
        bool enableLogging = true;
        bool showDebugInfo = false;

        // JSON'a dönüþtürme operatörü
        nlohmann::json ToJson() const {
            return {
                {"enableLogging", enableLogging},
                {"showDebugInfo", showDebugInfo}
            };
        }

        // JSON'dan dönüþtürme fonksiyonu
        static GD_AppConfig FromJson(const nlohmann::json& j) {
            GD_AppConfig config;
            if (j.contains("enableLogging") && j["enableLogging"].is_boolean()) config.enableLogging = j["enableLogging"];
            if (j.contains("showDebugInfo") && j["showDebugInfo"].is_boolean()) config.showDebugInfo = j["showDebugInfo"];
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

    // Offset ayarlarý - JSON'dan yükleme yerine sabit deðerlerle tanýmlandý
    struct GD_Offsets {
        // Güncel build numarasý
        const uint32_t build_number = 14070;

        // Temel offsetler
        const uint32_t dwBuildNumber = 5508068;
        const uint32_t dwEntityList = 27486688;
        const uint32_t dwLocalPlayer = 25734928;
        const uint32_t dwViewMatrix = 27928528;
        const uint32_t dwLocalPlayerController = 27820128;
        const uint32_t dwPlantedC4 = 27949520;

        // Entity offsetleri
        const uint32_t m_iHealth = 836;
        const uint32_t m_iTeamNum = 995;
        const uint32_t m_bIsDefusing = 9194;
        const uint32_t m_hPlayerPawn = 2060;
        const uint32_t m_sSanitizedPlayerName = 1904;
        const uint32_t m_vecAbsOrigin = 208;
        const uint32_t m_pGameSceneNode = 808;
        const uint32_t m_ArmorValue = 9244;
        const uint32_t m_pClippingWeapon = 5024;
        const uint32_t m_flC4Blow = 4032;
        const uint32_t m_szName = 3360;
        const uint32_t m_flFlashOverlayAlpha = 5120;
        const uint32_t m_flNextBeep = 4028;
        const uint32_t m_flTimerLength = 4040;
        const uint32_t m_iAccount = 64;
        const uint32_t m_pInGameMoneyServices = 1824;
        const uint32_t m_vOldOrigin = 4900;
    };

    // Ana yapýlandýrma sýnýfý
    class ConfigManager {
    private:
        GD_AppConfig m_appConfig;
        GD_KeyBindings m_keyBindings;
        GD_Offsets m_offsets;
        std::string m_configFilePath = "config.json";
        bool m_configLoaded = false;

    public:
        ConfigManager() {
            GD_LoadConfig();
            std::cout << "[geezy_digital] Offsetler hazýr, build numarasý: " << m_offsets.build_number << std::endl;
        }
        const GD_Offsets& GetOffsets() const { return m_offsets; }

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
                if (j.contains("appConfig") && j["appConfig"].is_object()) {
                    m_appConfig = GD_AppConfig::FromJson(j["appConfig"]);
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
                j["appConfig"] = m_appConfig.ToJson();
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

        // Getter metodlarý
        GD_AppConfig& GetAppConfig() { return m_appConfig; }
        GD_KeyBindings& GetKeyBindings() { return m_keyBindings; }
        const GD_Offsets& GetOffsets() const { return m_offsets; }

        bool IsConfigLoaded() const { return m_configLoaded; }
        bool AreOffsetsLoaded() const { return true; } // Sabit tanýmlý offsetler her zaman hazýr
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_CONFIG_HPP_