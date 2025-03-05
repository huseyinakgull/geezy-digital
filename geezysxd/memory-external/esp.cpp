#include "esp.hpp"
#include <string>

namespace core {

    // ESP s�n�f�n�n yap�c� fonksiyonu - Haf�za y�neticisini alacak �ekilde g�ncellenmi�
    ESP::ESP(Memory::MemoryManager* memoryManager)
        : m_memoryManager(memoryManager)
        , m_enabled(false)
        , m_teamESP(true)
        , m_boxESP(true)
        , m_nameESP(true)
        , m_healthESP(true)
        , m_distanceESP(false)
        , m_headpointESP(false)
    {
    }

    // Offset kullan�m�na �rnek - GetPlayerEntity fonksiyonu
    uintptr_t ESP::GetPlayerEntity(int index) {
        if (!m_memoryManager) return 0;

        uintptr_t clientModule = m_memoryManager->GetModuleBaseAddress("client.dll");
        if (clientModule == 0) return 0;

        // JSON'dan dwEntityList offset'ini kullan
        uintptr_t entityList = clientModule + m_memoryManager->GetOffset("dwEntityList");
        uintptr_t listEntry = m_memoryManager->Read<uintptr_t>(entityList + (8 * (index & 0x7FFF) >> 9) + 16);
        if (listEntry == 0) return 0;

        return m_memoryManager->Read<uintptr_t>(listEntry + 120 * (index & 0x1FF));
    }

    // Oyuncunun sa�l�k de�erini al
    int ESP::GetPlayerHealth(uintptr_t playerEntity) {
        if (!m_memoryManager || playerEntity == 0) return 0;

        // JSON'dan m_iHealth offset'ini kullan
        return m_memoryManager->Read<int>(playerEntity + m_memoryManager->GetOffset("m_iHealth"));
    }

    // Oyuncunun tak�m numaras�n� al
    int ESP::GetPlayerTeam(uintptr_t playerEntity) {
        if (!m_memoryManager || playerEntity == 0) return 0;

        // JSON'dan m_iTeamNum offset'ini kullan
        return m_memoryManager->Read<int>(playerEntity + m_memoryManager->GetOffset("m_iTeamNum"));
    }

    // Oyuncunun pozisyonunu al
    Vector3 ESP::GetPlayerPosition(uintptr_t playerEntity) {
        if (!m_memoryManager || playerEntity == 0) return { 0, 0, 0 };

        // JSON'dan m_vecOrigin offset'ini kullan - string format�nda olabilir
        uintptr_t originOffset = m_memoryManager->GetOffset("m_vecOrigin");
        return m_memoryManager->Read<Vector3>(playerEntity + originOffset);
    }


    // Oyuncunun ismini al
    std::string ESP::GetPlayerName(uintptr_t playerEntity) {
        if (!m_memoryManager || playerEntity == 0) return "Unknown";

        // JSON'dan m_sSanitizedPlayerName offset'ini kullan
        uintptr_t nameOffset = m_memoryManager->GetOffset("m_sSanitizedPlayerName");

        // Mevcut ReadBytes fonksiyonu bir vector<uint8_t> d�nd�r�r
        std::vector<uint8_t> nameData = m_memoryManager->ReadBytes(playerEntity + nameOffset, 128);

        if (!nameData.empty()) {
            // Vector'dan string olu�tur - null terminasyonu dikkate alarak
            return std::string(reinterpret_cast<char*>(nameData.data()));
        }

        return "Unknown";
    }

    // Lokal oyuncuyu al
    uintptr_t ESP::GetLocalPlayer() {
        if (!m_memoryManager) return 0;

        uintptr_t clientModule = m_memoryManager->GetModuleBaseAddress("client.dll");
        if (clientModule == 0) return 0;

        // JSON'dan dwLocalPlayerPawn offset'ini kullan
        return m_memoryManager->Read<uintptr_t>(clientModule + m_memoryManager->GetOffset("dwLocalPlayerPawn"));
    }

    // View Matrix'i al (D�nya koordinatlar�n� ekran koordinatlar�na d�n��t�rmek i�in)
    bool ESP::GetViewMatrix(ViewMatrix& matrix) {
        if (!m_memoryManager) return false;

        uintptr_t clientModule = m_memoryManager->GetModuleBaseAddress("client.dll");
        if (clientModule == 0) return false;

        // JSON'dan dwViewMatrix offset'ini kullan
        uintptr_t viewMatrixPtr = clientModule + m_memoryManager->GetOffset("dwViewMatrix");
        matrix = m_memoryManager->Read<ViewMatrix>(viewMatrixPtr);

        return true;
    }

