#include "weapon_skins.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>

// Gerekli diðer header'larý ekle
#include "memory/memory.hpp"
#include "config.hpp"

namespace geezy_digital {

    // Varsayýlan constructor
    WeaponSkinManager::WeaponSkinManager(ProcessManager& processManager, const ConfigManager& configManager)
        : m_processManager(processManager) {
        // Offsetleri konfigürasyondan güncelle
        m_offsets.UpdateFromConfig(configManager);
    }

    // Oyuncunun aktif silahýnýn adresini alýr
    uintptr_t WeaponSkinManager::GetActiveWeaponAddress(uintptr_t localPlayerAddress) {
        if (!localPlayerAddress) return 0;

        try {
            // Aktif silah handle'ýný oku
            uintptr_t activeWeaponHandle = m_processManager.GD_Read<uintptr_t>(localPlayerAddress + m_offsets.m_hActiveWeapon) & 0xFFF;

            // Entity listesinden silah adresini al
            uintptr_t entityList = m_processManager.GetBaseModule().base + m_processManager.GetOffsets().dwEntityList;
            return m_processManager.GD_Read<uintptr_t>(entityList + ((activeWeaponHandle - 1) * 0x8));
        }
        catch (const std::exception& e) {
            std::cerr << "Aktif silah adresi alýnýrken hata: " << e.what() << std::endl;
            return 0;
        }
    }

    // Oyuncunun tüm silahlarýný listeler
    std::vector<uintptr_t> WeaponSkinManager::GetPlayerWeapons(uintptr_t localPlayerAddress) {
        std::vector<uintptr_t> weapons;
        if (!localPlayerAddress) return weapons;

        try {
            // Entity listesi adresini al
            uintptr_t entityList = m_processManager.GetBaseModule().base + m_processManager.GetOffsets().dwEntityList;

            // Silah envanterini tara (CS2'de 8 slot)
            for (int i = 0; i < 8; i++) {
                uintptr_t weaponHandle = m_processManager.GD_Read<uintptr_t>(localPlayerAddress + m_offsets.m_hMyWeapons + (i * 0x4)) & 0xFFF;
                if (weaponHandle == 0xFFF) continue;

                uintptr_t weaponAddress = m_processManager.GD_Read<uintptr_t>(entityList + ((weaponHandle - 1) * 0x8));
                if (weaponAddress) {
                    weapons.push_back(weaponAddress);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Oyuncu silahlarý listelenirken hata: " << e.what() << std::endl;
        }

        return weapons;
    }

    // Aktif silahýn bilgilerini alýr
    WeaponInfo WeaponSkinManager::GetActiveWeaponInfo(uintptr_t localPlayerAddress) {
        WeaponInfo info;
        uintptr_t weaponAddress = GetActiveWeaponAddress(localPlayerAddress);

        if (!weaponAddress) return info;

        try {
            // Silah bilgilerini oku
            info.itemDefinitionIndex = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_iItemDefinitionIndex);
            info.entityQuality = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_iEntityQuality);
            info.fallbackStatTrak = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_nFallbackStatTrak);
            info.accountID = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_iAccountID);

            // Silah adýný belirle (burada daha kapsamlý bir enum dönüþümü yapýlabilir)
            switch (info.itemDefinitionIndex) {
            case 7: info.name = "AK-47"; break;
            case 9: info.name = "AWP"; break;
            case 16: info.name = "M4A4"; break;
            case 60: info.name = "M4A1-S"; break;
            case 1: info.name = "Desert Eagle"; break;
            case 4: info.name = "Glock-18"; break;
            case 61: info.name = "USP-S"; break;
            case 508: info.name = "Karambit"; break;
            case 507: info.name = "Butterfly Knife"; break;
                // Daha fazla silah eklenebilir
            default: info.name = "Unknown Weapon (" + std::to_string(info.itemDefinitionIndex) + ")";
            }

