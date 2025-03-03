#include "menu.hpp"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
#include "../memory-external/imgui/backends/imgui_impl_dx10.h"
#include <iostream>

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

        // Initialize style
        InitializeStyle();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(static_cast<HWND>(window))) {
            std::cout << "[ERROR] Failed to initialize ImGui Win32 backend" << std::endl;
            return false;
        }

        if (!ImGui_ImplDX10_Init(static_cast<ID3D10Device*>(device))) {
            std::cout << "[ERROR] Failed to initialize ImGui DirectX 10 backend" << std::endl;
            return false;
        }

        m_isInitialized = true;
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

        // Customize style
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;

        // Set custom colors if desired
        // style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.85f);
        // etc.
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
        // Create ImGui main window
        ImGuiIO& io = ImGui::GetIO();

        // Create a small window in the top-left corner
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.85f); // Semi-transparent background

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize;

        // Create window with frame and title bar
        if (!ImGui::Begin("GeezyDigital CS2 Menu", &m_isVisible, window_flags)) {
            ImGui::End();
            return;
        }

        // Title
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "GeezyDigital CS2 Menu");
        ImGui::Separator();

        // Game connection status
        bool isConnected = gameInterface != nullptr && gameInterface->IsConnected();
        ImGui::TextColored(isConnected ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
            "Game Status: %s", isConnected ? "Connected" : "Not Connected");

        if (isConnected) {
            ImGui::Text("Process ID: %u", gameInterface->GetProcessId());
            ImGui::Text("Client Module Base: 0x%llX", gameInterface->GetClientModuleBase());
        }

        ImGui::Separator();

        // Buttons and controls
        if (ImGui::Button("Reconnect", ImVec2(135, 0))) {
            // Handled by callback
        }

        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(135, 0))) {
            // Handled by callback
        }

        ImGui::Separator();

        // Usage instructions
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextWrapped("INSERT: Toggle Menu");
        ImGui::TextWrapped("END: Exit Program");
        ImGui::PopTextWrapPos();

        // Performance info
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::End();
    }

} // namespace ui