    // ESP �zelliklerini �iz
    void ESP::Render(ImDrawList* drawList, int screenWidth, int screenHeight) {
        if (!m_enabled || !m_memoryManager) return;

        uintptr_t localPlayer = GetLocalPlayer();
        if (localPlayer == 0) return;

        int localTeam = GetPlayerTeam(localPlayer);
        Vector3 localPos = GetPlayerPosition(localPlayer);

        ViewMatrix viewMatrix;
        if (!GetViewMatrix(viewMatrix)) return;

        // T�m oyuncular� tara ve �iz
        for (int i = 1; i < 64; i++) {
            uintptr_t entity = GetPlayerEntity(i);
            if (entity == 0 || entity == localPlayer) continue;

            int health = GetPlayerHealth(entity);
            if (health <= 0 || health > 100) continue;

            int team = GetPlayerTeam(entity);
            if (!m_teamESP && team == localTeam) continue;

            Vector3 origin = GetPlayerPosition(entity);
            Vector3 head = origin; head.z += 75.0f; // Yakla��k olarak kafa pozisyonu

            Vector2 originScreen, headScreen;
            if (WorldToScreen(origin, originScreen, viewMatrix, screenWidth, screenHeight) &&
                WorldToScreen(head, headScreen, viewMatrix, screenWidth, screenHeight)) {

                // Kutu y�ksekli�i ve geni�li�i
                float height = originScreen.y - headScreen.y;
                float width = height / 2.0f;

                // ESP rengi (tak�ma g�re)
                ImColor espColor = (team == localTeam) ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255);

                // Kutu �iz
                if (m_boxESP) {
                    drawList->AddRect(
                        ImVec2(headScreen.x - width / 2, headScreen.y),
                        ImVec2(headScreen.x + width / 2, originScreen.y),
                        espColor, 0.0f, 0, 2.0f);
                }

                // �sim �iz
                if (m_nameESP) {
                    std::string name = GetPlayerName(entity);
                    drawList->AddText(
                        ImVec2(headScreen.x - width / 2, headScreen.y - 15),
                        espColor, name.c_str());
                }

                // Sa�l�k �iz
                if (m_healthESP) {
                    float healthBarHeight = height * (health / 100.0f);
                    drawList->AddRectFilled(
                        ImVec2(headScreen.x - width / 2 - 8, headScreen.y),
                        ImVec2(headScreen.x - width / 2 - 4, headScreen.y + height),
                        ImColor(0, 0, 0, 180));
                    drawList->AddRectFilled(
                        ImVec2(headScreen.x - width / 2 - 8, headScreen.y + height - healthBarHeight),
                        ImVec2(headScreen.x - width / 2 - 4, headScreen.y + height),
                        ImColor(0, 255, 0, 255));
                }

                // Mesafe �iz
                if (m_distanceESP) {
                    float distance = GetDistance(localPos, origin);
                    std::string distText = std::to_string(static_cast<int>(distance)) + "m";
                    drawList->AddText(
                        ImVec2(originScreen.x + width / 2, originScreen.y),
                        espColor, distText.c_str());
                }

                // Kafa noktas� �iz
                if (m_headpointESP) {
                    drawList->AddCircle(
                        ImVec2(headScreen.x, headScreen.y),
                        3.0f, espColor, 12, 2.0f);
                }
            }
        }
    }

    // D�nya koordinatlar�n� ekran koordinatlar�na d�n��t�r
    bool ESP::WorldToScreen(const Vector3& pos, Vector2& screen, const ViewMatrix& vm, int screenWidth, int screenHeight) {
        float w = vm.m[3][0] * pos.x + vm.m[3][1] * pos.y + vm.m[3][2] * pos.z + vm.m[3][3];

        if (w < 0.001f)
            return false;

        float invw = 1.0f / w;

        screen.x = (vm.m[0][0] * pos.x + vm.m[0][1] * pos.y + vm.m[0][2] * pos.z + vm.m[0][3]) * invw;
        screen.y = (vm.m[1][0] * pos.x + vm.m[1][1] * pos.y + vm.m[1][2] * pos.z + vm.m[1][3]) * invw;

        // Normalize koordinatlar� (0-1) ekran koordinatlar�na d�n��t�r
        float x = screenWidth / 2.0f;
        float y = screenHeight / 2.0f;

        screen.x = x + (screen.x * x);
        screen.y = y - (screen.y * y);

        return true;
    }

    // �ki nokta aras�ndaki mesafeyi hesapla
    float ESP::GetDistance(const Vector3& p1, const Vector3& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dz = p2.z - p1.z;
        return sqrt(dx * dx + dy * dy + dz * dz) / 100.0f; // Oyun birimini metreye d�n��t�rme
    }

} // namespace core