#ifndef _GEEZY_DIGITAL_FEATURES_HPP_
#define _GEEZY_DIGITAL_FEATURES_HPP_

#include "memory/memory.hpp"
#include "config.hpp"
#include <string>
#include <vector>
#include <DirectXMath.h>

// 3D matematik i�in vekt�r yap�lar�
struct GD_Vector3 {
    float x, y, z;

    GD_Vector3() : x(0), y(0), z(0) {}
    GD_Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operat�rler
    GD_Vector3 operator+(const GD_Vector3& other) const { return GD_Vector3(x + other.x, y + other.y, z + other.z); }
    GD_Vector3 operator-(const GD_Vector3& other) const { return GD_Vector3(x - other.x, y - other.y, z - other.z); }
    GD_Vector3 operator*(float scalar) const { return GD_Vector3(x * scalar, y * scalar, z * scalar); }

    // Uzunluk hesapla
    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    // �ki nokta aras� mesafe
    static float Distance(const GD_Vector3& a, const GD_Vector3& b) {
        GD_Vector3 diff = a - b;
        return diff.Length();
    }
};

struct GD_Vector2 {
    float x, y;

    GD_Vector2() : x(0), y(0) {}
    GD_Vector2(float x, float y) : x(x), y(y) {}
};

// 4x4 Matrix
struct GD_Matrix4x4 {
    float m[4][4];

    // 3D d�nya koordinatlar�n� 2D ekran koordinatlar�na d�n��t�r
    bool WorldToScreen(const GD_Vector3& pos, GD_Vector2& screen, int screenWidth, int screenHeight) {
        // 4x4 transform
        float w = m[3][0] * pos.x + m[3][1] * pos.y + m[3][2] * pos.z + m[3][3];

        if (w < 0.01f)
            return false;

        float inverseW = 1.0f / w;
        float x = (m[0][0] * pos.x + m[0][1] * pos.y + m[0][2] * pos.z + m[0][3]) * inverseW;
        float y = (m[1][0] * pos.x + m[1][1] * pos.y + m[1][2] * pos.z + m[1][3]) * inverseW;

        // Koordinatlar� [-1, 1] aral���ndan [0, width/height] aral���na d�n��t�r
        screen.x = (screenWidth / 2.0f) * (1.0f + x);
        screen.y = (screenHeight / 2.0f) * (1.0f - y);

        return true;
    }
};

namespace geezy_digital {

    // ESP ile ilgili veri yap�lar�
    struct GD_PlayerInfo {
        GD_Vector3 position;
        GD_Vector3 headPosition;
        GD_Vector2 screenPos;
        GD_Vector2 headScreenPos;
        std::string name;
        int health;
        int team;
        bool isDefusing;
        bool isVisible;
        bool isTeammate;
        float distance;
        std::string weaponName;
    };

    class FeatureManager {
    private:
        ProcessManager& m_processManager;
        ConfigManager& m_configManager;

        // Oyun state verisi
        struct {
            uintptr_t localPlayerAddress = 0;
            uintptr_t entityListAddress = 0;
            int localPlayerTeam = 0;
            int maxPlayers = 64;
            GD_Matrix4x4 viewMatrix;
            int screenWidth = 1920;  // Varsay�lan de�erler
            int screenHeight = 1080;
        } m_gameState;

        // ESP i�in oyuncu verisi
        std::vector<GD_PlayerInfo> m_players;

        // Oyun verisini g�ncelle
        void GD_UpdateGameState();

        // Oyuncular� g�ncelle
        void GD_UpdatePlayers();

        // View matrix'i g�ncelle
        void GD_UpdateViewMatrix();

    public:
        FeatureManager(ProcessManager& processManager, ConfigManager& configManager)
            : m_processManager(processManager), m_configManager(configManager) {}

        // �zellikleri ba�lat
        bool GD_Initialize();

        // G�ncel ekran boyutlar�n� ayarla
        void GD_SetScreenDimensions(int width, int height) {
            m_gameState.screenWidth = width;
            m_gameState.screenHeight = height;
        }

        // ESP �zelli�i
        void GD_RenderESP();

        // Radar hacki
        void GD_UpdateRadar();

        // Glow efekti
        void GD_UpdateGlow();

        // FOV de�i�tirici
        void GD_UpdateFOV();

        // Silah skin de�i�tirici
        void GD_UpdateSkins();

        // Ana g�ncelleme d�ng�s�
        void GD_Update();

