#ifndef _GEEZY_DIGITAL_FEATURES_HPP_
#define _GEEZY_DIGITAL_FEATURES_HPP_

#include "memory/memory.hpp"
#include "config.hpp"
#include <string>
#include <vector>
#include <DirectXMath.h>

// 3D matematik için vektör yapýlarý
struct GD_Vector3 {
    float x, y, z;

    GD_Vector3() : x(0), y(0), z(0) {}
    GD_Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operatörler
    GD_Vector3 operator+(const GD_Vector3& other) const { return GD_Vector3(x + other.x, y + other.y, z + other.z); }
    GD_Vector3 operator-(const GD_Vector3& other) const { return GD_Vector3(x - other.x, y - other.y, z - other.z); }
    GD_Vector3 operator*(float scalar) const { return GD_Vector3(x * scalar, y * scalar, z * scalar); }

    // Uzunluk hesapla
    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    // Ýki nokta arasý mesafe
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

    // 3D dünya koordinatlarýný 2D ekran koordinatlarýna dönüþtür
    bool WorldToScreen(const GD_Vector3& pos, GD_Vector2& screen, int screenWidth, int screenHeight) {
        // 4x4 transform
        float w = m[3][0] * pos.x + m[3][1] * pos.y + m[3][2] * pos.z + m[3][3];

        if (w < 0.01f)
            return false;

        float inverseW = 1.0f / w;
        float x = (m[0][0] * pos.x + m[0][1] * pos.y + m[0][2] * pos.z + m[0][3]) * inverseW;
        float y = (m[1][0] * pos.x + m[1][1] * pos.y + m[1][2] * pos.z + m[1][3]) * inverseW;

        // Koordinatlarý [-1, 1] aralýðýndan [0, width/height] aralýðýna dönüþtür
        screen.x = (screenWidth / 2.0f) * (1.0f + x);
        screen.y = (screenHeight / 2.0f) * (1.0f - y);

        return true;
    }
};

namespace geezy_digital {

    // ESP ile ilgili veri yapýlarý
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
            int screenWidth = 1920;  // Varsayýlan deðerler
            int screenHeight = 1080;
        } m_gameState;

        // ESP için oyuncu verisi
        std::vector<GD_PlayerInfo> m_players;

        // Oyun verisini güncelle
        void GD_UpdateGameState();

        // Oyuncularý güncelle
        void GD_UpdatePlayers();

        // View matrix'i güncelle
        void GD_UpdateViewMatrix();

    public:
        FeatureManager(ProcessManager& processManager, ConfigManager& configManager)
            : m_processManager(processManager), m_configManager(configManager) {}

        // Özellikleri baþlat
        bool GD_Initialize();

        // Güncel ekran boyutlarýný ayarla
        void GD_SetScreenDimensions(int width, int height) {
            m_gameState.screenWidth = width;
            m_gameState.screenHeight = height;
        }

        // ESP özelliði
        void GD_RenderESP();

        // Radar hacki
        void GD_UpdateRadar();

        // Glow efekti
        void GD_UpdateGlow();

        // FOV deðiþtirici
        void GD_UpdateFOV();

        // Silah skin deðiþtirici
        void GD_UpdateSkins();

        // Ana güncelleme döngüsü
        void GD_Update();

        // Getter metodlarý
        const std::vector<GD_PlayerInfo>& GetPlayers() const { return m_players; }
        const GD_Matrix4x4& GetViewMatrix() const { return m_gameState.viewMatrix; }
        bool IsLocalPlayerValid() const { return m_gameState.localPlayerAddress != 0; }
    };

    class ESPRenderer {
    private:
        FeatureManager& m_featureManager;
        ConfigManager& m_configManager;

        // Direct2D/DirectWrite nesneleri burada olacak (implementasyon için)

    public:
        ESPRenderer(FeatureManager& featureManager, ConfigManager& configManager)
            : m_featureManager(featureManager), m_configManager(configManager) {}

        // ESP baþlat
        bool GD_Initialize();

        // ESP çiz
        void GD_Render();

        // 2D box çiz
        void GD_DrawBox(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, const GD_Color& color, float thickness);

        // Saðlýk çubuðu çiz
        void GD_DrawHealthBar(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, int health);

        // Ýsim çiz
        void GD_DrawName(const GD_Vector2& position, const std::string& name, const GD_Color& color);

        // Silah ismi çiz
        void GD_DrawWeaponName(const GD_Vector2& position, const std::string& weaponName, const GD_Color& color);

        // Mesafe çiz
        void GD_DrawDistance(const GD_Vector2& position, float distance, const GD_Color& color);
    };

    class SkinChanger {
    private:
        ProcessManager& m_processManager;
        ConfigManager& m_configManager;

        // Silahlarý takip et
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

        // Skin deðiþtirici baþlat
        bool GD_Initialize();

        // Skinleri güncelle
        void GD_Update();

        // Silah IDsinden silah adýný al
        static std::string GD_GetWeaponNameById(int weaponId);

        // Silah ID'sine göre uygun kaplama bilgisini bul
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

        // FOV deðiþtirici baþlat
        bool GD_Initialize();

        // FOV deðerini güncelle
        void GD_Update();

        // Orijinal FOV deðerine geri dön
        void GD_RestoreOriginalFOV();
    };

    // Tüm özellikleri yöneten ana sýnýf
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

        // Tüm özellikleri baþlat
        bool GD_Initialize();

        // Ana güncelleme döngüsü
        void GD_Update();

        // ESP çizimi
        void GD_Render();

        // Kaynaklarý temizle
        void GD_Shutdown();

        // Getter metodlarý
        FeatureManager& GetFeatureManager() { return m_featureManager; }
        ESPRenderer& GetESPRenderer() { return m_espRenderer; }
        SkinChanger& GetSkinChanger() { return m_skinChanger; }
        FOVChanger& GetFOVChanger() { return m_fovChanger; }
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_FEATURES_HPP_