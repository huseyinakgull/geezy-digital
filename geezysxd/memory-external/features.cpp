#include "features.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace geezy_digital {

    //===========================================
    // FeatureManager Implementasyonu
    //===========================================

    bool FeatureManager::GD_Initialize() {
        // Oyun durumunu ilk kez g�ncelle
        GD_UpdateGameState();
        return m_gameState.localPlayerAddress != 0;
    }

    void FeatureManager::GD_Update() {
        // Oyun durumunu ve oyuncu listesini g�ncelle
        GD_UpdateGameState();
        GD_UpdatePlayers();

        // Yap�land�rma de�erlerini al
        auto& espConfig = m_configManager.GetESPConfig();
        auto& glowConfig = m_configManager.GetGlowConfig();
        auto& radarConfig = m_configManager.GetRadarConfig();
        auto& fovConfig = m_configManager.GetFOVConfig();
        auto& skinChangerConfig = m_configManager.GetSkinChangerConfig();

        // �zellikleri aktif/pasif durumlar�na g�re g�ncelle
        if (radarConfig.enabled) GD_UpdateRadar();
        if (glowConfig.enabled) GD_UpdateGlow();
        if (fovConfig.enabled) GD_UpdateFOV();
        if (skinChangerConfig.enabled) GD_UpdateSkins();

        // ESP'yi �zel olarak g�ncellemeye gerek yok, render zaman�nda yap�lacak
    }

    void FeatureManager::GD_UpdateGameState() {
        auto& offsets = m_configManager.GetOffsets();
        const auto baseAddress = m_processManager.GetBaseModule().base;

        // Gerekli bellek adreslerini al
        m_gameState.entityListAddress = m_processManager.GD_Read<uintptr_t>(baseAddress + offsets.dwEntityList);
        m_gameState.localPlayerAddress = m_processManager.GD_Read<uintptr_t>(baseAddress + offsets.dwLocalPlayer);

        // Yerel oyuncu bilgisini al
        if (m_gameState.localPlayerAddress) {
            uintptr_t localPlayerPawn = m_processManager.GD_Read<uintptr_t>(m_gameState.localPlayerAddress + offsets.m_hPlayerPawn);
            if (localPlayerPawn) {
                m_gameState.localPlayerTeam = m_processManager.GD_Read<int>(localPlayerPawn + offsets.m_iTeamNum);
            }
        }

        // View matrix g�ncelle
        GD_UpdateViewMatrix();
    }

    void FeatureManager::GD_UpdateViewMatrix() {
        auto& offsets = m_configManager.GetOffsets();
        const auto baseAddress = m_processManager.GetBaseModule().base;

        // View matrix adresini bul ve oku
        uintptr_t viewMatrixAddr = baseAddress + offsets.dwViewMatrix;

        // 4x4 matris oku
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m_gameState.viewMatrix.m[i][j] = m_processManager.GD_Read<float>(viewMatrixAddr + (i * 4 + j) * sizeof(float));
            }
        }
    }

    void FeatureManager::GD_UpdatePlayers() {
        auto& offsets = m_configManager.GetOffsets();
        const auto baseAddress = m_processManager.GetBaseModule().base;

        // Oyuncu listesini temizle
        m_players.clear();

        if (!m_gameState.entityListAddress || !m_gameState.localPlayerAddress) {
            return;
        }

        // Yerel oyuncunun konumunu al
        uintptr_t localPlayerPawn = m_processManager.GD_Read<uintptr_t>(m_gameState.localPlayerAddress + offsets.m_hPlayerPawn);
        GD_Vector3 localPlayerPos = { 0, 0, 0 };

        if (localPlayerPawn) {
            localPlayerPos.x = m_processManager.GD_Read<float>(localPlayerPawn + offsets.m_vecAbsOrigin);
            localPlayerPos.y = m_processManager.GD_Read<float>(localPlayerPawn + offsets.m_vecAbsOrigin + 4);
            localPlayerPos.z = m_processManager.GD_Read<float>(localPlayerPawn + offsets.m_vecAbsOrigin + 8);
        }

        // Oyuncu listesini tara
        for (int i = 0; i < m_gameState.maxPlayers; i++) {
            uintptr_t entityList = m_processManager.GD_Read<uintptr_t>(m_gameState.entityListAddress + 0x10);
            if (!entityList) continue;

            uintptr_t entity = m_processManager.GD_Read<uintptr_t>(entityList + (i + 1) * 0x78);
            if (!entity) continue;

            uintptr_t playerPawn = m_processManager.GD_Read<uintptr_t>(entity + offsets.m_hPlayerPawn);
            if (!playerPawn || playerPawn == localPlayerPawn) continue;

            int health = m_processManager.GD_Read<int>(playerPawn + offsets.m_iHealth);
            if (health <= 0 || health > 100) continue;  // Ge�ersiz sa�l�k de�eri

            int team = m_processManager.GD_Read<int>(playerPawn + offsets.m_iTeamNum);
            bool isDefusing = m_processManager.GD_Read<bool>(playerPawn + offsets.m_bIsDefusing);

            // Oyuncu konum bilgisi
            GD_Vector3 position;
            position.x = m_processManager.GD_Read<float>(playerPawn + offsets.m_vecAbsOrigin);
            position.y = m_processManager.GD_Read<float>(playerPawn + offsets.m_vecAbsOrigin + 4);
            position.z = m_processManager.GD_Read<float>(playerPawn + offsets.m_vecAbsOrigin + 8);

            // Oyuncu ba� konumu (z ekseninde 64 birim yukar�)
            GD_Vector3 headPosition = position;
            headPosition.z += 64.0f;  // Yakla��k ba� y�ksekli�i

            // D�nya koordinatlar�n� ekran koordinatlar�na d�n��t�r
            GD_Vector2 screenPos, headScreenPos;
            bool isVisible = m_gameState.viewMatrix.WorldToScreen(position, screenPos, m_gameState.screenWidth, m_gameState.screenHeight);
            bool isHeadVisible = m_gameState.viewMatrix.WorldToScreen(headPosition, headScreenPos, m_gameState.screenWidth, m_gameState.screenHeight);

            // Oyuncu SADECE ekranda g�r�lebiliyorsa listeye ekle
            if (isVisible || isHeadVisible) {
                // Oyuncu ad�n� oku
                char nameBuffer[32];
                uintptr_t nameAddress = m_processManager.GD_Read<uintptr_t>(entity + offsets.m_sSanitizedPlayerName);
                m_processManager.GD_ReadRaw(m_processManager.GetProcessHandle(), nameAddress, nameBuffer, sizeof(nameBuffer));
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';  // Null-terminate

                // Oyuncu bilgisini doldur
                GD_PlayerInfo playerInfo;
                playerInfo.position = position;
                playerInfo.headPosition = headPosition;
                playerInfo.screenPos = screenPos;
                playerInfo.headScreenPos = headScreenPos;
                playerInfo.name = nameBuffer;
                playerInfo.health = health;
                playerInfo.team = team;
                playerInfo.isDefusing = isDefusing;
                playerInfo.isVisible = isVisible && isHeadVisible;
                playerInfo.isTeammate = (team == m_gameState.localPlayerTeam);
                playerInfo.distance = GD_Vector3::Distance(localPlayerPos, position);

                // Silah bilgisi
                uintptr_t weaponHandle = m_processManager.GD_Read<uintptr_t>(playerPawn + offsets.m_pClippingWeapon);
                if (weaponHandle) {
                    uintptr_t weaponNameAddr = m_processManager.GD_Read<uintptr_t>(weaponHandle + offsets.m_szName);
                    if (weaponNameAddr) {
                        char weaponNameBuffer[32];
                        m_processManager.GD_ReadRaw(m_processManager.GetProcessHandle(), weaponNameAddr, weaponNameBuffer, sizeof(weaponNameBuffer));
                        weaponNameBuffer[sizeof(weaponNameBuffer) - 1] = '\0';  // Null-terminate
                        playerInfo.weaponName = weaponNameBuffer;
                    }
                }

                // Listeye ekle
                m_players.push_back(playerInfo);
            }
        }

        // Oyuncular� mesafeye g�re s�rala (en yak�ndan en uza�a)
        std::sort(m_players.begin(), m_players.end(), [](const GD_PlayerInfo& a, const GD_PlayerInfo& b) {
            return a.distance < b.distance;
            });
    }

    void FeatureManager::GD_RenderESP() {
        // ESP rendering i�lemi, ESPRenderer s�n�f�nda yap�lacak
    }

    void FeatureManager::GD_UpdateRadar() {
        auto& offsets = m_configManager.GetOffsets();
        auto& radarConfig = m_configManager.GetRadarConfig();

        // Radar hacki: Oyuncular� radar �zerinde g�sterme
        // Bu �zellik m_bSpotted de�erini true yaparak �al���r
        for (auto& entity : m_players) {
            if (entity.isTeammate && !radarConfig.showTeammates) continue;
            if (!entity.isTeammate && !radarConfig.showEnemies) continue;

            // EntityList'ten oyuncu adresini al
            uintptr_t entityList = m_processManager.GD_Read<uintptr_t>(m_gameState.entityListAddress + 0x10);
            if (!entityList) continue;

            // Oyuncu ad�n� kar��la�t�rarak do�ru varl��� bul
            for (int i = 0; i < m_gameState.maxPlayers; i++) {
                uintptr_t entityAddr = m_processManager.GD_Read<uintptr_t>(entityList + (i + 1) * 0x78);
                if (!entityAddr) continue;

                uintptr_t playerPawn = m_processManager.GD_Read<uintptr_t>(entityAddr + offsets.m_hPlayerPawn);
                if (!playerPawn) continue;

                // Spotted de�erini g�ncelle
                m_processManager.GD_Write(playerPawn + offsets.m_bSpotted, true);
            }
        }

        // Bomba g�sterme (e�er aktifse)
        if (radarConfig.showBomb) {
            uintptr_t bombAddr = m_processManager.GD_Read<uintptr_t>(m_processManager.GetBaseModule().base + offsets.dwPlantedC4);
            if (bombAddr) {
                m_processManager.GD_Write(bombAddr + offsets.m_bSpotted, true);
            }
        }
    }

    void FeatureManager::GD_UpdateGlow() {
        auto& offsets = m_configManager.GetOffsets();
        auto& glowConfig = m_configManager.GetGlowConfig();

        // Glow manager adresini al
        uintptr_t glowObjectManager = m_processManager.GD_Read<uintptr_t>(m_processManager.GetBaseModule().base + offsets.dwGlowManager);
        if (!glowObjectManager) return;

        // Her oyuncu i�in glow efekti uygula
        for (auto& entity : m_players) {
            if (entity.isTeammate && !glowConfig.showTeammates) continue;
            if (!entity.isTeammate && !glowConfig.showEnemies) continue;

            // EntityList'ten oyuncu adresini al
            uintptr_t entityList = m_processManager.GD_Read<uintptr_t>(m_gameState.entityListAddress + 0x10);
            if (!entityList) continue;

            // Oyuncu ad�n� kar��la�t�rarak do�ru varl��� bul
            for (int i = 0; i < m_gameState.maxPlayers; i++) {
                uintptr_t entityAddr = m_processManager.GD_Read<uintptr_t>(entityList + (i + 1) * 0x78);
                if (!entityAddr) continue;

                uintptr_t playerPawn = m_processManager.GD_Read<uintptr_t>(entityAddr + offsets.m_hPlayerPawn);
                if (!playerPawn) continue;

                // Glow indeksi al
                int glowIndex = m_processManager.GD_Read<int>(playerPawn + offsets.m_iGlowIndex);
                if (glowIndex < 0) continue;

                // Glow rengini belirle
                GD_Color glowColor;
                if (entity.isDefusing) {
                    glowColor = glowConfig.enemyColor;  // Defuse yapan oyuncu rengi
                }
                else if (entity.isTeammate) {
                    glowColor = glowConfig.teammateColor;  // Tak�m arkada�� rengi
                }
                else {
                    glowColor = glowConfig.enemyColor;  // D��man rengi
                }

                // Glow efekti yap�s� (CS2'ye g�re)
                // Not: Ger�ek offset'ler ve yap�, oyun versiyonuna g�re de�i�ebilir
                uintptr_t glowAddress = glowObjectManager + (glowIndex * 0x38);

                // Glow rengini yaz
                m_processManager.GD_Write(glowAddress + 0x8, glowColor.r / 255.0f);   // R
                m_processManager.GD_Write(glowAddress + 0xC, glowColor.g / 255.0f);   // G
                m_processManager.GD_Write(glowAddress + 0x10, glowColor.b / 255.0f);  // B
                m_processManager.GD_Write(glowAddress + 0x14, glowColor.a / 255.0f);  // A

                // Glow parametrelerini ayarla
                m_processManager.GD_Write<bool>(glowAddress + 0x28, true);  // RenderWhenOccluded
                m_processManager.GD_Write<bool>(glowAddress + 0x29, false); // RenderWhenUnoccluded
            }
        }

        // Silah ve bomba glow efekti burada eklenebilir (�zellik geni�letildi�inde)
    }

    void FeatureManager::GD_UpdateFOV() {
        auto& offsets = m_configManager.GetOffsets();
        auto& fovConfig = m_configManager.GetFOVConfig();

        if (!m_gameState.localPlayerAddress) return;

        // Yerel oyuncu kontrolc�s�n� al
        uintptr_t localPlayerPawn = m_processManager.GD_Read<uintptr_t>(m_gameState.localPlayerAddress + offsets.m_hPlayerPawn);
        if (!localPlayerPawn) return;

        // FOV de�erini al
        int currentFOV = m_processManager.GD_Read<int>(localPlayerPawn + offsets.m_iFOV);
        int defaultFOV = m_processManager.GD_Read<int>(localPlayerPawn + offsets.m_iDefaultFOV);

        // FOV de�erini g�ncelle
        if (fovConfig.dynamicFOV) {
            // Dinamik FOV: Ko�arken veya zoomlarken orijinal de�erlere benzer �ekilde davran
            bool isScoped = m_processManager.GD_Read<bool>(localPlayerPawn + offsets.m_bIsScoped);

            if (isScoped) {
                // Zoom yaparken FOV'u azalt
                int newFOV = static_cast<int>(fovConfig.customFOV * fovConfig.zoomFactor);
                m_processManager.GD_Write(localPlayerPawn + offsets.m_iFOV, newFOV);
            }
            else {
                // Normal durumda
                m_processManager.GD_Write(localPlayerPawn + offsets.m_iFOV, static_cast<int>(fovConfig.customFOV));
            }
        }
        else {
            // Sabit FOV de�eri
            m_processManager.GD_Write(localPlayerPawn + offsets.m_iFOV, static_cast<int>(fovConfig.customFOV));
        }

        // Varsay�lan FOV'u da ayarla (scope vb. durumlar i�in referans olarak kullan�l�r)
        m_processManager.GD_Write(localPlayerPawn + offsets.m_iDefaultFOV, static_cast<int>(fovConfig.defaultFOV));

        // Viewmodel FOV ayar� da eklenebilir (�zellik geni�letildi�inde)
    }

    void FeatureManager::GD_UpdateSkins() {
        // Bu �zellik daha karma��k, SkinChanger s�n�f�nda implemente edilecek
    }

    //===========================================
    // FeatureController Implementasyonu
    //===========================================

    bool FeatureController::GD_Initialize() {
        if (m_initialized) return true;

        bool success = true;

        // Feature Manager'� ba�lat
        success &= m_featureManager.GD_Initialize();

        // ESP Renderer'� ba�lat
        success &= m_espRenderer.GD_Initialize();

        // Skin Changer'� ba�lat
        success &= m_skinChanger.GD_Initialize();

        // FOV Changer'� ba�lat
        success &= m_fovChanger.GD_Initialize();

        m_initialized = success;
        return success;
    }

    void FeatureController::GD_Update() {
        if (!m_initialized) return;

        // Feature Manager g�ncellemesi
        m_featureManager.GD_Update();

        // FOV Changer g�ncellemesi
        if (m_configManager.GetFOVConfig().enabled) {
            m_fovChanger.GD_Update();
        }

        // Skin Changer g�ncellemesi
        if (m_configManager.GetSkinChangerConfig().enabled) {
            m_skinChanger.GD_Update();
        }
    }

    void FeatureController::GD_Render() {
        if (!m_initialized) return;

        // ESP render etme
        if (m_configManager.GetESPConfig().enabled) {
            m_espRenderer.GD_Render();
        }
    }

    void FeatureController::GD_Shutdown() {
        if (!m_initialized) return;

        // Kaynaklar� temizle

        // FOV ayarlar�n� s�f�rla
        m_fovChanger.GD_RestoreOriginalFOV();

        m_initialized = false;
    }

    //===========================================
    // ESPRenderer Implementasyonu
    //===========================================

    bool ESPRenderer::GD_Initialize() {
        // Direct2D/DirectWrite ba�latma i�lemleri burada olacak
        // �imdilik sadece ba�ar� d�nd�r
        return true;
    }

    void ESPRenderer::GD_Render() {
        auto& espConfig = m_configManager.GetESPConfig();
        const auto& players = m_featureManager.GetPlayers();

        // Oyuncular� render et
        for (const auto& player : players) {
            // Yap�land�rma kontrol�
            if (player.isTeammate && !espConfig.showTeammates) continue;
            if (!player.isTeammate && !espConfig.showEnemies) continue;

            // Maksimum render mesafesi kontrol�
            if (player.distance > espConfig.maxRenderDistance) continue;

            // ESP Box rengi
            GD_Color boxColor;
            if (player.isDefusing) {
                boxColor = espConfig.defusingColor;
            }
            else if (player.isTeammate) {
                boxColor = espConfig.teammateColor;
            }
            else {
                boxColor = espConfig.enemyColor;
            }

            // D���k sa�l�k kontrol�
            if (player.health < 30) {
                boxColor = espConfig.lowHealthColor;
            }

            // ESP Box �izme
            if (espConfig.showBox && player.isVisible) {
                float height = player.headScreenPos.y - player.screenPos.y;
                float width = height * 0.65f;  // �nsan v�cudu oran� yakla��k

                GD_Vector2 topLeft = {
                    player.screenPos.x - width / 2,
                    player.headScreenPos.y
                };

                GD_Vector2 bottomRight = {
                    player.screenPos.x + width / 2,
                    player.screenPos.y
                };

                GD_DrawBox(topLeft, bottomRight, boxColor, static_cast<float>(espConfig.boxThickness));

                // Sa�l�k �ubu�u �izme
                if (espConfig.showHealth) {
                    GD_DrawHealthBar(
                        { topLeft.x - 8.0f, topLeft.y },  // Sol �st
                        { topLeft.x - 3.0f, bottomRight.y },  // Sa� alt
                        player.health
                    );
                }

                // �sim �izme
                if (espConfig.showName) {
                    GD_DrawName(
                        { (topLeft.x + bottomRight.x) / 2, topLeft.y - 15.0f },
                        player.name,
                        boxColor
                    );
                }

                // Silah ismi �izme
                if (espConfig.showWeapons && !player.weaponName.empty()) {
                    GD_DrawWeaponName(
                        { (topLeft.x + bottomRight.x) / 2, bottomRight.y + 5.0f },
                        player.weaponName,
                        boxColor
                    );
                }

                // Mesafe �izme
                if (espConfig.showDistance) {
                    GD_DrawDistance(
                        { (topLeft.x + bottomRight.x) / 2, bottomRight.y + 15.0f },
                        player.distance,
                        boxColor
                    );
                }
            }
        }
    }

    // �izim fonksiyonlar� (Direct2D/DirectWrite ile implemente edilecek)
    void ESPRenderer::GD_DrawBox(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, const GD_Color& color, float thickness) {
        // This is a placeholder. In a real implementation, this would use Direct2D to draw
    }

    void ESPRenderer::GD_DrawHealthBar(const GD_Vector2& topLeft, const GD_Vector2& bottomRight, int health) {
        // This is a placeholder. In a real implementation, this would use Direct2D to draw
    }

    void ESPRenderer::GD_DrawName(const GD_Vector2& position, const std::string& name, const GD_Color& color) {
        // This is a placeholder. In a real implementation, this would use DirectWrite to draw text
    }

    void ESPRenderer::GD_DrawWeaponName(const GD_Vector2& position, const std::string& weaponName, const GD_Color& color) {
        // This is a placeholder. In a real implementation, this would use DirectWrite to draw text
    }

    void ESPRenderer::GD_DrawDistance(const GD_Vector2& position, float distance, const GD_Color& color) {
        // This is a placeholder. In a real implementation, this would use DirectWrite to draw text
    }

    //===========================================
    // SkinChanger Implementasyonu
    //===========================================

    bool SkinChanger::GD_Initialize() {
        // Silah offsetlerini bul
        GD_FindWeaponHandles();
        return true;
    }

    void SkinChanger::GD_Update() {
        auto& skinChangerConfig = m_configManager.GetSkinChangerConfig();

        // Sadece spawn olunca g�ncelleme kontrol�
        static bool updatedOnce = false;
        if (skinChangerConfig.updateOnlyOnSpawn && updatedOnce) {
            return;
        }

        // Silah listesini g�ncelle
        GD_FindWeaponHandles();

        // T�m silahlar� g�ncelle
        for (const auto& weaponHandle : m_weaponHandles) {
            // Silah i�in uygun skin'i bul
            const GD_WeaponSkin* skin = GD_FindSkinForWeapon(weaponHandle.itemDefinitionIndex);

            // Skin bulundu mu kontrol et
            if (skin) {
                GD_ApplySkin(weaponHandle.weaponAddress, *skin);
            }
        }

        // B��ak skin'i de�i�tirme
        if (skinChangerConfig.knifeSkinChanger && skinChangerConfig.knifeModel != 0) {
            // T�m silahlar� kontrol et ve b��a�� bul
            for (const auto& weaponHandle : m_weaponHandles) {
                // B��ak ID aral��� kontrol� (CS2'ye g�re)
                if (weaponHandle.itemDefinitionIndex >= 500 && weaponHandle.itemDefinitionIndex <= 525) {
                    // B��ak modelini ve skin'i de�i�tir
                    m_processManager.GD_Write(weaponHandle.weaponAddress + m_configManager.GetOffsets().m_iItemDefinitionIndex, skinChangerConfig.knifeModel);

                    // B��ak skin'ini uygula
                    if (skinChangerConfig.knifeSkin > 0) {
                        // Ge�ici bir skin bilgisi olu�tur
                        GD_WeaponSkin knifeSkin;
                        knifeSkin.weaponID = skinChangerConfig.knifeModel;
                        knifeSkin.paintKit = skinChangerConfig.knifeSkin;
                        knifeSkin.wear = 0.01f;  // Fabrikadan yeni
                        knifeSkin.seed = 0;

                        GD_ApplySkin(weaponHandle.weaponAddress, knifeSkin);
                    }

                    m_knifeChanged = true;
                }
            }
        }

        updatedOnce = true;
    }

    void SkinChanger::GD_FindWeaponHandles() {
        auto& offsets = m_configManager.GetOffsets();

        // Silah listesini temizle
        m_weaponHandles.clear();

        // Yerel oyuncu kontrolc�s� adresini al
        uintptr_t localPlayerController = m_processManager.GD_Read<uintptr_t>(
            m_processManager.GetBaseModule().base + offsets.dwLocalPlayerController);
        if (!localPlayerController) return;

        // Yerel oyuncunun silahlar�n� tara
        for (int i = 0; i < 64; i++) {
            uintptr_t weaponHandle = m_processManager.GD_Read<uintptr_t>(
                localPlayerController + offsets.m_hMyWeapons + i * sizeof(uintptr_t));

            if (!weaponHandle) continue;

            int itemDefinitionIndex = m_processManager.GD_Read<int>(
                weaponHandle + offsets.m_iItemDefinitionIndex);

            if (itemDefinitionIndex > 0) {
                // Ge�erli bir silah, listeye ekle
                WeaponHandleInfo weaponInfo;
                weaponInfo.itemDefinitionIndex = itemDefinitionIndex;
                weaponInfo.weaponAddress = weaponHandle;

                m_weaponHandles.push_back(weaponInfo);
            }
        }
    }

    void SkinChanger::GD_ApplySkin(uintptr_t weaponAddress, const GD_WeaponSkin& skinInfo) {
        auto& offsets = m_configManager.GetOffsets();

        // Silah ID kontrolleri
        int currentItemDef = m_processManager.GD_Read<int>(weaponAddress + offsets.m_iItemDefinitionIndex);
        if (currentItemDef != skinInfo.weaponID && skinInfo.weaponID != 0) return;

        // Skin bilgilerini yaz
        if (skinInfo.paintKit > 0) {
            m_processManager.GD_Write<int>(weaponAddress + offsets.m_nFallbackPaintKit, skinInfo.paintKit);
        }

        if (skinInfo.seed >= 0) {
            m_processManager.GD_Write<int>(weaponAddress + offsets.m_nFallbackSeed, skinInfo.seed);
        }

        if (skinInfo.wear >= 0.0f && skinInfo.wear <= 1.0f) {
            m_processManager.GD_Write<float>(weaponAddress + offsets.m_flFallbackWear, skinInfo.wear);
        }

        if (skinInfo.statTrak >= 0) {
            m_processManager.GD_Write<int>(weaponAddress + offsets.m_nFallbackStatTrak, skinInfo.statTrak);
        }

        // Silah� sahiplenme ayarlar�
        m_processManager.GD_Write<int>(weaponAddress + offsets.m_iItemIDHigh, -1);
        m_processManager.GD_Write<int>(weaponAddress + offsets.m_OriginalOwnerXuidLow, 0);
        m_processManager.GD_Write<int>(weaponAddress + offsets.m_iAccountID, 1);
    }

    std::string SkinChanger::GD_GetWeaponNameById(int weaponId) {
        // CS2 silah ID'lerine g�re adlar
        // Not: Bu liste eksik olabilir, gerekti�inde g�ncellenebilir
        static const std::map<int, std::string> weaponNames = {
            {1, "Desert Eagle"},
            {2, "Dual Berettas"},
            {3, "Five-SeveN"},
            {4, "Glock-18"},
            {7, "AK-47"},
            {8, "AUG"},
            {9, "AWP"},
            {10, "FAMAS"},
            {11, "G3SG1"},
            {13, "Galil AR"},
            {14, "M249"},
            {16, "M4A4"},
            {17, "MAC-10"},
            {19, "P90"},
            {20, "MP5-SD"},
            {23, "MP7"},
            {24, "UMP-45"},
            {25, "XM1014"},
            {26, "PP-Bizon"},
            {27, "MAG-7"},
            {28, "Negev"},
            {29, "Sawed-Off"},
            {30, "Tec-9"},
            {31, "Zeus x27"},
            {32, "P2000"},
            {33, "MP9"},
            {34, "Nova"},
            {35, "P250"},
            {36, "SCAR-20"},
            {38, "SCAR-20"},
            {39, "SG 553"},
            {40, "SSG 08"},
            {60, "M4A1-S"},
            {61, "USP-S"},
            {63, "CZ75-Auto"},
            {64, "R8 Revolver"},
            // B��aklar
            {500, "Bayonet"},
            {505, "Flip Knife"},
            {506, "Gut Knife"},
            {507, "Karambit"},
            {508, "M9 Bayonet"},
            {509, "Huntsman Knife"},
            {512, "Falchion Knife"},
            {514, "Bowie Knife"},
            {515, "Butterfly Knife"},
            {516, "Shadow Daggers"},
            {517, "Paracord Knife"},
            {518, "Survival Knife"},
            {519, "Ursus Knife"},
            {520, "Navaja Knife"},
            {521, "Nomad Knife"},
            {522, "Stiletto Knife"},
            {523, "Talon Knife"},
            {525, "Classic Knife"}
        };

        auto it = weaponNames.find(weaponId);
        if (it != weaponNames.end()) {
            return it->second;
        }

        return "Unknown Weapon";
    }

    const GD_WeaponSkin* SkinChanger::GD_FindSkinForWeapon(int weaponId) {
        auto& skinChangerConfig = m_configManager.GetSkinChangerConfig();

        // Yap�land�r�lm�� silah skinlerini kontrol et
        for (const auto& skin : skinChangerConfig.weaponSkins) {
            if (skin.weaponID == weaponId || (skin.weaponID == 0 && skin.paintKit > 0)) {
                return &skin;
            }
        }

        return nullptr;
    }

    //===========================================
    // FOVChanger Implementasyonu
    //===========================================

    bool FOVChanger::GD_Initialize() {
        auto& offsets = m_configManager.GetOffsets();

        // Yerel oyuncu kontrolc�s� adresini al
        uintptr_t localPlayerController = m_processManager.GD_Read<uintptr_t>(
            m_processManager.GetBaseModule().base + offsets.dwLocalPlayerController);
        if (!localPlayerController) return false;

        // Yerel oyuncu pawn adresini al
        uintptr_t localPlayerPawn = m_processManager.GD_Read<uintptr_t>(
            localPlayerController + offsets.m_hPlayerPawn);
        if (!localPlayerPawn) return false;

        // Orijinal FOV de�erini kaydet
        m_originalFOV = m_processManager.GD_Read<float>(localPlayerPawn + offsets.m_iDefaultFOV);

        // View angle adresini kaydet
        m_viewAngleAddress = localPlayerPawn + offsets.m_iFOV;

        m_initialized = true;
        return true;
    }

    void FOVChanger::GD_Update() {
        if (!m_initialized) {
            // Tekrar ba�latmay� dene
            if (!GD_Initialize()) return;
        }

        auto& fovConfig = m_configManager.GetFOVConfig();

        // FOV de�erini g�ncelle
        if (m_viewAngleAddress) {
            m_processManager.GD_Write<float>(m_viewAngleAddress, fovConfig.customFOV);
        }
    }

    void FOVChanger::GD_RestoreOriginalFOV() {
        if (!m_initialized || !m_viewAngleAddress) return;

        // Orijinal FOV de�erini geri yaz
        m_processManager.GD_Write<float>(m_viewAngleAddress, m_originalFOV);
    }

} // namespace geezy_digital