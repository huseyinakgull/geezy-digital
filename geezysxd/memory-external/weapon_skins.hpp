#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "../memory-external/memory/memory.hpp"
#include "config.hpp"

namespace geezy_digital {

    // Silah Skin Bilgisi
    struct SkinInfo {
        int id;
        std::string name;
        float wear;
        int seed;
        int statTrak;
        bool enabled;
    };

    // Offsetlar
    struct WeaponOffsets {
        uintptr_t itemDefinitionIndex;
        uintptr_t fallbackPaintKit;
        uintptr_t fallbackWear;
        uintptr_t fallbackSeed;
        uintptr_t itemIDHigh;
        uintptr_t accountID;
        uintptr_t fallbackStatTrak;
        uintptr_t entityQuality;

        // Konfigurasyon dosyas�ndan offset de�erlerini g�ncelle
        void UpdateFromConfig(const Config::ConfigManager& configManager);
    };

    // Silah Skin Y�neticisi
    class WeaponSkinManager {
    private:
        Memory::MemoryManager& m_memoryManager;
        Config::ConfigManager& m_configManager;
        WeaponOffsets m_offsets;
        std::unordered_map<int, SkinInfo> m_skins;
        bool m_enabled;

        // Silah ID'sinden skin bilgisi al
        SkinInfo* GetSkinInfoByWeaponID(int weaponID);

    public:
        WeaponSkinManager(Memory::MemoryManager& memoryManager, Config::ConfigManager& configManager);
        ~WeaponSkinManager() = default;

        // Skinleri konfig�rasyondan y�kle
        bool LoadSkins();

        // Skinleri konfig�rasyona kaydet
        bool SaveSkins();

        // �zel skin ekle/g�ncelle
        void SetSkin(int weaponID, int skinID, float wear = 0.01f, int seed = 0, int statTrak = -1);

        // Skinleri uygula
        void ApplySkins();

        // Bellek tarama ve offsetlar� bulma
        bool ScanForOffsets();

        // Y�neticiyi etkinle�tir/devre d��� b�rak
        void Enable(bool enable);

        // Etkinlik durumunu al
        bool IsEnabled() const;
    };

} // namespace geezy_digital