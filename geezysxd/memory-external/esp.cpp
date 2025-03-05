// esp.cpp
#include "esp.hpp"
#include "logger.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace esp {

    ESP::ESP() {
        LoadOffsets();
    }

    ESP::~ESP() {
    }

    bool ESP::LoadOffsets() {
        // Offset deðerlerini JSON dosyasýndan veya hard-coded olarak yükle
        // Þimdilik temel offsetleri tanýmlayalým
        offsets.dwEntityList = 0x17C1950;
        offsets.dwLocalPlayerController = 0x1810EC0;
        offsets.dwViewMatrix = 0x1820150;
        offsets.dwPlantedC4 = 0x188E440;
        offsets.m_hPlayerPawn = 0x7EC;
        offsets.m_iTeamNum = 0x3BF;
        offsets.m_vOldOrigin = 0xE54;
        offsets.m_iHealth = 0x32C;
        offsets.m_ArmorValue = 0x1508;
        offsets.m_pGameSceneNode = 0x310;
        offsets.m_sSanitizedPlayerName = 0x640;
        offsets.m_bIsDefusing = 0x1531;
        offsets.m_pInGameMoneyServices = 0x650;
        offsets.m_iAccount = 0x40;
        offsets.m_flFlashOverlayAlpha = 0x1450;
        offsets.m_pClippingWeapon = 0x12A0;
        offsets.m_szName = 0x20;
        offsets.m_vecAbsOrigin = 0x80;

        utils::LogInfo("ESP offsetleri yüklendi");
        return true;
    }

    ImVec2 ESP::WorldToScreen(const ViewMatrix& viewMatrix, const float* position, int screenWidth, int screenHeight) {
        // DirectX10 ile uyumlu world to screen dönüþümü
        float w = viewMatrix.matrix[3] * position[0] + viewMatrix.matrix[7] * position[1] + viewMatrix.matrix[11] * position[2] + viewMatrix.matrix[15];

        if (w < 0.001f)
            return ImVec2(0, 0);

        float x = viewMatrix.matrix[0] * position[0] + viewMatrix.matrix[4] * position[1] + viewMatrix.matrix[8] * position[2] + viewMatrix.matrix[12];
        float y = viewMatrix.matrix[1] * position[0] + viewMatrix.matrix[5] * position[1] + viewMatrix.matrix[9] * position[2] + viewMatrix.matrix[13];

        // Normalize edin
        float invw = 1.0f / w;
        x *= invw;
        y *= invw;

        // Normalize koordinatlarý ekran koordinatlarýna dönüþtürün
        float screenX = (screenWidth / 2.0f) * (1.0f + x);
        float screenY = (screenHeight / 2.0f) * (1.0f - y);

        return ImVec2(screenX, screenY);
    }

    float ESP::CalculateDistance(const ImVec2& a, const ImVec2& b) {
        return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
    }

    void ESP::UpdatePlayerData(game::GameInterface* gameInterface) {
        if (!gameInterface || !gameInterface->IsConnected() || !gameInterface->GetMemoryManager())
            return;

        Memory::MemoryManager* memoryManager = gameInterface->GetMemoryManager();
        uintptr_t clientModule = gameInterface->GetClientModuleBase();

        if (!clientModule)
            return;

        // Yerel oyuncu bilgilerini al
        uintptr_t localPlayerController = memoryManager->Read<uintptr_t>(clientModule + offsets.dwLocalPlayerController);
        if (!localPlayerController)
            return;

        uint32_t localPlayerPawn = memoryManager->Read<uint32_t>(localPlayerController + offsets.m_hPlayerPawn);
        if (!localPlayerPawn)
            return;

        uintptr_t entityList = memoryManager->Read<uintptr_t>(clientModule + offsets.dwEntityList);
        if (!entityList)
            return;

        uintptr_t localListEntry2 = memoryManager->Read<uintptr_t>(entityList + 0x8 * ((localPlayerPawn & 0x7FFF) >> 9) + 16);
        if (!localListEntry2)
            return;

        uintptr_t localpCSPlayerPawn = memoryManager->Read<uintptr_t>(localListEntry2 + 120 * (localPlayerPawn & 0x1FF));
        if (!localpCSPlayerPawn)
            return;

        // View matrix al
        ViewMatrix viewMatrix = memoryManager->Read<ViewMatrix>(clientModule + offsets.dwViewMatrix);

        // Ekran boyutunu al
        RECT windowRect;
        HWND gameWindow = gameInterface->FindGameWindow();
        if (!gameWindow)
            return;

        GetClientRect(gameWindow, &windowRect);
        int screenWidth = windowRect.right - windowRect.left;
        int screenHeight = windowRect.bottom - windowRect.top;

        // Yerel oyuncu takýmý ve konumu
        localTeam = memoryManager->Read<int>(localPlayerController + offsets.m_iTeamNum);

        // Yerel oyuncunun konumunu al
        float originRaw[3];
        originRaw[0] = memoryManager->Read<float>(localpCSPlayerPawn + offsets.m_vOldOrigin);
        originRaw[1] = memoryManager->Read<float>(localpCSPlayerPawn + offsets.m_vOldOrigin + 4);
        originRaw[2] = memoryManager->Read<float>(localpCSPlayerPawn + offsets.m_vOldOrigin + 8);
        localOrigin = ImVec2(originRaw[0], originRaw[1]);

        // C4 bilgilerini kontrol et
        isC4Planted = memoryManager->Read<bool>(clientModule + offsets.dwPlantedC4 - 0x8);
        if (isC4Planted) {
            uintptr_t plantedC4 = memoryManager->Read<uintptr_t>(memoryManager->Read<uintptr_t>(clientModule + offsets.dwPlantedC4));
            if (plantedC4) {
                uintptr_t c4Node = memoryManager->Read<uintptr_t>(plantedC4 + offsets.m_pGameSceneNode);
                if (c4Node) {
                    float c4OriginRaw[3];
                    c4OriginRaw[0] = memoryManager->Read<float>(c4Node + offsets.m_vecAbsOrigin);
                    c4OriginRaw[1] = memoryManager->Read<float>(c4Node + offsets.m_vecAbsOrigin + 4);
                    c4OriginRaw[2] = memoryManager->Read<float>(c4Node + offsets.m_vecAbsOrigin + 8);
                    c4Info.position = WorldToScreen(viewMatrix, c4OriginRaw, screenWidth, screenHeight);
                }
            }
        }

        // Oyuncu listesini temizle
        players.clear();

        // Oyuncu listesini tara
        int playerIndex = 0;
        while (true) {
            playerIndex++;
            uintptr_t listEntry = memoryManager->Read<uintptr_t>(entityList + (8 * (playerIndex & 0x7FFF) >> 9) + 16);
            if (!listEntry)
                break;

            uintptr_t entity = memoryManager->Read<uintptr_t>(listEntry + 120 * (playerIndex & 0x1FF));
            if (!entity)
                continue;

            // Oyuncu takýmýný al
            int team = memoryManager->Read<int>(entity + offsets.m_iTeamNum);

            // Takým arkadaþý filtresi
            if (teamESP && (team == localTeam))
                continue;

            uint32_t playerPawn = memoryManager->Read<uint32_t>(entity + offsets.m_hPlayerPawn);
            if (!playerPawn)
                continue;

            uintptr_t listEntry2 = memoryManager->Read<uintptr_t>(entityList + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);
            if (!listEntry2)
                continue;

            uintptr_t pCSPlayerPawn = memoryManager->Read<uintptr_t>(listEntry2 + 120 * (playerPawn & 0x1FF));
            if (!pCSPlayerPawn)
                continue;

            // Oyuncu saðlýk durumunu kontrol et
            int health = memoryManager->Read<int>(pCSPlayerPawn + offsets.m_iHealth);
            if (health <= 0 || health > 100)
                continue;

            // Oyuncu bilgilerini al
            PlayerInfo player;
            player.team = team;
            player.health = health;
            player.armor = memoryManager->Read<int>(pCSPlayerPawn + offsets.m_ArmorValue);

            // Oyuncu adýný al
            uintptr_t playerNameData = memoryManager->Read<uintptr_t>(entity + offsets.m_sSanitizedPlayerName);
            char nameBuffer[256] = { 0 };

            // Ýlk 256 karakteri oku
            for (int i = 0; i < 256; ++i) {
                nameBuffer[i] = memoryManager->Read<char>(playerNameData + i);
                if (nameBuffer[i] == '\0') break;
            }

            player.name = std::string(nameBuffer);

            // Oyuncu konumunu al
            float originRaw[3];
            originRaw[0] = memoryManager->Read<float>(pCSPlayerPawn + offsets.m_vOldOrigin);
            originRaw[1] = memoryManager->Read<float>(pCSPlayerPawn + offsets.m_vOldOrigin + 4);
            originRaw[2] = memoryManager->Read<float>(pCSPlayerPawn + offsets.m_vOldOrigin + 8);

            // Kendimizi filtrele
            if (originRaw[0] == localOrigin.x && originRaw[1] == localOrigin.y)
                continue;

            // Mesafe kontrolü
            float distance = std::sqrt(std::pow(originRaw[0] - localOrigin.x, 2) + std::pow(originRaw[1] - localOrigin.y, 2));
            if (renderDistance != -1 && distance > renderDistance)
                continue;

            // Baþ pozisyonunu hesapla (head = feet + 75 birim)
            float headRaw[3] = { originRaw[0], originRaw[1], originRaw[2] + 75.0f };

            player.origin = WorldToScreen(viewMatrix, originRaw, screenWidth, screenHeight);
            player.head = WorldToScreen(viewMatrix, headRaw, screenWidth, screenHeight);

            // Ýskelet ve kafa izleyici için kemik bilgilerini al
            if (showSkeletonESP || showHeadTracker) {
                uintptr_t gameSceneNode = memoryManager->Read<uintptr_t>(pCSPlayerPawn + offsets.m_pGameSceneNode);
                uintptr_t boneArray = memoryManager->Read<uintptr_t>(gameSceneNode + 0x1F0);

                // Kafa izleyiciyi göster
                if (showHeadTracker) {
                    uintptr_t boneAddress = boneArray + 6 * 32; // head bone index = 6
                    float bonePos[3];
                    bonePos[0] = memoryManager->Read<float>(boneAddress);
                    bonePos[1] = memoryManager->Read<float>(boneAddress + 4);
                    bonePos[2] = memoryManager->Read<float>(boneAddress + 8);
                    player.bones.bonePositions["head"] = WorldToScreen(viewMatrix, bonePos, screenWidth, screenHeight);
                }

                // Ýskelet ESP için tüm kemikleri al
                if (showSkeletonESP) {
                    for (const auto& entry : boneMap) {
                        const std::string& boneName = entry.first;
                        int boneIndex = entry.second;
                        uintptr_t boneAddress = boneArray + boneIndex * 32;

                        float bonePos[3];
                        bonePos[0] = memoryManager->Read<float>(boneAddress);
                        bonePos[1] = memoryManager->Read<float>(boneAddress + 4);
                        bonePos[2] = memoryManager->Read<float>(boneAddress + 8);
                        player.bones.bonePositions[boneName] = WorldToScreen(viewMatrix, bonePos, screenWidth, screenHeight);
                    }
                }
            }

            // Ek bayraklarý göster
            if (showExtraFlags) {
                player.isDefusing = memoryManager->Read<bool>(pCSPlayerPawn + offsets.m_bIsDefusing);

                uintptr_t playerMoneyServices = memoryManager->Read<uintptr_t>(entity + offsets.m_pInGameMoneyServices);
                player.money = memoryManager->Read<int32_t>(playerMoneyServices + offsets.m_iAccount);

                player.flashAlpha = memoryManager->Read<float>(pCSPlayerPawn + offsets.m_flFlashOverlayAlpha);

                uintptr_t clippingWeapon = memoryManager->Read<uint64_t>(pCSPlayerPawn + offsets.m_pClippingWeapon);
                if (clippingWeapon) {
                    uintptr_t firstLevel = memoryManager->Read<uint64_t>(clippingWeapon + 0x10);
                    uintptr_t weaponData = memoryManager->Read<uint64_t>(firstLevel + 0x20);

                    if (weaponData) {
                        char weaponBuffer[128] = { 0 };

                        // Ýlk 128 karakteri oku
                        for (int i = 0; i < 128; ++i) {
                            weaponBuffer[i] = memoryManager->Read<char>(weaponData + i);
                            if (weaponBuffer[i] == '\0') break;
                        }

                        std::string weaponName = std::string(weaponBuffer);

                        if (weaponName.compare(0, 7, "weapon_") == 0)
                            player.weapon = weaponName.substr(7);
                        else
                            player.weapon = "Unknown";
                    }
                }
            }

            // Oyuncuyu listeye ekle
            players.push_back(player);
        }
    }

    void ESP::Render(game::GameInterface* gameInterface) {
        if (!gameInterface || !gameInterface->IsConnected())
            return;

        // Oyuncu verilerini güncelle
        UpdatePlayerData(gameInterface);

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // C4 bombasýný göster
        if (isC4Planted && c4Info.position.x > 0 && c4Info.position.y > 0) {
            float c4Distance = CalculateDistance(ImVec2(localOrigin.x, localOrigin.y), ImVec2(c4Info.position.x, c4Info.position.y));
            float c4RoundedDistance = std::round(c4Distance / 500.0f);

            float height = 10 - c4RoundedDistance;
            float width = height * 1.4f;

            drawList->AddRectFilled(
                ImVec2(c4Info.position.x - (width / 2), c4Info.position.y - (height / 2)),
                ImVec2(c4Info.position.x + (width / 2), c4Info.position.y + (height / 2)),
                ImGui::ColorConvertFloat4ToU32(boxColorEnemy)
            );

            drawList->AddText(
                ImVec2(c4Info.position.x + (width / 2 + 5), c4Info.position.y),
                ImGui::ColorConvertFloat4ToU32(nameColor),
                "C4"
            );
        }

        // Oyuncularý göster
        for (const auto& player : players) {
            // Ekranda geçerli olup olmadýðýný kontrol et
            if (player.origin.x <= 0 || player.origin.y <= 0 || player.head.x <= 0 || player.head.y <= 0)
                continue;

            // Mesafe hesapla
            float distance = CalculateDistance(ImVec2(localOrigin.x, localOrigin.y), ImVec2(player.origin.x, player.origin.y));
            int roundedDistance = std::round(distance / 10.0f);

            // ESP kutusu boyutlarýný hesapla
            float height = player.origin.y - player.head.y;
            float width = height / 2.4f;

            // Kafa izleyiciyi göster
            if (showHeadTracker && player.bones.bonePositions.find("head") != player.bones.bonePositions.end()) {
                ImVec4 headColor = (localTeam == player.team) ? skeletonColorTeam : skeletonColorEnemy;
                drawList->AddCircle(
                    ImVec2(player.bones.bonePositions.at("head").x, player.bones.bonePositions.at("head").y - width / 12),
                    width / 5,
                    ImGui::ColorConvertFloat4ToU32(headColor)
                );
            }

            // Ýskelet ESP göster
            if (showSkeletonESP) {
                ImVec4 skeletonColor = (localTeam == player.team) ? skeletonColorTeam : skeletonColorEnemy;
                for (const auto& connection : boneConnections) {
                    const std::string& boneFrom = connection.first;
                    const std::string& boneTo = connection.second;

                    auto fromIt = player.bones.bonePositions.find(boneFrom);
                    auto toIt = player.bones.bonePositions.find(boneTo);

                    if (fromIt != player.bones.bonePositions.end() && toIt != player.bones.bonePositions.end()) {
                        drawList->AddLine(
                            ImVec2(fromIt->second.x, fromIt->second.y),
                            ImVec2(toIt->second.x, toIt->second.y),
                            ImGui::ColorConvertFloat4ToU32(skeletonColor)
                        );
                    }
                }
            }

            // ESP kutusu göster
            if (showBoxESP) {
                ImVec4 boxColor = (localTeam == player.team) ? boxColorTeam : boxColorEnemy;
                drawList->AddRect(
                    ImVec2(player.head.x - width / 2, player.head.y),
                    ImVec2(player.head.x + width / 2, player.origin.y),
                    ImGui::ColorConvertFloat4ToU32(boxColor)
                );
            }

            // Zýrh ve saðlýk çubuklarý
            drawList->AddRectFilled(
                ImVec2(player.head.x - (width / 2 + 10), player.head.y + (height * (100 - player.armor) / 100)),
                ImVec2(player.head.x - (width / 2 + 8), player.origin.y),
                IM_COL32(0, 185, 255, 255)
            );

            drawList->AddRectFilled(
                ImVec2(player.head.x - (width / 2 + 5), player.head.y + (height * (100 - player.health) / 100)),
                ImVec2(player.head.x - (width / 2 + 3), player.origin.y),
                IM_COL32(
                    255 - player.health,
                    55 + player.health * 2,
                    75,
                    255
                )
            );

            // Oyuncu adýný göster
            drawList->AddText(
                ImVec2(player.head.x + (width / 2 + 5), player.head.y),
                ImGui::ColorConvertFloat4ToU32(nameColor),
                player.name.c_str()
            );

            // Mesafe kontrolü - ek bilgiler için
            if (roundedDistance > flagRenderDistance)
                continue;

            // Saðlýk ve zýrh bilgisi
            drawList->AddText(
                ImVec2(player.head.x + (width / 2 + 5), player.head.y + 10),
                IM_COL32(
                    255 - player.health,
                    55 + player.health * 2,
                    75,
                    255
                ),
                (std::to_string(player.health) + "hp").c_str()
            );

            drawList->AddText(
                ImVec2(player.head.x + (width / 2 + 5), player.head.y + 20),
                IM_COL32(
                    255 - player.armor,
                    55 + player.armor * 2,
                    75,
                    255
                ),
                (std::to_string(player.armor) + "armor").c_str()
            );

            // Ek bayraklarý göster
            if (showExtraFlags) {
                drawList->AddText(
                    ImVec2(player.head.x + (width / 2 + 5), player.head.y + 30),
                    ImGui::ColorConvertFloat4ToU32(distanceColor),
                    player.weapon.c_str()
                );

                drawList->AddText(
                    ImVec2(player.head.x + (width / 2 + 5), player.head.y + 40),
                    ImGui::ColorConvertFloat4ToU32(distanceColor),
                    (std::to_string(roundedDistance) + "m away").c_str()
                );

                drawList->AddText(
                    ImVec2(player.head.x + (width / 2 + 5), player.head.y + 50),
                    IM_COL32(0, 125, 0, 255),
                    ("$" + std::to_string(player.money)).c_str()
                );

                if (player.flashAlpha > 100) {
                    drawList->AddText(
                        ImVec2(player.head.x + (width / 2 + 5), player.head.y + 60),
                        ImGui::ColorConvertFloat4ToU32(distanceColor),
                        "Player is flashed"
                    );
                }

                if (player.isDefusing) {
                    drawList->AddText(
                        ImVec2(player.head.x + (width / 2 + 5), player.head.y + 60),
                        ImGui::ColorConvertFloat4ToU32(distanceColor),
                        "Player is defusing"
                    );
                }
            }
        }
    }

} // namespace esp