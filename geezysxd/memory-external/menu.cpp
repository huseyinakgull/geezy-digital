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
        , m_activeTab(Tab::SETTINGS)
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

    bool Menu::CustomTabButton(const char* label, bool active, const ImVec2& size_arg) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.20f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.20f, 0.20f, 1.00f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.10f, 0.10f, 0.80f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.15f, 0.15f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.20f, 0.20f, 1.00f));
        }

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
        ImGui::SetNextWindowSize(ImVec2(650, 450), ImGuiCond_Once);

        // Configure window flags - Remove title bar and collapse button
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoTitleBar; // Remove title bar

        // Begin the window
        if (!ImGui::Begin("##GeezyDigitalCS2Menu", &m_isVisible, window_flags)) {
            ImGui::End();
            return;
        }

        // Custom header with logo and drag area
        ImGui::BeginGroup();

        // Logo and Title
        ImGui::SetCursorPosX(10);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.1f, 1.0f));
        ImGui::Text("GEEZY.DIGITAL");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 40);
        ImGui::Text("v1.0");

        ImGui::EndGroup();

        // Make the entire header area draggable
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##draggable_area", ImVec2(ImGui::GetWindowWidth(), 30));
        if (ImGui::IsItemActive()) {
            ImGui::SetWindowPos(ImVec2(
                ImGui::GetIO().MousePos.x - ImGui::GetIO().MouseDelta.x,
                ImGui::GetIO().MousePos.y - ImGui::GetIO().MouseDelta.y
            ));
        }

        CustomSeparator();

        // Connection status
        bool isConnected = gameInterface != nullptr && gameInterface->IsConnected();
        ImGui::TextColored(isConnected ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
            "Status: %s", isConnected ? "Connected" : "Not Connected");

        if (isConnected) {
            ImGui::SameLine();
            ImGui::Text(" | PID: %u", gameInterface->GetProcessId());
        }

        ImGui::Spacing();

        // Custom tab buttons instead of ImGui::TabBar
        float windowWidth = ImGui::GetWindowWidth();
        float tabWidth = (windowWidth - 20) / 5; // 5 tabs with 10px padding on each side

        ImGui::SetCursorPosX(10);

        // Settings Tab
        if (CustomTabButton("Settings", m_activeTab == Tab::SETTINGS, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::SETTINGS;
        }

        ImGui::SameLine(0, 0);

        // Visuals Tab
        if (CustomTabButton("Visuals", m_activeTab == Tab::VISUALS, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::VISUALS;
        }

        ImGui::SameLine(0, 0);

        // Aimbot Tab
        if (CustomTabButton("Aimbot", m_activeTab == Tab::AIMBOT, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::AIMBOT;
        }

        ImGui::SameLine(0, 0);

        // Misc Tab
        if (CustomTabButton("Misc", m_activeTab == Tab::MISC, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::MISC;
        }

        ImGui::SameLine(0, 0);

        // Info Tab
        if (CustomTabButton("Info", m_activeTab == Tab::INFO, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::INFO;
        }

        ImGui::Spacing();
        CustomSeparator();
        ImGui::Spacing();

        // Content area for the active tab
        ImGui::BeginGroup();

        // Render the active tab content
        switch (m_activeTab) {
        case Tab::SETTINGS:
            RenderSettingsTab();
            break;
        case Tab::VISUALS:
            RenderVisualsTab();
            break;
        case Tab::AIMBOT:
            RenderAimbotTab();
            break;
        case Tab::MISC:
            RenderMiscTab();
            break;
        case Tab::INFO:
            RenderInfoTab(gameInterface);
            break;
        }

        ImGui::EndGroup();

        // Bottom info area
        CustomSeparator();

        // Bottom buttons
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 45);

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
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float columnWidth = (contentWidth - 10) / 2; // Two columns with 10px spacing

        // Left Column
        ImGui::BeginGroup();

        // General Settings
        CustomGroupBox("General Settings", ImVec2(columnWidth, 120));
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

        // Theme Settings
        CustomGroupBox("Theme Settings", ImVec2(columnWidth, 130));
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

        ImGui::EndGroup();

        // Right Column
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Hotkey Settings
        CustomGroupBox("Hotkey Settings", ImVec2(columnWidth, 120));
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

        // Performance Settings
        CustomGroupBox("Performance Settings", ImVec2(columnWidth, 130));
        {
            static bool lowEndMode = false;
            CustomCheckbox("Low-End Mode", &lowEndMode);

            static int maxFps = 120;
            ImGui::Text("Maximum FPS");
            CustomSliderInt("##MaxFPS", &maxFps, 30, 300, "%d");

            static bool threadedRendering = true;
            CustomCheckbox("Threaded Rendering", &threadedRendering);
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    void Menu::RenderVisualsTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float columnWidth = (contentWidth - 10) / 2; // Two columns with 10px spacing

        // Left Column
        ImGui::BeginGroup();

        // ESP Settings
        CustomGroupBox("ESP Settings", ImVec2(columnWidth, 260));
        {
            static bool espEnabled = true;
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

            static bool showWeapon = true;
            CustomCheckbox("Show Weapon", &showWeapon);

            ImGui::Text("Maximum Distance");
            static float maxDistance = 1000.0f;
            CustomSliderFloat("##MaxDistance", &maxDistance, 100.0f, 3000.0f, "%.0f");

            ImGui::Text("Box Thickness");
            static int boxThickness = 2;
            CustomSliderInt("##BoxThickness", &boxThickness, 1, 5, "%d");

            ImGui::Text("Box Color");
            static float boxColor[3] = { 0.9f, 0.2f, 0.2f };
            ImGui::ColorEdit3("##BoxColor", boxColor);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Right Column
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Glow Settings
        CustomGroupBox("Glow Settings", ImVec2(columnWidth, 150));
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

        ImGui::Spacing();

        // Chams Settings
        CustomGroupBox("Chams Settings", ImVec2(columnWidth, 100));
        {
            static bool chamsEnabled = false;
            CustomCheckbox("Chams Enabled", &chamsEnabled);

            ImGui::BeginDisabled(!chamsEnabled);

            static int chamsType = 0;
            const char* chamsTypes[] = { "Flat", "Textured", "Wireframe", "Metallic" };
            CustomCombo("##ChamsType", &chamsType, chamsTypes, IM_ARRAYSIZE(chamsTypes));

            ImGui::Text("Visible Color");
            static float visibleColor[3] = { 0.2f, 0.8f, 0.2f };
            ImGui::ColorEdit3("##VisibleColor", visibleColor);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    void Menu::RenderAimbotTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float columnWidth = (contentWidth - 10) / 2; // Two columns with 10px spacing

        // Left Column
        ImGui::BeginGroup();

        // Aimbot Settings
        CustomGroupBox("Aimbot Settings", ImVec2(columnWidth, 260));
        {
            static bool aimbotEnabled = false;
            CustomCheckbox("Aimbot Enabled", &aimbotEnabled);

            ImGui::BeginDisabled(!aimbotEnabled);

            static int aimbotKey = 2;
            const char* keys[] = { "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "Shift", "Ctrl", "Alt" };
            ImGui::Text("Aimbot Key");
            CustomCombo("##AimbotKey", &aimbotKey, keys, IM_ARRAYSIZE(keys));

            static int aimbotBone = 0;
            const char* bones[] = { "Head", "Neck", "Chest", "Stomach" };
            ImGui::Text("Target Bone");
            CustomCombo("##TargetBone", &aimbotBone, bones, IM_ARRAYSIZE(bones));

            ImGui::Text("Aimbot FOV");
            static float aimbotFov = 5.0f;
            CustomSliderFloat("##AimbotFOV", &aimbotFov, 1.0f, 30.0f, "%.1f°");

            ImGui::Text("Smoothing");
            static float aimbotSmoothing = 1.0f;
            CustomSliderFloat("##AimbotSmoothing", &aimbotSmoothing, 1.0f, 10.0f, "%.1f");

            static bool visibilityCheck = true;
            CustomCheckbox("Visibility Check", &visibilityCheck);

            static bool teamCheck = true;
            CustomCheckbox("Team Check", &teamCheck);

            static bool drawFov = true;
            CustomCheckbox("Draw FOV Circle", &drawFov);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Right Column
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Triggerbot Settings
        CustomGroupBox("Triggerbot Settings", ImVec2(columnWidth, 150));
        {
            static bool triggerbotEnabled = false;
            CustomCheckbox("Triggerbot Enabled", &triggerbotEnabled);

            ImGui::BeginDisabled(!triggerbotEnabled);

            static int triggerbotKey = 4;
            const char* keys[] = { "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "Shift", "Ctrl", "Alt" };
            ImGui::Text("Triggerbot Key");
            CustomCombo("##TriggerbotKey", &triggerbotKey, keys, IM_ARRAYSIZE(keys));

            ImGui::Text("Delay (ms)");
            static int triggerbotDelay = 100;
            CustomSliderInt("##TriggerbotDelay", &triggerbotDelay, 0, 500, "%d ms");

            static bool visibilityCheck = true;
            CustomCheckbox("Visibility Check", &visibilityCheck);

            static bool teamCheck = true;
            CustomCheckbox("Team Check", &teamCheck);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // RCS Settings
        CustomGroupBox("Recoil Control Settings", ImVec2(columnWidth, 100));
        {
            static bool rcsEnabled = false;
            CustomCheckbox("RCS Enabled", &rcsEnabled);

            ImGui::BeginDisabled(!rcsEnabled);

            ImGui::Text("X Strength");
            static float rcsX = 1.0f;
            CustomSliderFloat("##RCSX", &rcsX, 0.0f, 2.0f, "%.1f");

            ImGui::Text("Y Strength");
            static float rcsY = 1.0f;
            CustomSliderFloat("##RCSY", &rcsY, 0.0f, 2.0f, "%.1f");

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    void Menu::RenderMiscTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float columnWidth = (contentWidth - 10) / 2; // Two columns with 10px spacing

        // Left Column
        ImGui::BeginGroup();

        // Miscellaneous Features
        CustomGroupBox("Miscellaneous Features", ImVec2(columnWidth, 260));
        {
            static bool bhopEnabled = false;
            CustomCheckbox("Bunny Hop", &bhopEnabled);

            static bool autoStrafe = false;
            CustomCheckbox("Auto Strafe", &autoStrafe);

            static bool noFlash = false;
            CustomCheckbox("No Flash", &noFlash);

            ImGui::Text("Flash Reduction");
            static float flashReduction = 50.0f;
            CustomSliderFloat("##FlashReduction", &flashReduction, 0.0f, 100.0f, "%.0f%%");

            static bool radarHack = false;
            CustomCheckbox("Radar Hack", &radarHack);

            static bool nightMode = false;
            CustomCheckbox("Night Mode", &nightMode);

            ImGui::Text("Night Brightness");
            static float nightBrightness = 0.3f;
            CustomSliderFloat("##NightBrightness", &nightBrightness, 0.1f, 1.0f, "%.1f");

            static bool fovChanger = false;
            CustomCheckbox("FOV Changer", &fovChanger);

            ImGui::Text("Custom FOV");
            static int customFov = 90;
            CustomSliderInt("##CustomFOV", &customFov, 70, 120, "%d°");
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Right Column
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Skin Changer
        CustomGroupBox("Skin Changer", ImVec2(columnWidth, 150));
        {
            static bool skinChangerEnabled = false;
            CustomCheckbox("Skin Changer Enabled", &skinChangerEnabled);

            ImGui::BeginDisabled(!skinChangerEnabled);

            static int weaponSelect = 0;
            const char* weapons[] = { "AK-47", "M4A4", "M4A1-S", "AWP", "Desert Eagle" };
            ImGui::Text("Weapon");
            CustomCombo("##WeaponSelect", &weaponSelect, weapons, IM_ARRAYSIZE(weapons));

            static int skinSelect = 0;
            const char* skins[] = { "Default", "Asiimov", "Hyper Beast", "Neo-Noir", "Dragon Lore" };
            ImGui::Text("Skin");
            CustomCombo("##SkinSelect", &skinSelect, skins, IM_ARRAYSIZE(skins));

            ImGui::Text("Wear");
            static float wear = 0.0f;
            CustomSliderFloat("##Wear", &wear, 0.0f, 1.0f, "%.2f");

            static bool statTrak = false;
            CustomCheckbox("StatTrak", &statTrak);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Crosshair
        CustomGroupBox("Custom Crosshair", ImVec2(columnWidth, 100));
        {
            static bool customCrosshair = false;
            CustomCheckbox("Custom Crosshair", &customCrosshair);

            ImGui::BeginDisabled(!customCrosshair);

            static int crosshairType = 0;
            const char* crosshairTypes[] = { "Cross", "Circle", "Dot", "T-Shape" };
            ImGui::Text("Type");
            CustomCombo("##CrosshairType", &crosshairType, crosshairTypes, IM_ARRAYSIZE(crosshairTypes));

            ImGui::Text("Size");
            static int crosshairSize = 5;
            CustomSliderInt("##CrosshairSize", &crosshairSize, 1, 20, "%d");

            ImGui::Text("Color");
            static float crosshairColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            ImGui::ColorEdit4("##CrosshairColor", crosshairColor);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    void Menu::RenderInfoTab(game::GameInterface* gameInterface) {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float columnWidth = (contentWidth - 10) / 2; // Two columns with 10px spacing

        // Left Column
        ImGui::BeginGroup();

        // Application Info
        CustomGroupBox("Application Info", ImVec2(columnWidth, 120));
        {
            ImGui::BulletText("Version: 1.0");
            ImGui::BulletText("Build Date: %s", __DATE__);
            ImGui::BulletText("Build: Debug");
            ImGui::BulletText("Developer: GeezyDigital");
            ImGui::BulletText("License: Premium");
        }
        EndGroupBox();

        ImGui::Spacing();

        // Performance Monitoring
        CustomGroupBox("Performance Monitoring", ImVec2(columnWidth, 150));
        {
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
            ImGui::BulletText("Memory Usage: 32.5 MB");

            // FPS chart
            ImGui::PlotLines("Frame Time (ms)", frameTimes, IM_ARRAYSIZE(frameTimes), frameTimeIdx, NULL, 0.0f, 50.0f, ImVec2(0, 80));
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Right Column
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // System Info
        CustomGroupBox("System Info", ImVec2(columnWidth, 120));
        {
            if (gameInterface && gameInterface->IsConnected()) {
                ImGui::BulletText("Process ID: %u", gameInterface->GetProcessId());
                ImGui::BulletText("Client Module: 0x%llX", gameInterface->GetClientModuleBase());
                ImGui::BulletText("Game Version: 1.39.2.5");
                ImGui::BulletText("Handle Access: Full");
            }
            else {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "No game connection!");
                ImGui::BulletText("Launch CS2 to connect");
            }
        }
        EndGroupBox();

        ImGui::Spacing();

        // Contact & Support
        CustomGroupBox("Contact", ImVec2(columnWidth, 150));
        {
            ImGui::BulletText("Discord: geezydigital");
            ImGui::BulletText("Web: geezy.digital");
            ImGui::BulletText("Support Email: support@geezy.digital");

            ImGui::Spacing();
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 250) / 2);

            if (CustomButton("Get Support", ImVec2(120, 25))) {
                // Open support page
            }

            ImGui::SameLine();
            if (CustomButton("Check Update", ImVec2(120, 25))) {
                // Update function
            }

            ImGui::Spacing();
            if (CustomButton("Visit Forum", ImVec2(240, 25))) {
                // Visit forum
            }
        }
        EndGroupBox();

        ImGui::EndGroup();
    }