#pragma once

#include <Windows.h>
#include "memory_manager.hpp"
#include "../memory-external/imgui/imgui.h"

namespace core {

    // Vector3 yapýsý - 3D koordinatlar için
    struct Vector3 {
        float x, y, z;
    };

    // Vector2 yapýsý - 2D koordinatlar için
    struct Vector2 {
        float x, y;
    };

    // ViewMatrix - Dünya koordinatlarýný ekran koordinatlarýna dönüþtürmek için 4x4 matris
    struct ViewMatrix {
        float m[4][4];
    };

    class ESP {
    public:
        ESP(Memory::MemoryManager* memoryManager);

        // ESP özelliklerini kontrol etmek için ayarlayýcýlar
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        void SetTeamESP(bool enabled) { m_teamESP = enabled; }
        void SetBoxESP(bool enabled) { m_boxESP = enabled; }
        void SetNameESP(bool enabled) { m_nameESP = enabled; }
        void SetHealthESP(bool enabled) { m_healthESP = enabled; }
        void SetDistanceESP(bool enabled) { m_distanceESP = enabled; }
        void SetHeadpointESP(bool enabled) { m_headpointESP = enabled; }

        // ESP özelliklerinin durumunu kontrol etmek için eriþimciler
        bool IsEnabled() const { return m_enabled; }
        bool IsTeamESP() const { return m_teamESP; }
        bool IsBoxESP() const { return m_boxESP; }
        bool IsNameESP() const { return m_nameESP; }
        bool IsHealthESP() const { return m_healthESP; }
        bool IsDistanceESP() const { return m_distanceESP; }
        bool IsHeadpointESP() const { return m_headpointESP; }

        // ImGui çizim listesine ESP özelliklerini çiz
        void Render(ImDrawList* drawList, int screenWidth, int screenHeight);

    private:
        Memory::MemoryManager* m_memoryManager;

        // ESP özellikleri
        bool m_enabled;
        bool m_teamESP;
        bool m_boxESP;
        bool m_nameESP;
        bool m_healthESP;
        bool m_distanceESP;
        bool m_headpointESP;

        // Oyuncu varlýðýný al (entity)
        uintptr_t GetPlayerEntity(int index);

        // Oyuncu saðlýðýný al
        int GetPlayerHealth(uintptr_t playerEntity);

        // Oyuncu takýmýný al
        int GetPlayerTeam(uintptr_t playerEntity);

        // Oyuncu pozisyonunu al
        Vector3 GetPlayerPosition(uintptr_t playerEntity);

        // Oyuncu ismini al
        std::string GetPlayerName(uintptr_t playerEntity);

        // Lokal oyuncuyu al
        uintptr_t GetLocalPlayer();

        // View Matrix'i al
        bool GetViewMatrix(ViewMatrix& matrix);

        // 3D dünya koordinatlarýný 2D ekran koordinatlarýna dönüþtür
        bool WorldToScreen(const Vector3& pos, Vector2& screen, const ViewMatrix& vm, int screenWidth, int screenHeight);

        // Ýki 3D nokta arasýndaki mesafeyi hesapla
        float GetDistance(const Vector3& p1, const Vector3& p2);
    };

} // namespace core