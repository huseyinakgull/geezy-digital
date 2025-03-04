#pragma once

// GameInterface s�n�f�n� tam olarak tan�mlamak i�in header'� dahil edin
#include "game_interface.hpp"

// Windows tipi tan�mlar� i�in
#include <windows.h>
// DirectX tipleri i�in
#include <d3d10_1.h>

// Imgui ba�l���n� do�rudan dahil edin
#include "../memory-external/imgui/imgui.h"

namespace ui {
    class Menu {
    public:
        Menu();
        ~Menu();

        // Initialize ImGui
        bool Initialize(void* window, void* device);

        // Shutdown ImGui
        void Shutdown();

        // Start a new frame
        void NewFrame();

        // Render menu
        void Render(bool visible, game::GameInterface* gameInterface);

        // Render ImGui data
        void RenderDrawData();

        // Set menu visibility
        void SetVisible(bool visible) { m_isVisible = visible; }

        // Get menu visibility
        bool IsVisible() const { return m_isVisible; }

        // Toggle menu visibility
        void ToggleVisibility() { m_isVisible = !m_isVisible; }

    private:
        bool m_isVisible;
        bool m_isInitialized;

        // Render the main menu window
        void RenderMainMenu(game::GameInterface* gameInterface);

        // Initialize ImGui style
        void InitializeStyle();

        // Tab render functions
        void RenderSettingsTab();
        void RenderVisualsTab();
        void RenderInfoTab(game::GameInterface* gameInterface);

        // Modern UI helper functions
        bool CustomCheckbox(const char* label, bool* v);
        bool CustomButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
        bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
        bool CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
        bool CustomCombo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
        void CustomSeparator();
        void CustomGroupBox(const char* name, const ImVec2& size_arg = ImVec2(0, 0));
        void EndGroupBox();
    };

} // namespace ui