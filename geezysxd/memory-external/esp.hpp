#pragma once

#include <Windows.h>
#include "memory_manager.hpp"
#include "../memory-external/imgui/imgui.h"

namespace core {

    // Vector3 yap�s� - 3D koordinatlar i�in
    struct Vector3 {
        float x, y, z;
    };

    // Vector2 yap�s� - 2D koordinatlar i�in
    struct Vector2 {
        float x, y;
    };

    // ViewMatrix - D�nya koordinatlar�n� ekran koordinatlar�na d�n��t�rmek i�in 4x4 matris
    struct ViewMatrix {
        float m[4][4];
    };

    class ESP {
    public:
        ESP(Memory::MemoryManager* memoryManager);

        // ESP �zelliklerini kontrol etmek i�in ayarlay�c�lar
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        void SetTeamESP(bool enabled) { m_teamESP = enabled; }
        void SetBoxESP(bool enabled) { m_boxESP = enabled; }
        void SetNameESP(bool enabled) { m_nameESP = enabled; }
        void SetHealthESP(bool enabled) { m_healthESP = enabled; }
        void SetDistanceESP(bool enabled) { m_distanceESP = enabled; }
        void SetHeadpointESP(bool enabled) { m_headpointESP = enabled; }

        // ESP �zelliklerinin durumunu kontrol etmek i�in eri�imciler
        bool IsEnabled() const { return m_enabled; }
        bool IsTeamESP() const { return m_teamESP; }
        bool IsBoxESP() const { return m_boxESP; }
        bool IsNameESP() const { return m_nameESP; }
        bool IsHealthESP() const { return m_healthESP; }
        bool IsDistanceESP() const { return m_distanceESP; }
        bool IsHeadpointESP() const { return m_headpointESP; }

        // ImGui �izim listesine ESP �zelliklerini �iz
        void Render(ImDrawList* drawList, int screenWidth, int screenHeight);

    private:
        Memory::MemoryManager* m_memoryManager;

        // ESP �zellikleri
        bool m_enabled;
        bool m_teamESP;
        bool m_boxESP;
        bool m_nameESP;
        bool m_healthESP;
        bool m_distanceESP;
        bool m_headpointESP;

        // Oyuncu varl���n� al (entity)
        uintptr_t GetPlayerEntity(int index);

        // Oyuncu sa�l���n� al
        int GetPlayerHealth(uintptr_t playerEntity);

        // Oyuncu tak�m�n� al
        int GetPlayerTeam(uintptr_t playerEntity);

        // Oyuncu pozisyonunu al
        Vector3 GetPlayerPosition(uintptr_t playerEntity);

        // Oyuncu ismini al
        std::string GetPlayerName(uintptr_t playerEntity);

        // Lokal oyuncuyu al
        uintptr_t GetLocalPlayer();

        // View Matrix'i al
        bool GetViewMatrix(ViewMatrix& matrix);

        // 3D d�nya koordinatlar�n� 2D ekran koordinatlar�na d�n��t�r
        bool WorldToScreen(const Vector3& pos, Vector2& screen, const ViewMatrix& vm, int screenWidth, int screenHeight);

        // �ki 3D nokta aras�ndaki mesafeyi hesapla
        float GetDistance(const Vector3& p1, const Vector3& p2);
    };

} // namespace core