        // Getter metodlar�
        const std::vector<GD_PlayerInfo>& GetPlayers() const { return m_players; }
        const GD_Matrix4x4& GetViewMatrix() const { return m_gameState.viewMatrix; }
        bool IsLocalPlayerValid() const { return m_gameState.localPlayerAddress != 0; }
    };

    class ESPRenderer {
    private:
        FeatureManager& m_featureManager;
        ConfigManager& m_configManager;

        // Direct2D/DirectWrite nesneleri burada olacak (implementasyon i�in)

    public:
        ESPRenderer(FeatureManager& featureManager, ConfigManager& configManager)
            : m_featureManager(featureManager), m_configManager(configManager) {}

        // ESP ba�lat
        bool GD_Initialize();

        // ESP �iz
        void GD_Render();

        // 2D box �iz
        void GD_DrawBox(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, const GD_Color& color, float thickness);

        // Sa�l�k �ubu�u �iz
        void GD_DrawHealthBar(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, int health);

        // �sim �iz
        void GD_DrawName(const GD_Vector2& position, const std::string& name, const GD_Color& color);

        // Silah ismi �iz
        void GD_DrawWeaponName(const GD_Vector2& position, const std::string& weaponName, const GD_Color& color);

        // Mesafe �iz
        void GD_DrawDistance(const GD_Vector2& position, float distance, const GD_Color& color);
    };

    class SkinChanger {
    private:
        ProcessManager& m_processManager;
        ConfigManager& m_configManager;

        // Silahlar� takip et
        struct WeaponHandleInfo {
            int itemDefinitionIndex;
            uintptr_t weaponAddress;
        };

        std::vector<WeaponHandleInfo> m_weaponHandles;
        bool m_knifeChanged = false;

        // Silah offsetlerini bul
        void GD_FindWeaponHandles();

        // Silah kaplama bilgisini uygula
        void GD_ApplySkin(uintptr_t weaponAddress, const GD_WeaponSkin& skinInfo);

    public:
        SkinChanger(ProcessManager& processManager, ConfigManager& configManager)
            : m_processManager(processManager), m_configManager(configManager) {}

        // Skin de�i�tirici ba�lat
        bool GD_Initialize();

        // Skinleri g�ncelle
        void GD_Update();

        // Silah IDsinden silah ad�n� al
        static std::string GD_GetWeaponNameById(int weaponId);

        // Silah ID'sine g�re uygun kaplama bilgisini bul
        const GD_WeaponSkin* GD_FindSkinForWeapon(int weaponId);
    };

    class FOVChanger {
    private:
        ProcessManager& m_processManager;
        ConfigManager& m_configManager;

        uintptr_t m_viewAngleAddress = 0;
        float m_originalFOV = 90.0f;
        bool m_initialized = false;

    public:
        FOVChanger(ProcessManager& processManager, ConfigManager& configManager)
            : m_processManager(processManager), m_configManager(configManager) {}

        // FOV de�i�tirici ba�lat
        bool GD_Initialize();

        // FOV de�erini g�ncelle
        void GD_Update();

        // Orijinal FOV de�erine geri d�n
        void GD_RestoreOriginalFOV();
    };

    // T�m �zellikleri y�neten ana s�n�f
    class FeatureController {
    private:
        ProcessManager& m_processManager;
        ConfigManager& m_configManager;

        FeatureManager m_featureManager;
        ESPRenderer m_espRenderer;
        SkinChanger m_skinChanger;
        FOVChanger m_fovChanger;

        bool m_initialized = false;

    public:
        FeatureController(ProcessManager& processManager, ConfigManager& configManager)
            : m_processManager(processManager),
            m_configManager(configManager),
            m_featureManager(processManager, configManager),
            m_espRenderer(m_featureManager, configManager),
            m_skinChanger(processManager, configManager),
            m_fovChanger(processManager, configManager) {}

        // T�m �zellikleri ba�lat
        bool GD_Initialize();

        // Ana g�ncelleme d�ng�s�
        void GD_Update();

        // ESP �izimi
        void GD_Render();

        // Kaynaklar� temizle
        void GD_Shutdown();

        // Getter metodlar�
        FeatureManager& GetFeatureManager() { return m_featureManager; }
        ESPRenderer& GetESPRenderer() { return m_espRenderer; }
        SkinChanger& GetSkinChanger() { return m_skinChanger; }
        FOVChanger& GetFOVChanger() { return m_fovChanger; }
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_FEATURES_HPP_