            // Özel ismi oku
            char customName[32];
            if (m_processManager.GD_ReadRaw(weaponAddress + m_offsets.m_szCustomName, customName, sizeof(customName))) {
                if (customName[0] != '\0') {
                    info.customName = true;
                    memcpy(info.customNameTag, customName, sizeof(customName));
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Aktif silah bilgileri alýnýrken hata: " << e.what() << std::endl;
        }

        return info;
    }

    // Aktif silahýn kaplama bilgilerini alýr
    WeaponSkin WeaponSkinManager::GetActiveWeaponSkin(uintptr_t localPlayerAddress) {
        WeaponSkin skin;
        uintptr_t weaponAddress = GetActiveWeaponAddress(localPlayerAddress);

        if (!weaponAddress) return skin;

        try {
            // Kaplama bilgilerini oku
            skin.paintKit = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_nFallbackPaintKit);
            skin.wear = m_processManager.GD_Read<float>(weaponAddress + m_offsets.m_flFallbackWear);
            skin.seed = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_nFallbackSeed);

            // Kaplama adýný belirle (gerçek bir implementasyonda daha kapsamlý bir veritabaný olmalý)
            switch (skin.paintKit) {
            case 0: skin.name = "Default"; break;
            case 1: skin.name = "Blaze"; break;
            case 2: skin.name = "Fire Serpent"; break;
            case 3: skin.name = "Howl"; break;
            case 4: skin.name = "Dragon Lore"; break;
            case 5: skin.name = "Fade"; break;
            case 6: skin.name = "Crimson Web"; break;
            case 7: skin.name = "Case Hardened"; break;
            case 8: skin.name = "Hyper Beast"; break;
            case 9: skin.name = "Kill Confirmed"; break;
            case 10: skin.name = "Asiimov"; break;
                // Daha fazla kaplama eklenebilir
            default: skin.name = "Unknown Skin (" + std::to_string(skin.paintKit) + ")";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Aktif silah kaplama bilgileri alýnýrken hata: " << e.what() << std::endl;
        }

