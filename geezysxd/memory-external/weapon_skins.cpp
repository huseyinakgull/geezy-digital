#include "weapon_skins.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace geezy_digital {

    // WeaponOffsets sýnýfý
    void WeaponOffsets::UpdateFromConfig(const Config::ConfigManager& configManager) {
        // Konfigürasyon dosyasýndan offset deðerlerini oku
        itemDefinitionIndex = configManager.GetWeaponOffset("ItemDefinitionIndex", 12202);
        fallbackPaintKit = configManager.GetWeaponOffset("FallbackPaintKit", 12744);
        fallbackWear = configManager.GetWeaponOffset("FallbackWear", 12752);
        fallbackSeed = configManager.GetWeaponOffset("FallbackSeed", 12748);
        itemIDHigh = configManager.GetWeaponOffset("ItemIDHigh", 12224);
        accountID = configManager.GetWeaponOffset("AccountID", 12232);
        fallbackStatTrak = configManager.GetWeaponOffset("FallbackStatTrak", 12756);
        entityQuality = configManager.GetWeaponOffset("EntityQuality", 12204);

        std::cout << "[BILGI] Weapon offsetleri yuklendi" << std::endl;
    }

    // WeaponSkinManager sýnýfý
    WeaponSkinManager::WeaponSkinManager(Memory::MemoryManager& memoryManager, Config::ConfigManager& configManager)
        : m_memoryManager(memoryManager), m_configManager(configManager), m_enabled(false), m_autoUpdate(false), m_updateInterval(1000) {

        // Offsetleri yapýlandýrmadan yükle
        m_offsets.UpdateFromConfig(configManager);

        // Ayarlarý yapýlandýrmadan yükle
        m_enabled = configManager.GetBool("SkinChanger.Enabled", false);
        m_autoUpdate = configManager.GetBool("SkinChanger.AutoUpdate", false);
        m_updateInterval = configManager.GetInt("SkinChanger.UpdateInterval", 1000);

        // Skinleri yükle
        LoadSkins();
    }

    bool WeaponSkinManager::LoadSkins() {
        try {
            // Ayarlardan skin bilgilerini oku
            json skinsJson = m_configManager.GetSection("Skins");
            if (skinsJson.empty()) {
                std::cout << "[BILGI] Skin bulunamadi veya bos" << std::endl;
                return true;
            }

            m_skins.clear();

            for (const auto& skinItem : skinsJson) {
                SkinInfo skin;
                int weaponID = skinItem.value("WeaponID", 0);

                skin.id = skinItem.value("SkinID", 0);
                skin.name = skinItem.value("Name", "");
                skin.wear = skinItem.value("Wear", 0.01f);
                skin.seed = skinItem.value("Seed", 0);
                skin.statTrak = skinItem.value("StatTrak", -1);
                skin.enabled = skinItem.value("Enabled", true);

                m_skins[weaponID] = skin;
            }

            std::cout << "[BASARI] " << m_skins.size() << " adet skin yuklendi" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << "[HATA] Skinler yuklenemedi: " << e.what() << std::endl;
            return false;
        }
    }

    bool WeaponSkinManager::SaveSkins() {
        try {
            // Skinleri JSON dizisine dönüþtür
            json skinsArray = json::array();

            for (const auto& [weaponID, skin] : m_skins) {
                json skinItem;
                skinItem["WeaponID"] = weaponID;
                skinItem["SkinID"] = skin.id;
                skinItem["Name"] = skin.name;
                skinItem["Wear"] = skin.wear;
                skinItem["Seed"] = skin.seed;
                skinItem["StatTrak"] = skin.statTrak;
                skinItem["Enabled"] = skin.enabled;

                skinsArray.push_back(skinItem);
            }

            // Ayarlar nesnesini güncelle
            m_configManager.Set("Skins", skinsArray);
            m_configManager.SetNested("SkinChanger", "Enabled", m_enabled);
            m_configManager.SetNested("SkinChanger", "AutoUpdate", m_autoUpdate);
            m_configManager.SetNested("SkinChanger", "UpdateInterval", m_updateInterval);

            // Ayarlarý kaydet
            m_configManager.SaveConfig();

            std::cout << "[BASARI] " << m_skins.size() << " adet skin kaydedildi" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << "[HATA] Skinler kaydedilemedi: " << e.what() << std::endl;
            return false;
        }
    }

    void WeaponSkinManager::SetSkin(int weaponID, int skinID, float wear, int seed, int statTrak) {
        SkinInfo skin;
        skin.id = skinID;
        skin.name = "Custom Skin"; // Ýsim için özel API çaðrýsý yapýlabilir
        skin.wear = wear;
        skin.seed = seed;
        skin.statTrak = statTrak;
        skin.enabled = true;

        m_skins[weaponID] = skin;
        std::cout << "[BILGI] Weapon ID " << weaponID << " icin skin ayarlandi: " << skinID << std::endl;
    }

    void WeaponSkinManager::ProcessWeapons() {
        // Local player pointer
        uintptr_t clientModule = m_memoryManager.GetModuleBaseAddress("client.dll");
        uintptr_t localPlayerPtr = m_memoryManager.Read<uintptr_t>(
            clientModule + m_configManager.GetOffset("dwLocalPlayer")
        );

        if (!localPlayerPtr) {
            return;
        }

        // Entity list
        uintptr_t entityList = clientModule + m_configManager.GetOffset("dwEntityList");

        // Read local player ID
        int localPlayerID = m_memoryManager.Read<int>(localPlayerPtr + m_configManager.GetOffset("m_iTeamNum"));

        // Weapon count is typically 64 (maximum number of entities/weapons to check)
        constexpr int MAX_WEAPONS = 64;

        for (int i = 0; i < MAX_WEAPONS; i++) {
            uintptr_t entity = m_memoryManager.Read<uintptr_t>(entityList + i * 0x10);

            if (!entity) {
                continue;
            }

            // Check if weapon
            int itemDefinitionIndex = m_memoryManager.Read<int>(entity + m_offsets.itemDefinitionIndex);

            // Skip if not a valid weapon
            if (itemDefinitionIndex <= 0 || itemDefinitionIndex > 100) {
                continue;
            }

            // Find skin for this weapon
            SkinInfo* skin = GetSkinInfoByWeaponID(itemDefinitionIndex);

            if (skin && skin->enabled) {
                // Apply skin attributes
                m_memoryManager.Write<int>(entity + m_offsets.fallbackPaintKit, skin->id);
                m_memoryManager.Write<float>(entity + m_offsets.fallbackWear, skin->wear);
                m_memoryManager.Write<int>(entity + m_offsets.fallbackSeed, skin->seed);

                // If StatTrak is enabled
                if (skin->statTrak >= 0) {
                    m_memoryManager.Write<int>(entity + m_offsets.fallbackStatTrak, skin->statTrak);
                    m_memoryManager.Write<int>(entity + m_offsets.entityQuality, 9); // Set quality to StatTrak
                }

                // Force item ID high to be -1 (bypasses valve server verification)
                m_memoryManager.Write<int>(entity + m_offsets.itemIDHigh, -1);

                // Set account ID to current player for ownership
                int accountID = m_memoryManager.Read<int>(localPlayerPtr + m_offsets.accountID);
                m_memoryManager.Write<int>(entity + m_offsets.accountID, accountID);
            }
        }
    }

    void WeaponSkinManager::ApplySkins() {
        if (!m_enabled) {
            std::cout << "[BILGI] Skin Changer devre disi, skinler uygulanmadi" << std::endl;
            return;
        }

        ProcessWeapons();
        std::cout << "[BILGI] Skinler uygulandi" << std::endl;
    }

    void WeaponSkinManager::StartAutoUpdate() {
        m_autoUpdate = true;
        std::cout << "[BILGI] Otomatik skin guncelleme etkinlestirildi" << std::endl;
    }

    void WeaponSkinManager::StopAutoUpdate() {
        m_autoUpdate = false;
        std::cout << "[BILGI] Otomatik skin guncelleme durduruldu" << std::endl;
    }

    bool WeaponSkinManager::IsAutoUpdateEnabled() const {
        return m_autoUpdate;
    }

    void WeaponSkinManager::Enable(bool enable) {
        m_enabled = enable;
        std::cout << "[BILGI] Weapon skin yoneticisi " << (enable ? "etkinlestirildi" : "devre disi birakildi") << std::endl;
    }

    bool WeaponSkinManager::IsEnabled() const {
        return m_enabled;
    }

    SkinInfo* WeaponSkinManager::GetSkinInfoByWeaponID(int weaponID) {
        auto it = m_skins.find(weaponID);
        if (it != m_skins.end()) {
            return &it->second;
        }
        return nullptr;
    }

} // namespace geezy_digital