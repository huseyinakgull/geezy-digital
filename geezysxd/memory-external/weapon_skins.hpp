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

        // Konfigurasyon dosyasýndan offset deðerlerini güncelle
        void UpdateFromConfig(const Config::ConfigManager& configManager);
    };

    // Silah Skin Yöneticisi
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

        // Skinleri konfigürasyondan yükle
        bool LoadSkins();

        // Skinleri konfigürasyona kaydet
        bool SaveSkins();

        // Özel skin ekle/güncelle
        void SetSkin(int weaponID, int skinID, float wear = 0.01f, int seed = 0, int statTrak = -1);

        // Skinleri uygula
        void ApplySkins();

        // Bellek tarama ve offsetlarý bulma
        bool ScanForOffsets();

        // Yöneticiyi etkinleþtir/devre dýþý býrak
        void Enable(bool enable);

        // Etkinlik durumunu al
        bool IsEnabled() const;
    };

} // namespace geezy_digital