        return skin;
    }

    // Aktif silahýn kaplama deðerlerini günceller
    bool WeaponSkinManager::UpdateActiveWeaponSkin(uintptr_t localPlayerAddress, const WeaponSkin& skin) {
        uintptr_t weaponAddress = GetActiveWeaponAddress(localPlayerAddress);

        if (!weaponAddress) return false;

        try {
            // Kaplama deðerlerini güncelle
            bool success = true;
            success &= m_processManager.GD_Write(weaponAddress + m_offsets.m_nFallbackPaintKit, skin.paintKit);
            success &= m_processManager.GD_Write(weaponAddress + m_offsets.m_flFallbackWear, skin.wear);
            success &= m_processManager.GD_Write(weaponAddress + m_offsets.m_nFallbackSeed, skin.seed);

            return success;
        }
        catch (const std::exception& e) {
            std::cerr << "Silah kaplamasý güncellenirken hata: " << e.what() << std::endl;
            return false;
        }
    }

    // Silaha özel isim verir (weaponAddress versiyonu)
    bool WeaponSkinManager::SetCustomName(uintptr_t weaponAddress, const std::string& name) {
        if (!weaponAddress) return false;

        try {
            char customName[32];
            memset(customName, 0, sizeof(customName));

            // Ýsmi kopyala (32 karakterle sýnýrla)
            strncpy_s(customName, name.c_str(), sizeof(customName) - 1);

            // Ýsmi belleðe yaz
            return m_processManager.GD_WriteBytes(weaponAddress + m_offsets.m_szCustomName,
                std::vector<uint8_t>(customName, customName + sizeof(customName)));
        }
        catch (const std::exception& e) {
            std::cerr << "Silaha özel isim verilirken hata: " << e.what() << std::endl;
            return false;
        }
    }

    // Silaha özel isim verir (localPlayerAddress versiyonu)
    bool WeaponSkinManager::SetActiveWeaponCustomName(uintptr_t localPlayerAddress, const std::string& name) {
        uintptr_t weaponAddress = GetActiveWeaponAddress(localPlayerAddress);
        if (!weaponAddress) return false;

        return SetCustomName(weaponAddress, name);
    }

    // StatTrak deðerini günceller (weaponAddress versiyonu)
    bool WeaponSkinManager::UpdateStatTrak(uintptr_t weaponAddress, int kills) {
        if (!weaponAddress) return false;

        try {
            // StatTrak durumuna göre kalite ayarla (StatTrak = 9)
            bool success = m_processManager.GD_Write(weaponAddress + m_offsets.m_iEntityQuality, 9);

            // Kill sayýsýný güncelle
            success &= m_processManager.GD_Write(weaponAddress + m_offsets.m_nFallbackStatTrak, kills);

            return success;
        }
        catch (const std::exception& e) {
            std::cerr << "StatTrak güncellenirken hata: " << e.what() << std::endl;
            return false;
        }
    }

    // StatTrak deðerini günceller (localPlayerAddress versiyonu)
    bool WeaponSkinManager::UpdateActiveWeaponStatTrak(uintptr_t localPlayerAddress, int kills) {
        uintptr_t weaponAddress = GetActiveWeaponAddress(localPlayerAddress);
        if (!weaponAddress) return false;

        return UpdateStatTrak(weaponAddress, kills);
    }

    // Silah bilgilerini konsola yazdýrýr
    void WeaponSkinManager::PrintWeaponInfo(const WeaponInfo& info, const WeaponSkin& skin) {
        std::cout << "==================================================" << std::endl;
        std::cout << "Aktif Silah Bilgileri:" << std::endl;
        std::cout << "==================================================" << std::endl;
        std::cout << "Silah: " << info.name << " (ID: " << info.itemDefinitionIndex << ")" << std::endl;
        std::cout << "Kaplama: " << skin.name << " (ID: " << skin.paintKit << ")" << std::endl;
        std::cout << "Aþýnma Deðeri: " << std::fixed << std::setprecision(6) << skin.wear << std::endl;
        std::cout << "Desen Tohumu: " << skin.seed << std::endl;

        if (info.fallbackStatTrak >= 0) {
            std::cout << "StatTrak™: " << info.fallbackStatTrak << " kills" << std::endl;
        }

        if (info.customName) {
            std::cout << "Özel Ýsim: " << info.customNameTag << std::endl;
        }

        std::cout << "==================================================" << std::endl;
    }

    // Popüler kaplamalarý listeler
    void WeaponSkinManager::ListPopularSkins() {
        std::cout << "==================================================" << std::endl;
        std::cout << "Popüler Kaplamalar:" << std::endl;
        std::cout << "==================================================" << std::endl;

        for (const auto& pair : kPopularSkins) {
            const WeaponSkin& skin = pair.second;
            std::cout << pair.first << ". " << skin.name << " (ID: " << skin.paintKit << ", Wear: "
                << std::fixed << std::setprecision(2) << skin.wear << ")" << std::endl;
        }

        std::cout << "==================================================" << std::endl;
    }

    // Oyuncunun tüm silahlarýný listeler
    void WeaponSkinManager::ListPlayerWeapons(uintptr_t localPlayerAddress) {
        std::vector<uintptr_t> weapons = GetPlayerWeapons(localPlayerAddress);

        std::cout << "==================================================" << std::endl;
        std::cout << "Envanterdeki Silahlar (" << weapons.size() << "):" << std::endl;
        std::cout << "==================================================" << std::endl;

        for (size_t i = 0; i < weapons.size(); i++) {
            uintptr_t weaponAddress = weapons[i];

            try {
                // Silah bilgilerini oku
                int itemId = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_iItemDefinitionIndex);
                int paintKit = m_processManager.GD_Read<int>(weaponAddress + m_offsets.m_nFallbackPaintKit);

                // Silah adýný belirle
                std::string weaponName;
                switch (itemId) {
                case 7: weaponName = "AK-47"; break;
                case 9: weaponName = "AWP"; break;
