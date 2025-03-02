#ifndef _GEEZY_DIGITAL_WEAPON_SKINS_HPP_
#define _GEEZY_DIGITAL_WEAPON_SKINS_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include "memory/memory.hpp"
#include "config.hpp"

namespace geezy_digital {

    // Silah kaplama bilgilerini tutan yap�
    struct WeaponSkin {
        int paintKit;        // Silah kaplamalar�n�n ID de�eri
        float wear;          // A��nma de�eri (0.0-1.0)
        int seed;            // Desen tohumu
        std::string name;    // Kaplama ad� (g�r�nt�leme ama�l�)

        WeaponSkin(int paintKit = 0, float wear = 0.0f, int seed = 0, const std::string& name = "Default")
            : paintKit(paintKit), wear(wear), seed(seed), name(name) {}
    };

    // Silah bilgilerini tutan yap�
    struct WeaponInfo {
        int itemDefinitionIndex;     // Silah�n tan�mlama indeksi
        std::string name;            // Silah ad�
        int entityQuality;           // Silah kalitesi (normal, statTrak, vb)
        int fallbackStatTrak;        // StatTrak sayac�
        int accountID;               // Silah�n sahibinin hesap ID'si
        bool customName;             // �zel isim kullan�l�yor mu
        char customNameTag[32];      // �zel isim

        WeaponInfo() : itemDefinitionIndex(0), entityQuality(0),
            fallbackStatTrak(-1), accountID(0), customName(false) {
            memset(customNameTag, 0, sizeof(customNameTag));
        }
    };

    // Silah offsetlerini tutan yap�
    struct WeaponOffsets {

        uint32_t m_hMyWeapons = 0x40;
        uint32_t m_hActiveWeapon = 0x58;

        // �lk �nce weapon services offsetini bilmemiz gerekiyor
        uint32_t m_pWeaponServices = 0x1110; // Bu de�eri bilmiyoruz, bu y�zden �rnek de�er

        // C_EconItemView
        uint32_t m_iItemDefinitionIndex = 0x1BA;
        uint32_t m_iEntityQuality = 0x1BC;
        uint32_t m_iAccountID = 0x1D8;
        uint32_t m_szCustomName = 0x2D0;

        // C_EconEntity
        uint32_t m_nFallbackPaintKit = 0x15F8;
        uint32_t m_nFallbackSeed = 0x15FC;
        uint32_t m_flFallbackWear = 0x1600;
        uint32_t m_nFallbackStatTrak = 0x1604;
        // Belle�e ba�l� olarak bu offsetler g�ncellenebilir
        void UpdateFromConfig(const ConfigManager& configManager) {
            // Burada oyun g�ncellemelerinde offsetleri g�ncellemek i�in 
            // bir mekanizma kurulabilir
        }
    };

    // Pop�ler silah kaplamalar�n�n listesi
    const std::unordered_map<int, WeaponSkin> kPopularSkins = {
        {1, WeaponSkin(1, 0.01f, 1, "Desert Eagle | Blaze")},
        {2, WeaponSkin(2, 0.01f, 1, "AK-47 | Fire Serpent")},
        {3, WeaponSkin(3, 0.01f, 1, "M4A4 | Howl")},
        {4, WeaponSkin(4, 0.01f, 1, "AWP | Dragon Lore")},
        {5, WeaponSkin(5, 0.01f, 1, "Karambit | Fade")},
        {6, WeaponSkin(6, 0.01f, 1, "M9 Bayonet | Crimson Web")},
        {7, WeaponSkin(7, 0.01f, 1, "Butterfly Knife | Case Hardened")},
        {8, WeaponSkin(8, 0.01f, 1, "Glock-18 | Fade")},
        {9, WeaponSkin(9, 0.01f, 1, "USP-S | Kill Confirmed")},
        {10, WeaponSkin(10, 0.01f, 1, "AK-47 | Asiimov")},
        // Daha fazla kaplama eklenebilir
    };

    // Silah kaplamalar�n� y�neten s�n�f
    class WeaponSkinManager {
    private:
        ProcessManager& m_processManager;
        WeaponOffsets m_offsets;

        // Silah envanterindeki silahlar� listelemek i�in kullan�l�r
        std::vector<uintptr_t> GetPlayerWeapons(uintptr_t localPlayerAddress);

    public:
        // Aktif silah entitiesinin adresi
        uintptr_t GetActiveWeaponAddress(uintptr_t localPlayerAddress);
        WeaponSkinManager(ProcessManager& processManager, const ConfigManager& configManager)
            : m_processManager(processManager) {
            m_offsets.UpdateFromConfig(configManager);
        }


        // Aktif silah�n bilgilerini al
        WeaponInfo GetActiveWeaponInfo(uintptr_t localPlayerAddress);

        // Aktif silah�n kaplama bilgilerini al
        WeaponSkin GetActiveWeaponSkin(uintptr_t localPlayerAddress);

        // Aktif silah�n kaplama bilgilerini g�ncelle
        bool UpdateActiveWeaponSkin(uintptr_t localPlayerAddress, const WeaponSkin& skin);

        // Silaha �zel isim ver (weaponAddress parametresi olan versiyon)
        bool SetCustomName(uintptr_t weaponAddress, const std::string& name);

        // Silaha �zel isim ver (localPlayerAddress parametresi olan versiyon)
        bool SetActiveWeaponCustomName(uintptr_t localPlayerAddress, const std::string& name);

        // StatTrak de�erini g�ncelle (weaponAddress parametresi olan versiyon)
        bool UpdateStatTrak(uintptr_t weaponAddress, int kills);

        // StatTrak de�erini g�ncelle (localPlayerAddress parametresi olan versiyon)
        bool UpdateActiveWeaponStatTrak(uintptr_t localPlayerAddress, int kills);

        // Silah�n �zelliklerini konsola yazd�r
        void PrintWeaponInfo(const WeaponInfo& info, const WeaponSkin& skin);

        // Pop�ler kaplamalar� listele
        void ListPopularSkins();

        // Oyuncunun t�m silahlar�n� listele
        void ListPlayerWeapons(uintptr_t localPlayerAddress);
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_WEAPON_SKINS_HPP_