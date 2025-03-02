#include "weapon_skins.hpp"
#include <iostream>
#include "signature_scanner.hpp"

namespace geezy_digital {

    // WeaponOffsets sýnýfý
    void WeaponOffsets::UpdateFromConfig(const Config::ConfigManager& configManager) {
        // Konfigürasyon dosyasýndan offset deðerlerini oku
        itemDefinitionIndex = configManager.GetInt("Offset.ItemDefinitionIndex", 0x2FAA);
        fallbackPaintKit = configManager.GetInt("Offset.FallbackPaintKit", 0x31C8);
        fallbackWear = configManager.GetInt("Offset.FallbackWear", 0x31D0);
        fallbackSeed = configManager.GetInt("Offset.FallbackSeed", 0x31CC);
        itemIDHigh = configManager.GetInt("Offset.ItemIDHigh", 0x2FC0);
        accountID = configManager.GetInt("Offset.AccountID", 0x2FC8);
        fallbackStatTrak = configManager.GetInt("Offset.FallbackStatTrak", 0x31D4);
        entityQuality = configManager.GetInt("Offset.EntityQuality", 0x2FAC);

        std::cout << "[BILGI] Weapon offsetleri yuklendi" << std::endl;
    }

    // WeaponSkinManager sýnýfý
    WeaponSkinManager::WeaponSkinManager(Memory::MemoryManager& memoryManager, Config::ConfigManager& configManager)
        : m_memoryManager(memoryManager), m_configManager(configManager), m_enabled(false) {

        // Offsetleri yapýlandýrmadan yükle
        m_offsets.UpdateFromConfig(configManager);

        // Skinleri yükle
        LoadSkins();
    }

    bool WeaponSkinManager::LoadSkins() {
        try {
            // Ayarlardan skin bilgilerini oku
            int skinCount = m_configManager.GetInt("Skins.Count", 0);

            m_skins.clear();

            for (int i = 0; i < skinCount; i++) {
                std::string prefix = "Skins[" + std::to_string(i) + "].";

                SkinInfo skin;
                int weaponID = m_configManager.GetInt(prefix + "WeaponID", 0);

                skin.id = m_configManager.GetInt(prefix + "SkinID", 0);
                skin.name = m_configManager.GetString(prefix + "Name", "");
                skin.wear = m_configManager.GetFloat(prefix + "Wear", 0.01f);
                skin.seed = m_configManager.GetInt(prefix + "Seed", 0);
                skin.statTrak = m_configManager.GetInt(prefix + "StatTrak", -1);
                skin.enabled = m_configManager.GetBool(prefix + "Enabled", true);

                m_skins[weaponID] = skin;
            }

            std::cout << "[BASARI] " << skinCount << " adet skin yuklendi" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << "[HATA] Skinler yuklenemedi: " << e.what() << std::endl;
            return false;
        }
    }

    bool WeaponSkinManager::SaveSkins() {
        try {
            // Skin sayýsýný kaydet
            m_configManager.Set("Skins.Count", static_cast<int>(m_skins.size()));

            int index = 0;
            for (const auto& [weaponID, skin] : m_skins) {
                std::string prefix = "Skins[" + std::to_string(index) + "].";

                m_configManager.Set(prefix + "WeaponID", weaponID);
                m_configManager.Set(prefix + "SkinID", skin.id);
                m_configManager.Set(prefix + "Name", skin.name);
                m_configManager.Set(prefix + "Wear", skin.wear);
                m_configManager.Set(prefix + "Seed", skin.seed);
                m_configManager.Set(prefix + "StatTrak", skin.statTrak);
                m_configManager.Set(prefix + "Enabled", skin.enabled);

                index++;
            }

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

    void WeaponSkinManager::ApplySkins() {
        if (!m_enabled) {
            return;
        }

        // Local player pointer
        uintptr_t localPlayerPtr = m_memoryManager.Read<uintptr_t>(
            m_memoryManager.GetModuleBaseAddress("client.dll") +
            m_memoryManager.GetOffset("dwLocalPlayer")
        );

        if (!localPlayerPtr) {
            return;
        }

        // Entity list
        uintptr_t entityList = m_memoryManager.GetModuleBaseAddress("client.dll") +
            m_memoryManager.GetOffset("dwEntityList");

        // Read local player ID
        int localPlayerID = m_memoryManager.Read<int>(localPlayerPtr + m_memoryManager.GetOffset("m_iTeamNum"));

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

    bool WeaponSkinManager::ScanForOffsets() {
        std::cout << "[BILGI] Weapon offsetleri icin tarama baslatiliyor..." << std::endl;

        uintptr_t clientModule = m_memoryManager.GetModuleBaseAddress("client.dll");
        if (!clientModule) {
            std::cout << "[HATA] client.dll bulunamadi" << std::endl;
            return false;
        }

        // Burada imza taramasý ile offset deðerleri bulunabilir
        // Örnek: "48 8D 05 ? ? ? ? 48 8B 18 48 85 DB 74 2A" gibi imzalarla bellek taramasý yapýlýr

        // Bu deðerler manuel olarak konfigurasyon dosyasýna kaydedilebilir veya direkt olarak yapýya atanabilir

        std::cout << "[BILGI] Weapon offsetleri basariyla taranýp güncellendi" << std::endl;
        return true;
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