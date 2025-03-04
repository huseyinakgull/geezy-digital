#include "menu.hpp"
#include "../memory-external/imgui/imgui.h"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
#include "../memory-external/imgui/backends/imgui_impl_dx10.h"
#include <iostream>
#include "logger.hpp"

namespace ui {

    Menu::Menu()
        : m_isVisible(true)
        , m_isInitialized(false)
    {
    }

    Menu::~Menu() {
        Shutdown();
    }

    bool Menu::Initialize(void* window, void* device) {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Disable ImGui's ini file for configuration
        io.IniFilename = NULL;

        // Initialize style
        InitializeStyle();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(static_cast<HWND>(window))) {
            utils::LogError("Failed to initialize ImGui Win32 backend");
            return false;
        }

        if (!ImGui_ImplDX10_Init(static_cast<ID3D10Device*>(device))) {
            utils::LogError("Failed to initialize ImGui DirectX 10 backend");
            return false;
        }

        m_isInitialized = true;
        utils::LogSuccess("ImGui initialized successfully");
        return true;
    }

    void Menu::Shutdown() {
        if (m_isInitialized) {
            ImGui_ImplDX10_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            m_isInitialized = false;
        }
    }

    void Menu::InitializeStyle() {
        // Setup ImGui style
        ImGui::StyleColorsDark();

        // Customize style for a more modern, flat look
        ImGuiStyle& style = ImGui::GetStyle();

        // Edge & corners
        style.WindowRounding = 0.0f;
        style.ChildRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.PopupRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;

        // Alignment
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

        // Sizes and spacing - more compact
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(5.0f, 3.5f);
        style.ItemSpacing = ImVec2(6.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 12.0f;
        style.ScrollbarSize = 10.0f;
        style.GrabMinSize = 8.0f;

        // Borders
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        // Other settings
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;

        // Colors - keeping the red accent theme
        ImVec4* colors = style.Colors;

        // Background colors - dark with slight transparency
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.97f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.60f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);

        // Headers, tabs, and title bar
        colors[ImGuiCol_Header] = ImVec4(0.50f, 0.10f, 0.10f, 0.35f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.15f, 0.15f, 0.45f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.70f, 0.20f, 0.20f, 0.55f);

        colors[ImGuiCol_Tab] = ImVec4(0.40f, 0.08f, 0.08f, 0.80f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.60f, 0.12f, 0.12f, 0.85f);
        colors[ImGuiCol_TabActive] = ImVec4(0.75f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.05f, 0.05f, 0.70f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.40f, 0.08f, 0.08f, 0.75f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.35f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.50f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25f, 0.05f, 0.05f, 0.75f);

        // Interactive elements
        colors[ImGuiCol_Button] = ImVec4(0.40f, 0.10f, 0.10f, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.15f, 0.15f, 0.85f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.20f, 0.20f, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);

        colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);

        // Frame backgrounds (for checkbox, radio button, etc.)
        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 0.60f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.20f, 0.20f, 0.70f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.25f, 0.25f, 0.80f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.10f, 0.10f, 0.40f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.15f, 0.15f, 0.60f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.20f, 0.20f, 0.80f);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // Separators, borders
        colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.15f, 0.15f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.20f, 0.20f, 0.70f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.07f, 0.07f, 0.50f);
    }

    void Menu::NewFrame() {
        if (!m_isInitialized)
            return;

        ImGui_ImplDX10_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Menu::Render(bool visible, game::GameInterface* gameInterface) {
        // Store visibility state
        m_isVisible = visible;

        if (m_isVisible) {
            RenderMainMenu(gameInterface);
        }
    }

    void Menu::RenderDrawData() {
        if (!m_isInitialized)
            return;

        ImGui::Render();
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    }

    // Custom UI elements implementation
// Özel checkbox implementasyonu - ImGui'nin iç API'lerini kullanmadan
    bool Menu::CustomCheckbox(const char* label, bool* v) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.20f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));

        // Checkbox stilini daha modern görünüme yaklaþtýrma
        float originalRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 2.0f;

        bool result = ImGui::Checkbox(label, v);

        // Orijinal deðere geri dön
        ImGui::GetStyle().FrameRounding = originalRounding;
        ImGui::PopStyleColor(4);

        return result;
    }
    bool Menu::CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.90f, 0.30f, 0.30f, 1.0f));

        bool result = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);

        ImGui::PopStyleColor(3);
        return result;
    }

    bool Menu::CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.90f, 0.30f, 0.30f, 1.0f));

        bool result = ImGui::SliderInt(label, v, v_min, v_max, format, flags);

        ImGui::PopStyleColor(3);
        return result;
    }

    bool Menu::CustomButton(const char* label, const ImVec2& size_arg) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.10f, 0.10f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.15f, 0.15f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.20f, 0.20f, 1.00f));

        bool result = ImGui::Button(label, size_arg);

        ImGui::PopStyleColor(3);
        return result;
    }

    bool Menu::CustomCombo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.12f, 0.97f));

        bool result = ImGui::Combo(label, current_item, items, items_count, height_in_items);

        ImGui::PopStyleColor(2);
        return result;
    }

    void Menu::CustomSeparator() {
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.60f, 0.15f, 0.15f, 0.50f));
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    void Menu::CustomGroupBox(const char* name, const ImVec2& size_arg) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.07f, 0.07f, 0.50f));

        ImGui::BeginChild(name, size_arg, true);

        // Group title
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
        ImGui::Text("%s", name);
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }

    void Menu::EndGroupBox() {
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
    }

    void Menu::RenderMainMenu(game::GameInterface* gameInterface) {
        // Get display size
        ImGuiIO& io = ImGui::GetIO();

        // Position the menu in the center of the screen
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(620, 450), ImGuiCond_Once);

        // Configure window flags
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings;

        // Begin the window
        if (!ImGui::Begin("Geezy.Digital CS2 Menu", &m_isVisible, window_flags)) {
            ImGui::End();
            return;
        }

        // Header with logo
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.1f, 1.0f));
        ImGui::Text("GEEZY.DIGITAL");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 40);
        ImGui::Text("v1.0");
        ImGui::EndGroup();

        CustomSeparator();

        // Connection status - using English characters with colored text
        bool isConnected = gameInterface != nullptr && gameInterface->IsConnected();
        ImGui::TextColored(isConnected ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
            "Status: %s", isConnected ? "Connected" : "Not Connected");

        if (isConnected) {
            ImGui::SameLine();
            ImGui::Text(" | PID: %u", gameInterface->GetProcessId());
        }

        // Modern Tab Bar
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.30f, 0.07f, 0.07f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.40f, 0.10f, 0.10f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.70f, 0.20f, 0.20f, 1.00f));

        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Settings")) {
                RenderSettingsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Visual Helpers")) {
                RenderVisualsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Info")) {
                RenderInfoTab(gameInterface);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleColor(3);

        // Bottom info area
        CustomSeparator();

        // Bottom buttons
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 55);

        if (CustomButton("Reconnect", ImVec2(130, 30))) {
            // Reconnect function
        }

        ImGui::SameLine();
        if (CustomButton("Close", ImVec2(130, 30))) {
            // Close function
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(!isConnected);
        if (CustomButton("Game Window", ImVec2(130, 30))) {
            // Game window function
        }
        ImGui::EndDisabled();

        // FPS info
        ImGui::SetCursorPos(ImVec2(10, ImGui::GetWindowHeight() - 25));
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        // Hotkey info
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Text("E: Menu | END: Exit");

        ImGui::End();
    }

    void Menu::RenderSettingsTab() {
        ImGui::BeginChild("SettingsChild", ImVec2(0, 0), false);

        // General Settings
        CustomGroupBox("General Settings", ImVec2(ImGui::GetContentRegionAvail().x, 100));
        {
            static bool vsync = true;
            CustomCheckbox("VSync", &vsync);

            static int refreshRate = 60;
            ImGui::TextUnformatted("Refresh Rate");
            CustomSliderInt("##RefreshRate", &refreshRate, 30, 240, "%d");

            static bool overlay = true;
            CustomCheckbox("Overlay Enabled", &overlay);
        }
        EndGroupBox();

        ImGui::Spacing();

        // Hotkey Settings
        CustomGroupBox("Hotkey Settings", ImVec2(ImGui::GetContentRegionAvail().x, 100));
        {
            const char* keys[] = { "F1", "F2", "F3", "F4", "E", "R", "T", "X", "Z", "TAB" };
            static int menuKey = 4; // "E" default

            ImGui::Text("Menu Key");
            CustomCombo("##MenuKey", &menuKey, keys, IM_ARRAYSIZE(keys));

            static int exitKey = 0;
            ImGui::Text("Exit Key");
            CustomCombo("##ExitKey", &exitKey, keys, IM_ARRAYSIZE(keys));
        }
        EndGroupBox();

        ImGui::Spacing();

        // Theme Settings
        CustomGroupBox("Theme Settings", ImVec2(ImGui::GetContentRegionAvail().x, 120));
        {
            static float alpha = 1.0f;
            ImGui::Text("Window Opacity");
            CustomSliderFloat("##WindowOpacity", &alpha, 0.5f, 1.0f, "%.2f");

            static float r = 0.7f, g = 0.1f, b = 0.1f;
            ImGui::Text("Accent Color");
            ImGui::ColorEdit3("##AccentColor", (float*)&r);

            if (CustomButton("Reset Colors", ImVec2(120, 25))) {
                r = 0.7f; g = 0.1f; b = 0.1f;
                alpha = 1.0f;
            }
        }
        EndGroupBox();

        ImGui::EndChild();
    }

    void Menu::RenderVisualsTab() {
        ImGui::BeginChild("VisualsChild", ImVec2(0, 0), false);

        // ESP Settings
        CustomGroupBox("ESP Settings", ImVec2(ImGui::GetContentRegionAvail().x, 180));
        {
            static bool espEnabled = false;
            CustomCheckbox("ESP Enabled", &espEnabled);

            ImGui::BeginDisabled(!espEnabled);

            static bool showBox = true;
            CustomCheckbox("Show Box", &showBox);

            static bool showHealth = true;
            CustomCheckbox("Show Health", &showHealth);

            static bool showName = true;
            CustomCheckbox("Show Name", &showName);

            static bool showDistance = false;
            CustomCheckbox("Show Distance", &showDistance);

            ImGui::Text("Maximum Distance");
            static float maxDistance = 1000.0f;
            CustomSliderFloat("##MaxDistance", &maxDistance, 100.0f, 3000.0f, "%.0f");

            ImGui::Text("Box Thickness");
            static int boxThickness = 2;
            CustomSliderInt("##BoxThickness", &boxThickness, 1, 5, "%d");

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Glow Settings
        CustomGroupBox("Glow Settings", ImVec2(ImGui::GetContentRegionAvail().x, 160));
        {
            static bool glowEnabled = false;
            CustomCheckbox("Glow Enabled", &glowEnabled);

            ImGui::BeginDisabled(!glowEnabled);

            ImGui::Text("Glow Intensity");
            static float glowIntensity = 0.5f;
            CustomSliderFloat("##GlowIntensity", &glowIntensity, 0.1f, 1.0f, "%.2f");

            ImGui::Text("Glow Style");
            static int glowStyle = 0;
            const char* glowStyles[] = { "Normal", "Pulsing", "Outer Edge", "Inner Edge" };
            CustomCombo("##GlowStyle", &glowStyle, glowStyles, IM_ARRAYSIZE(glowStyles));

            ImGui::Text("Team Color");
            static float teamColor[3] = { 0.2f, 0.5f, 0.9f };
            ImGui::ColorEdit3("##TeamColor", teamColor);

            ImGui::Text("Enemy Color");
            static float enemyColor[3] = { 0.9f, 0.2f, 0.2f };
            ImGui::ColorEdit3("##EnemyColor", enemyColor);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndChild();
    }

    void Menu::RenderInfoTab(game::GameInterface* gameInterface) {
        ImGui::BeginChild("InfoChild", ImVec2(0, 0), false);

        // Application Info
        CustomGroupBox("Application Info", ImVec2(ImGui::GetContentRegionAvail().x, 100));
        {
            ImGui::BulletText("Version: 1.0");
            ImGui::BulletText("Build Date: %s", __DATE__);
            ImGui::BulletText("Build: Debug");
        }
        EndGroupBox();

        ImGui::Spacing();

        // System Info
        CustomGroupBox("System Info", ImVec2(ImGui::GetContentRegionAvail().x, 200));
        {
            if (gameInterface && gameInterface->IsConnected()) {
                ImGui::BulletText("Process ID: %u", gameInterface->GetProcessId());
                ImGui::BulletText("Client Module: 0x%llX", gameInterface->GetClientModuleBase());

                // Performance statistics
                static float frameTimes[100] = {};
                static int frameTimeIdx = 0;

                frameTimes[frameTimeIdx] = 1000.0f / ImGui::GetIO().Framerate;
                frameTimeIdx = (frameTimeIdx + 1) % IM_ARRAYSIZE(frameTimes);

                float avgFrameTime = 0;
                for (int i = 0; i < IM_ARRAYSIZE(frameTimes); i++) avgFrameTime += frameTimes[i];
                avgFrameTime /= IM_ARRAYSIZE(frameTimes);

                ImGui::BulletText("Current FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::BulletText("Average Frame Time: %.2f ms", avgFrameTime);

                // FPS chart
                ImGui::PlotLines("Frame Time (ms)", frameTimes, IM_ARRAYSIZE(frameTimes), frameTimeIdx, NULL, 0.0f, 50.0f, ImVec2(0, 80));
            }
            else {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "No game connection!");
            }
        }
        EndGroupBox();

        ImGui::Spacing();

        // Contact info
        CustomGroupBox("Contact", ImVec2(ImGui::GetContentRegionAvail().x, 100));
        {
            ImGui::BulletText("Discord: geezydigital");
            ImGui::BulletText("Web: geezy.digital");

            ImGui::Spacing();
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 250) / 2);

            if (CustomButton("Get Support", ImVec2(120, 25))) {
                // Open support page
            }

            ImGui::SameLine();
            if (CustomButton("Update", ImVec2(120, 25))) {
                // Update function
            }
        }
        EndGroupBox();

        ImGui::EndChild();
    }

} // namespace ui