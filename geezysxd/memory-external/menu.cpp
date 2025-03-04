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

        // Enable docking if desired
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

        // Customize style to make it more visible
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.Alpha = 1.0f; // Full opacity

        // Increase contrast with brighter colors
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.30f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.39f, 0.39f, 0.70f, 0.60f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.41f, 0.80f, 0.75f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.43f, 0.43f, 0.90f, 0.90f);

        // Increase text contrast
        style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
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

    void Menu::RenderMainMenu(game::GameInterface* gameInterface) {
        // Set display size in case we need to force it
        ImGuiIO& io = ImGui::GetIO();
        // Create a more visible window in the center of the screen
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_Once);
        ImGui::SetNextWindowBgAlpha(0.95f); // Very visible background

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse;

        // Create window with frame and title bar
        if (!ImGui::Begin("product by huseyin.", &m_isVisible, window_flags)) {
            ImGui::End();
            return;
        }

        // Title
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "geezy.digital");
        ImGui::Separator();

        // Game connection status
        bool isConnected = gameInterface != nullptr && gameInterface->IsConnected();
        ImGui::TextColored(isConnected ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
            "Durum: %s", isConnected ? "Baglandi." : "Baglanilamadi.");

        if (isConnected) {
            ImGui::Text("Process ID: %u", gameInterface->GetProcessId());
            ImGui::Text("Client Module Base: 0x%llX", gameInterface->GetClientModuleBase());
        }

        ImGui::Separator();

        // Buttons with clear visual feedback
        ImGui::Separator();

        if (ImGui::Button("Reconnect", ImVec2(160, 30))) {
            // Handled by callback
            utils::LogInfo("Reconnect button clicked");
        }

        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(160, 30))) {
            // Handled by callback
            utils::LogInfo("Exit button clicked");
        }

        ImGui::Separator();

        // Usage instructions
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextWrapped("E: Toggle Menu");
        ImGui::TextWrapped("END: Exit Program");
        ImGui::PopTextWrapPos();

        // Performance info
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::End();
    }

} // namespace ui