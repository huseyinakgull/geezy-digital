#include "menu.hpp"
#include "../memory-external/imgui/imgui.h"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
#include "../memory-external/imgui/backends/imgui_impl_dx10.h"
#include <iostream>
#include "game_interface.hpp"
#include "logger.hpp"
#include <algorithm>

template<typename T> static inline T ImClamp(T v, T mn, T mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }

namespace ui {

    Menu::Menu()
        : m_isVisible(true)
        , m_isInitialized(false)
        , m_activeTab(Tab::SETTINGS)
        , m_scaleFactor(1.0f)
        , m_lastWindowSize(ImVec2(0, 0))
        , m_esp(nullptr)
    {
    }

    Menu::~Menu() {
        Shutdown();
        if (m_esp) {
            delete m_esp;
            m_esp = nullptr;
        }
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
        // Setup ImGui style - daha modern flat tasarým
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();

        // Köþe yuvarlama - modern tasarým için daha fazla yuvarlatma
        style.WindowRounding = 5.0f;      // Pencere köþeleri
        style.ChildRounding = 5.0f;       // Ýç paneller
        style.FrameRounding = 4.0f;       // Buton ve çerçeveler
        style.PopupRounding = 4.0f;       // Popup menüler
        style.ScrollbarRounding = 4.0f;   // Scrollbar
        style.GrabRounding = 4.0f;        // Slider grabber
        style.TabRounding = 4.0f;         // Sekmeler

        // Hizalama
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);  // Baþlýk ortalamasý
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);   // Buton metni ortalamasý

        // Boyutlar ve aralýklar - daha ferah görünüm
        style.WindowPadding = ImVec2(12.0f, 12.0f);    // Pencere iç dolgusu
        style.FramePadding = ImVec2(6.0f, 4.0f);       // Çerçeve dolgusu
        style.ItemSpacing = ImVec2(8.0f, 8.0f);        // Öðeler arasý boþluk
        style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);   // Öðe içi boþluk
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);  // Dokunmatik ekran için ek dolgu
        style.IndentSpacing = 20.0f;                   // Girinti boþluðu
        style.ScrollbarSize = 12.0f;                   // Scrollbar boyutu
        style.GrabMinSize = 10.0f;                     // Slider grab minimum boyutu

        // Kenarlýklar - minimal görünüm için ince kenarlýklar
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;    // Çerçeve kenarlýklarý (deðeri 0'dan 1'e çektim)
        style.TabBorderSize = 0.0f;

        // Diðer ayarlar
        style.WindowMenuButtonPosition = ImGuiDir_None;  // Menü butonunu kaldýr
        style.ColorButtonPosition = ImGuiDir_Right;      // Renk butonlarý saðda
        style.Alpha = 1.0f;                              // Genel opaklýk
        style.DisabledAlpha = 0.6f;                      // Devre dýþý öðeler için opaklýk
        style.AntiAliasedLines = true;                   // Kenar yumuþatma
        style.AntiAliasedFill = true;                    // Dolgu yumuþatma

        // Renk paleti - daha modern, koyu ama canlý tonlarla
        ImVec4* colors = style.Colors;

        // Ana arka plan renkleri - koyu gri/siyah tonlarý, hafif transparan
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.97f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.18f, 0.60f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);

        // Ana tema rengi - daha canlý kýrmýzý
        ImVec4 mainAccent = ImVec4(0.85f, 0.15f, 0.15f, 1.00f);     // Ana vurgu rengi
        ImVec4 mainAccentHover = ImVec4(0.95f, 0.25f, 0.25f, 1.00f); // Hover durumu rengi
        ImVec4 mainAccentActive = ImVec4(1.00f, 0.30f, 0.30f, 1.00f); // Aktif durum rengi

        // Headers, tabs ve baþlýk çubuklarý
        colors[ImGuiCol_Header] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.35f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(mainAccentHover.x, mainAccentHover.y, mainAccentHover.z, 0.45f);
        colors[ImGuiCol_HeaderActive] = ImVec4(mainAccentActive.x, mainAccentActive.y, mainAccentActive.z, 0.55f);

        // Sekmeler
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.18f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(mainAccentHover.x, mainAccentHover.y, mainAccentHover.z, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.18f, 0.90f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.28f, 0.90f);

        // Baþlýk çubuðu
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.18f, 0.75f);

        // Etkileþimli öðeler
        colors[ImGuiCol_Button] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(mainAccentHover.x, mainAccentHover.y, mainAccentHover.z, 0.85f);
        colors[ImGuiCol_ButtonActive] = ImVec4(mainAccentActive.x, mainAccentActive.y, mainAccentActive.z, 1.00f);

        colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);

        colors[ImGuiCol_SliderGrab] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(mainAccentActive.x, mainAccentActive.y, mainAccentActive.z, 1.00f);

        // Çerçeve arkaplanlarý (checkbox, radio buton vb. için)
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 0.60f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 0.70f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.32f, 0.80f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.13f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 0.40f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.40f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.70f);

        // Metin
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // Ayýrýcýlar, kenarlýklar
        colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.27f, 0.70f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(mainAccent.x, mainAccent.y, mainAccent.z, 0.70f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(mainAccentActive.x, mainAccentActive.y, mainAccentActive.z, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
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

        if (!m_esp && gameInterface && gameInterface->GetMemoryManager()) {
            m_esp = new core::ESP(gameInterface->GetMemoryManager());
            utils::LogInfo("ESP module initialized");
        }
    }

    void Menu::RenderDrawData() {
        if (!m_isInitialized)
            return;

        ImGui::Render();
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    }

    // Ekran boyutuna göre ölçeklendirme hesapla
    void Menu::CalculateScaling() {
        ImGuiIO& io = ImGui::GetIO();

        // Mevcut pencere boyutu
        ImVec2 currentSize = ImGui::GetWindowSize();

        // Pencere boyutu deðiþtiðinde ölçeklendirmeyi yeniden hesapla
        if (m_lastWindowSize.x != currentSize.x || m_lastWindowSize.y != currentSize.y) {
            m_lastWindowSize = currentSize;

            // Referans tasarým boyutu (950x650)
            const float referenceWidth = 950.0f;
            const float referenceHeight = 650.0f;

            // Boyutun referans deðere oranýný kullanarak ölçeklendirme faktörünü hesapla
            float widthScale = currentSize.x / referenceWidth;
            float heightScale = currentSize.y / referenceHeight;

            // Ýki ölçeði ortala veya küçük olaný kullan
            m_scaleFactor = (widthScale + heightScale) * 0.5f;

            // Aþýrý büyük veya küçük deðerleri sýnýrla
            m_scaleFactor = ImClamp(m_scaleFactor, 0.8f, 1.5f);
        }
    }

    // Tab içeriklerinde dinamik boyutlar için yardýmcý fonksiyon
    ImVec2 Menu::GetScaledSize(float width, float height) {
        return ImVec2(width * m_scaleFactor, height * m_scaleFactor);
    }

    float Menu::GetScaledFontSize(float size) {
        return size * m_scaleFactor;
    }

    // Custom UI elements implementation
    bool Menu::CustomCheckbox(const char* label, bool* v) {
        // Checkbox kutusu için stil
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.21f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.23f, 0.23f, 0.26f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.28f, 0.28f, 0.31f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));

        // Checkbox daha yuvarlak görünüm için
        float originalRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 3.0f;

        // Checkbox boyutu
        float originalScale = ImGui::GetStyle().FramePadding.y;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));

        bool result = ImGui::Checkbox(label, v);

        // Orijinal deðerlere geri dön
        ImGui::GetStyle().FrameRounding = originalRounding;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        return result;
    }

    bool Menu::CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        // Modern slider stili
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.21f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.85f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.00f, 0.30f, 0.30f, 1.0f));

        // Slider yuvarlaklýðý
        float originalRounding = ImGui::GetStyle().GrabRounding;
        ImGui::GetStyle().GrabRounding = 3.0f;

        // Frame yuvarlaklýðý
        float originalFrameRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 4.0f;

        // Slider'ýn geniþliði ve yüksekliði
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 18.0f); // Grab'i daha kolay tutulabilir yap

        bool result = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);

        // Stili geri al
        ImGui::GetStyle().GrabRounding = originalRounding;
        ImGui::GetStyle().FrameRounding = originalFrameRounding;
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        return result;
    }

    bool Menu::CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
        // Modern slider stili
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.21f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.85f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.00f, 0.30f, 0.30f, 1.0f));

        // Slider yuvarlaklýðý
        float originalRounding = ImGui::GetStyle().GrabRounding;
        ImGui::GetStyle().GrabRounding = 3.0f;

        // Frame yuvarlaklýðý
        float originalFrameRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 4.0f;

        // Slider'ýn geniþliði ve yüksekliði
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 18.0f); // Grab'i daha kolay tutulabilir yap

        bool result = ImGui::SliderInt(label, v, v_min, v_max, format, flags);

        // Stili geri al
        ImGui::GetStyle().GrabRounding = originalRounding;
        ImGui::GetStyle().FrameRounding = originalFrameRounding;
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        return result;
    }

    bool Menu::CustomButton(const char* label, const ImVec2& size_arg) {
        // Modern buton stili
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.15f, 0.15f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.00f, 0.30f, 0.30f, 1.00f));

        // Buton yuvarlaklýðý
        float originalRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 4.0f;

        // Buton padding
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));

        bool result = ImGui::Button(label, size_arg);

        // Stili geri al
        ImGui::GetStyle().FrameRounding = originalRounding;
        ImGui::PopStyleVar();
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
        // Modern combo stili
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.21f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.15f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.85f, 0.15f, 0.15f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.90f, 0.20f, 0.20f, 0.7f));

        // Combo yuvarlaklýðý
        float originalRounding = ImGui::GetStyle().FrameRounding;
        ImGui::GetStyle().FrameRounding = 4.0f;
        float originalPopupRounding = ImGui::GetStyle().PopupRounding;
        ImGui::GetStyle().PopupRounding = 4.0f;

        // Padding
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));

        bool result = ImGui::Combo(label, current_item, items, items_count, height_in_items);

        // Stili geri al
        ImGui::GetStyle().FrameRounding = originalRounding;
        ImGui::GetStyle().PopupRounding = originalPopupRounding;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        return result;
    }

    void Menu::CustomSeparator() {
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.25f, 0.25f, 0.27f, 0.70f));
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    void Menu::CustomGroupBox(const char* name, const ImVec2& size_arg) {
        // Özel bir çerçeve stili uygula
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.28f, 0.50f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);  // Yuvarlak köþeler
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);  // Ýnce kenarlýk

        // Grup baþlýðý için biraz boþluk ekle
        ImGui::BeginChild(name, size_arg, true);

        // Grup baþlýðý - daha belirgin ve modern
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Varsayýlan font
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
        ImGui::SetCursorPosX(10); // Sol kenardan boþluk
        ImGui::Text("%s", name);
        ImGui::PopStyleColor();
        ImGui::PopFont();

        // Baþlýk altý çizgi
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.85f, 0.20f, 0.20f, 0.5f));
        const float lineWidth = ImGui::GetContentRegionAvail().x * 0.99f; // Biraz daha kýsa
        const float lineY = ImGui::GetCursorPosY() + 2;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetCursorPosX(), lineY),
            ImVec2(ImGui::GetCursorPosX() + lineWidth, lineY),
            ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
        ImGui::PopStyleColor();

        // Ýçerik için kenar boþluðu ayarla
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::Indent(8.0f);
    }

    void Menu::EndGroupBox() {
        ImGui::Unindent(8.0f);
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    void Menu::RenderMainMenu(game::GameInterface* gameInterface) {
        // Pencere boyutuna göre ölçeklendirmeyi hesapla
        CalculateScaling();

        // Get display size
        ImGuiIO& io = ImGui::GetIO();

        // Position the menu in the center of the screen
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(950, 650), ImGuiCond_Once);

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
        if (ImGui::Button("X", ImVec2(25, 20))) {
            // Close button functionality
            m_isVisible = false;
        }

        ImGui::EndGroup();

        // Make the entire header area draggable
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##draggable_area", ImVec2(ImGui::GetWindowWidth() - 30, 30));
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
        float tabWidth = (windowWidth - 20) / 6; // 6 tabs with 10px padding on each side

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

        ImGui::SameLine(0, 0);

        // Account Tab 
        if (CustomTabButton("Account", m_activeTab == Tab::ACCOUNT, ImVec2(tabWidth, 30))) {
            m_activeTab = Tab::ACCOUNT;
        }

        ImGui::Spacing();
        CustomSeparator();
        ImGui::Spacing();

        // ÖNEMLÝ DEÐÝÞÝKLÝK: Tab içeriðini içeren bir child window oluþtur
        // Bu sayede içerik alt kýsýmdaki butonlara kadar tüm alaný kullanabilir
        const float contentHeight = ImGui::GetContentRegionAvail().y - 50; // Alt buton alanýný çýkar
        ImGui::BeginChild("##TabContent", ImVec2(0, contentHeight), true);

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
        case Tab::ACCOUNT:
            RenderAccountTab();
            break;
        }

        ImGui::EndChild();

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

    // Settings Tab için aþaðýdaki düzeltmeleri yapabiliriz
    // RenderSettingsTab fonksiyonu içindeki deðiþiklikler:

    void Menu::RenderSettingsTab() {
        // Mevcut içerik geniþliðini ve yüksekliðini al
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun arasýnda 10px boþluk

        // Her sütundaki GroupBox'lar için dinamik yükseklik hesapla
        // Formül: Toplam alan / kutu sayýsý - kutular arasý boþluk
        const float leftColumnBoxCount = 2; // Sol sütunda 2 grup kutusu var
        const float rightColumnBoxCount = 3; // Sað sütunda 3 grup kutusu var (boþluðu doldurmak için ekstra 1 kutu ekliyoruz)
        const float groupSpacing = 10.0f; // Grup kutularý arasý boþluk

        const float leftGroupHeight = (contentHeight - ((leftColumnBoxCount - 1) * groupSpacing)) / leftColumnBoxCount;
        const float rightGroupHeight = (contentHeight - ((rightColumnBoxCount - 1) * groupSpacing)) / rightColumnBoxCount;

        // Sol Sütun
        ImGui::BeginGroup();

        // General Settings
        CustomGroupBox("General Settings", ImVec2(columnWidth, leftGroupHeight * 0.45f));
        {
            // Mevcut içerik buraya...
            static bool vsync = false;
            CustomCheckbox("VSync", &vsync);

            static int refreshRate = 60;
            ImGui::TextUnformatted("Refresh Rate");
            CustomSliderInt("##RefreshRate", &refreshRate, 30, 240, "%d");

            static bool overlay = false;
            CustomCheckbox("Overlay Enabled", &overlay);

            // Ýsteðe baðlý olarak daha fazla ayar ekleyebilirsiniz...
        }
        EndGroupBox();

        ImGui::Spacing();

        // Theme Settings
        CustomGroupBox("Theme Settings", ImVec2(columnWidth, leftGroupHeight * 0.55f - groupSpacing));
        {
            // Mevcut içerik buraya...
            static float alpha = 1.0f;
            ImGui::Text("Window Opacity");
            CustomSliderFloat("##WindowOpacity", &alpha, 0.5f, 1.0f, "%.2f");

            static float r = 0.7f, g = 0.1f, b = 0.1f;
            ImGui::Text("Accent Color");
            ImGui::ColorEdit3("##AccentColor", (float*)&r);

            // Diðer tema ayarlarýný burada ekleyebilirsiniz
            ImGui::Spacing();
            ImGui::Text("Border Thickness");
            static float borderThickness = 1.0f;
            CustomSliderFloat("##BorderThickness", &borderThickness, 0.0f, 2.0f, "%.1f");

            ImGui::Spacing();
            ImGui::Text("Corner Roundness");
            static float cornerRoundness = 4.0f;
            CustomSliderFloat("##CornerRoundness", &cornerRoundness, 0.0f, 10.0f, "%.1f");

            if (CustomButton("Reset Colors", ImVec2(120, 25))) {
                r = 0.7f; g = 0.1f; b = 0.1f;
                alpha = 1.0f;
                borderThickness = 1.0f;
                cornerRoundness = 4.0f;
            }
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Hotkey Settings
        CustomGroupBox("Hotkey Settings", ImVec2(columnWidth, rightGroupHeight));
        {
            const char* keys[] = { "F1", "F2", "F3", "F4", "E", "R", "T", "X", "Z", "TAB" };
            static int menuKey = 4; // "E" default

            ImGui::Text("Menu Key");
            CustomCombo("##MenuKey", &menuKey, keys, IM_ARRAYSIZE(keys));

            static int exitKey = 0;
            ImGui::Text("Exit Key");
            CustomCombo("##ExitKey", &exitKey, keys, IM_ARRAYSIZE(keys));

            // Ek tuþ atamalarý
            ImGui::Spacing();
            static int screenshotKey = 1;
            ImGui::Text("Screenshot Key");
            CustomCombo("##ScreenshotKey", &screenshotKey, keys, IM_ARRAYSIZE(keys));

            ImGui::Spacing();
            static int toggleOverlayKey = 2;
            ImGui::Text("Toggle Overlay Key");
            CustomCombo("##ToggleOverlayKey", &toggleOverlayKey, keys, IM_ARRAYSIZE(keys));
        }
        EndGroupBox();

        ImGui::Spacing();

        // Performance Settings
        CustomGroupBox("Performance Settings", ImVec2(columnWidth, rightGroupHeight));
        {
            static bool lowEndMode = false;
            CustomCheckbox("Low-End Mode", &lowEndMode);

            static int maxFps = 120;
            ImGui::Text("Maximum FPS");
            CustomSliderInt("##MaxFPS", &maxFps, 30, 300, "%d");

            static bool threadedRendering = false;
            CustomCheckbox("Threaded Rendering", &threadedRendering);

            // Ek performans ayarlarý
            ImGui::Spacing();
            static bool multiCoreRendering = false;
            CustomCheckbox("Multi-Core Rendering", &multiCoreRendering);

            ImGui::Spacing();
            ImGui::Text("Memory Usage Limit (MB)");
            static int memoryLimit = 512;
            CustomSliderInt("##MemoryLimit", &memoryLimit, 128, 1024, "%d MB");

            ImGui::Spacing();
            static bool asyncTextureLoading = false;
            CustomCheckbox("Async Texture Loading", &asyncTextureLoading);

            ImGui::Spacing();
            static bool modelCaching = false;
            CustomCheckbox("Model Caching", &modelCaching);
        }
        EndGroupBox();

        ImGui::Spacing();

        // Alt kýsýmdaki boþluðu doldurmak için ek bir grup kutusu ekleyelim
        CustomGroupBox("Advanced Settings", ImVec2(columnWidth, rightGroupHeight - 10));
        {
            static bool debugLogging = false;
            CustomCheckbox("Debug Logging", &debugLogging);

            static bool extendedStats = false;
            CustomCheckbox("Extended Statistics", &extendedStats);

            static bool compatibilityMode = false;
            CustomCheckbox("Compatibility Mode", &compatibilityMode);

            static int updateInterval = 1000;
            ImGui::Text("Update Check Interval (ms)");
            CustomSliderInt("##UpdateInterval", &updateInterval, 100, 5000, "%d");

            static bool experimentalFeatures = false;
            CustomCheckbox("Experimental Features", &experimentalFeatures);

            if (CustomButton("Reset All Settings", ImVec2(150, 25))) {
                // Tüm ayarlarý sýfýrlama fonksiyonu
            }
        }
        EndGroupBox();

        ImGui::EndGroup();
    }
    // Visuals Tab
    void Menu::RenderVisualsTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun

        // Grup kutusu yüksekliklerini dinamik hesapla
        const float groupSpacing = 10.0f;
        const float leftGroupHeight = contentHeight; // Sol tarafta bir büyük kutu
        const float rightGroupHeight1 = contentHeight * 0.5f - groupSpacing / 2;
        const float rightGroupHeight2 = contentHeight * 0.5f - groupSpacing / 2;

        // Sol Sütun - Büyük bir ESP kutusu
        ImGui::BeginGroup();

        // ESP Settings - Tam yükseklik
        CustomGroupBox("ESP Settings", ImVec2(columnWidth, leftGroupHeight));
        {
            bool espEnabled = m_esp->IsEnabled();
            if (CustomCheckbox("ESP Enabled", &espEnabled)) {
                m_esp->SetEnabled(espEnabled);
            }

            ImGui::BeginDisabled(!espEnabled);

            static bool showBox = false;
            CustomCheckbox("Show Box", &showBox);

            static bool showHealth = false;
            CustomCheckbox("Show Health", &showHealth);

            static bool showName = false;
            CustomCheckbox("Show Name", &showName);

            static bool showDistance = false;
            CustomCheckbox("Show Distance", &showDistance);

            static bool showWeapon = false;
            CustomCheckbox("Show Weapon", &showWeapon);

            // Yeni opsiyonlar
            static bool showAmmo = false;
            CustomCheckbox("Show Ammo", &showAmmo);

            static bool showRank = false;
            CustomCheckbox("Show Rank", &showRank);

            static bool showSkeleton = false;
            CustomCheckbox("Show Skeleton", &showSkeleton);

            static bool showSnaplines = false;
            CustomCheckbox("Show Snaplines", &showSnaplines);

            static bool showHeadDot = false;
            CustomCheckbox("Show Head Dot", &showHeadDot);

            ImGui::Text("Maximum Distance");
            static float maxDistance = 1000.0f;
            CustomSliderFloat("##MaxDistance", &maxDistance, 100.0f, 3000.0f, "%.0f");

            ImGui::Text("Box Thickness");
            static int boxThickness = 2;
            CustomSliderInt("##BoxThickness", &boxThickness, 1, 5, "%d");

            ImGui::Text("Box Color");
            static float boxColor[3] = { 0.9f, 0.2f, 0.2f };
            ImGui::ColorEdit3("##BoxColor", boxColor);

            // Diðer özelleþtirme seçenekleri
            ImGui::Text("Health Bar Position");
            static int healthBarPos = 0;
            const char* healthBarPositions[] = { "Left", "Right", "Top", "Bottom" };
            CustomCombo("##HealthBarPos", &healthBarPos, healthBarPositions, IM_ARRAYSIZE(healthBarPositions));

            ImGui::Text("Text Scale");
            static float textScale = 1.0f;
            CustomSliderFloat("##TextScale", &textScale, 0.8f, 1.5f, "%.1f");

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun - Ýki bölüm
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Glow Settings
        CustomGroupBox("Glow Settings", ImVec2(columnWidth, rightGroupHeight1));
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

            ImGui::Text("Pulse Rate");
            static float pulseRate = 1.0f;
            CustomSliderFloat("##PulseRate", &pulseRate, 0.5f, 2.0f, "%.1f Hz");

            ImGui::Text("Glow Through Walls");
            static bool glowThroughWalls = false;
            CustomCheckbox("##GlowThroughWalls", &glowThroughWalls);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Chams Settings
        CustomGroupBox("Chams Settings", ImVec2(columnWidth, rightGroupHeight2));
        {
            static bool chamsEnabled = false;
            CustomCheckbox("Chams Enabled", &chamsEnabled);

            ImGui::BeginDisabled(!chamsEnabled);

            static int chamsType = 0;
            const char* chamsTypes[] = { "Flat", "Textured", "Wireframe", "Metallic", "Pulse", "Crystal" };
            ImGui::Text("Chams Type");
            CustomCombo("##ChamsType", &chamsType, chamsTypes, IM_ARRAYSIZE(chamsTypes));

            ImGui::Text("Visible Color");
            static float visibleColor[3] = { 0.2f, 0.8f, 0.2f };
            ImGui::ColorEdit3("##VisibleColor", visibleColor);

            ImGui::Text("Invisible Color");
            static float invisibleColor[3] = { 0.9f, 0.6f, 0.1f };
            ImGui::ColorEdit3("##InvisibleColor", invisibleColor);

            ImGui::Text("Material Brightness");
            static float materialBrightness = 1.0f;
            CustomSliderFloat("##MaterialBrightness", &materialBrightness, 0.1f, 2.0f, "%.1f");

            ImGui::Text("Material Gloss");
            static float materialGloss = 0.5f;
            CustomSliderFloat("##MaterialGloss", &materialGloss, 0.0f, 1.0f, "%.2f");

            ImGui::Text("Apply To");
            static int applyTo = 0;
            const char* applyToOptions[] = { "Enemies Only", "Team Only", "All Players", "Weapons" };
            CustomCombo("##ApplyTo", &applyTo, applyToOptions, IM_ARRAYSIZE(applyToOptions));

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    // Aimbot Sekmesi
    void Menu::RenderAimbotTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun arasý 10px boþluk

        // Grup kutusu yüksekliklerini dinamik hesapla
        const float groupSpacing = 10.0f;

        // Sol tarafta bir büyük kutu
        const float leftGroupHeight = contentHeight;

        // Sað tarafta iki kutu
        const float rightGroupHeight1 = contentHeight * 0.45f - groupSpacing / 2;
        const float rightGroupHeight2 = contentHeight * 0.55f - groupSpacing / 2;

        // Sol Sütun
        ImGui::BeginGroup();

        // Aimbot Settings - Tam yükseklik
        CustomGroupBox("Aimbot Settings", ImVec2(columnWidth, leftGroupHeight));
        {
            static bool aimbotEnabled = false;
            CustomCheckbox("Aimbot Enabled", &aimbotEnabled);

            ImGui::BeginDisabled(!aimbotEnabled);

            static int aimbotKey = 2;
            const char* keys[] = { "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "Shift", "Ctrl", "Alt", "Space", "C" };
            ImGui::Text("Aimbot Key");
            CustomCombo("##AimbotKey", &aimbotKey, keys, IM_ARRAYSIZE(keys));

            static int aimbotBone = 0;
            const char* bones[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis", "Closest", "Dynamic" };
            ImGui::Text("Target Bone");
            CustomCombo("##TargetBone", &aimbotBone, bones, IM_ARRAYSIZE(bones));

            ImGui::Text("Aimbot FOV");
            static float aimbotFov = 5.0f;
            CustomSliderFloat("##AimbotFOV", &aimbotFov, 1.0f, 180.0f, "%.1f°");

            ImGui::Text("Smoothing");
            static float aimbotSmoothing = 1.0f;
            CustomSliderFloat("##AimbotSmoothing", &aimbotSmoothing, 1.0f, 20.0f, "%.1f");

            static bool visibilityCheck = false;
            CustomCheckbox("Visibility Check", &visibilityCheck);

            static bool teamCheck = false;
            CustomCheckbox("Team Check", &teamCheck);

            static bool drawFov = false;
            CustomCheckbox("Draw FOV Circle", &drawFov);

            // Ek aimbot ayarlarý
            ImGui::Spacing();
            ImGui::Text("FOV Circle Color");
            static float fovColor[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
            ImGui::ColorEdit4("##FOVCircleColor", fovColor);

            ImGui::Spacing();
            static bool autoFire = false;
            CustomCheckbox("Auto Fire", &autoFire);

            ImGui::Spacing();
            static bool autoScope = false;
            CustomCheckbox("Auto Scope", &autoScope);

            ImGui::Spacing();
            static bool silentAim = false;
            CustomCheckbox("Silent Aim", &silentAim);

            ImGui::Spacing();
            ImGui::Text("Delay After Kill (ms)");
            static int delayAfterKill = 0;
            CustomSliderInt("##DelayAfterKill", &delayAfterKill, 0, 1000, "%d ms");

            ImGui::Spacing();
            ImGui::Text("Target Selection");
            static int targetSelection = 0;
            const char* targetOptions[] = { "Closest to Crosshair", "Closest Distance", "Lowest Health", "Highest Damage" };
            CustomCombo("##TargetSelection", &targetSelection, targetOptions, IM_ARRAYSIZE(targetOptions));

            ImGui::Spacing();
            ImGui::Text("Priority List");

            static bool priorityHead = false;
            static bool priorityBody = false;
            static bool priorityArms = false;
            static bool priorityLegs = false;

            CustomCheckbox("Head", &priorityHead);
            ImGui::SameLine(columnWidth * 0.3f);
            CustomCheckbox("Body", &priorityBody);
            ImGui::SameLine(columnWidth * 0.6f);
            CustomCheckbox("Arms", &priorityArms);
            ImGui::SameLine(columnWidth * 0.9f);
            CustomCheckbox("Legs", &priorityLegs);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun - Ýki bölüm
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Triggerbot Settings
        CustomGroupBox("Triggerbot Settings", ImVec2(columnWidth, rightGroupHeight1));
        {
            static bool triggerbotEnabled = false;
            CustomCheckbox("Triggerbot Enabled", &triggerbotEnabled);

            ImGui::BeginDisabled(!triggerbotEnabled);

            static int triggerbotKey = 4;
            const char* keys[] = { "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Mouse5", "Shift", "Ctrl", "Alt", "None (Always on)" };
            ImGui::Text("Triggerbot Key");
            CustomCombo("##TriggerbotKey", &triggerbotKey, keys, IM_ARRAYSIZE(keys));

            ImGui::Text("Delay (ms)");
            static int triggerbotDelay = 100;
            CustomSliderInt("##TriggerbotDelay", &triggerbotDelay, 0, 500, "%d ms");

            ImGui::Text("Duration (ms)");
            static int triggerbotDuration = 100;
            CustomSliderInt("##TriggerbotDuration", &triggerbotDuration, 10, 300, "%d ms");

            static bool visibilityCheck = false;
            CustomCheckbox("Visibility Check", &visibilityCheck);

            static bool teamCheck = false;
            CustomCheckbox("Team Check", &teamCheck);

            ImGui::Spacing();
            ImGui::Text("Trigger When");
            static int triggerWhen = 0;
            const char* triggerOptions[] = { "On Target", "On Any Body Part", "On Head Only", "On Custom" };
            CustomCombo("##TriggerWhen", &triggerWhen, triggerOptions, IM_ARRAYSIZE(triggerOptions));

            if (triggerWhen == 3) { // "On Custom" seçildiyse
                static bool triggerHead = false;
                static bool triggerChest = false;
                static bool triggerStomach = false;
                static bool triggerArms = false;
                static bool triggerLegs = false;

                ImGui::Text("Trigger Parts:");
                CustomCheckbox("Head", &triggerHead);
                ImGui::SameLine();
                CustomCheckbox("Chest", &triggerChest);
                ImGui::SameLine();
                CustomCheckbox("Stomach", &triggerStomach);

                CustomCheckbox("Arms", &triggerArms);
                ImGui::SameLine();
                CustomCheckbox("Legs", &triggerLegs);
            }

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Recoil Control Settings
        CustomGroupBox("Recoil & Accuracy Settings", ImVec2(columnWidth, rightGroupHeight2));
        {
            static bool rcsEnabled = false;
            CustomCheckbox("Recoil Control System", &rcsEnabled);

            ImGui::BeginDisabled(!rcsEnabled);

            ImGui::Text("X Strength");
            static float rcsX = 1.0f;
            CustomSliderFloat("##RCSX", &rcsX, 0.0f, 2.0f, "%.1f");

            ImGui::Text("Y Strength");
            static float rcsY = 1.0f;
            CustomSliderFloat("##RCSY", &rcsY, 0.0f, 2.0f, "%.1f");

            ImGui::Spacing();
            static bool smoothRCS = false;
            CustomCheckbox("Smooth RCS", &smoothRCS);

            ImGui::Text("RCS Smoothing");
            static float rcsSmoothing = 5.0f;
            CustomSliderFloat("##RCSSmoothing", &rcsSmoothing, 1.0f, 20.0f, "%.1f");

            ImGui::Spacing();
            static bool rcsAlwaysOn = false;
            CustomCheckbox("RCS Always On", &rcsAlwaysOn);

            ImGui::Spacing();
            static bool noSpread = false;
            CustomCheckbox("No Spread", &noSpread);

            static bool noRecoil = false;
            CustomCheckbox("No Recoil", &noRecoil);

            static bool accuracyBoost = false;
            CustomCheckbox("Accuracy Boost", &accuracyBoost);

            ImGui::Text("Accuracy Boost Level");
            static int accuracyLevel = 1;
            CustomSliderInt("##AccuracyLevel", &accuracyLevel, 1, 5, "Level %d");

            ImGui::Spacing();
            static bool weaponSpecificSettings = false;
            CustomCheckbox("Weapon-Specific Settings", &weaponSpecificSettings);

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    // Misc Sekmesi 
    void Menu::RenderMiscTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun

        // Grup kutusu yüksekliklerini dinamik hesapla
        const float groupSpacing = 10.0f;

        // Her sütunda iki kutu
        const float leftGroupHeight1 = contentHeight * 0.6f - groupSpacing / 2;
        const float leftGroupHeight2 = contentHeight * 0.4f - groupSpacing / 2;
        const float rightGroupHeight1 = contentHeight * 0.4f - groupSpacing / 2;
        const float rightGroupHeight2 = contentHeight * 0.6f - groupSpacing / 2;

        // Sol Sütun
        ImGui::BeginGroup();

        // Miscellaneous Features - Büyük kutu
        CustomGroupBox("Movement & Game Features", ImVec2(columnWidth, leftGroupHeight1));
        {
            static bool bhopEnabled = false;
            CustomCheckbox("Bunny Hop", &bhopEnabled);

            static bool autoStrafe = false;
            CustomCheckbox("Auto Strafe", &autoStrafe);

            ImGui::Text("Auto Strafe Mode");
            static int strafeMode = 0;
            const char* strafeModes[] = { "Legit", "Rage", "Silent", "Directional" };
            CustomCombo("##StrafeMode", &strafeMode, strafeModes, IM_ARRAYSIZE(strafeModes));

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

            // Ek özellikler
            static bool speedHack = false;
            CustomCheckbox("Speed Hack (Risky)", &speedHack);
            ImGui::BeginDisabled(!speedHack);
            ImGui::Text("Speed Multiplier");
            static float speedMultiplier = 1.3f;
            CustomSliderFloat("##SpeedMultiplier", &speedMultiplier, 1.0f, 3.0f, "%.1fx");
            ImGui::EndDisabled();

            static bool jumpBug = false;
            CustomCheckbox("Jump Bug", &jumpBug);

            static bool edgeJump = false;
            CustomCheckbox("Edge Jump", &edgeJump);

            static bool fakeLag = false;
            CustomCheckbox("Fake Lag (Risky)", &fakeLag);
            ImGui::BeginDisabled(!fakeLag);
            ImGui::Text("Lag Amount");
            static int lagAmount = 5;
            CustomSliderInt("##LagAmount", &lagAmount, 1, 14, "%d ticks");
            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Second box on left
        CustomGroupBox("HUD & Sound Settings", ImVec2(columnWidth, leftGroupHeight2));
        {
            static bool damageIndicator = false;
            CustomCheckbox("Damage Indicator", &damageIndicator);

            static bool hitMarker = false;
            CustomCheckbox("Hit Marker", &hitMarker);

            static bool hitSound = false;
            CustomCheckbox("Hit Sound", &hitSound);

            ImGui::Text("Hit Sound Type");
            static int hitSoundType = 0;
            const char* sounds[] = { "Classic", "Bell", "Bubble", "Metallic", "Headshot" };
            CustomCombo("##HitSoundType", &hitSoundType, sounds, IM_ARRAYSIZE(sounds));

            ImGui::Text("Sound Volume");
            static float soundVolume = 0.5f;
            CustomSliderFloat("##SoundVolume", &soundVolume, 0.0f, 1.0f, "%.1f");

            static bool killfeed = false;
            CustomCheckbox("Custom Killfeed", &killfeed);

            static bool bombTimer = false;
            CustomCheckbox("Bomb Timer", &bombTimer);

            static bool specList = false;
            CustomCheckbox("Spectator List", &specList);
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun 
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Skin Changer - Küçük kutu üstte
        CustomGroupBox("Skin Changer", ImVec2(columnWidth, rightGroupHeight1));
        {
            static bool skinChangerEnabled = false;
            CustomCheckbox("Skin Changer Enabled", &skinChangerEnabled);

            ImGui::BeginDisabled(!skinChangerEnabled);

            static int weaponSelect = 0;
            const char* weapons[] = { "AK-47", "M4A4", "M4A1-S", "AWP", "Desert Eagle", "USP-S", "Glock-18", "Knife" };
            ImGui::Text("Weapon");
            CustomCombo("##WeaponSelect", &weaponSelect, weapons, IM_ARRAYSIZE(weapons));

            static int skinSelect = 0;
            const char* skins[] = { "Default", "Asiimov", "Hyper Beast", "Neo-Noir", "Dragon Lore", "Fade", "Doppler", "Tiger Tooth" };
            ImGui::Text("Skin");
            CustomCombo("##SkinSelect", &skinSelect, skins, IM_ARRAYSIZE(skins));

            ImGui::Text("Wear");
            static float wear = 0.0f;
            CustomSliderFloat("##Wear", &wear, 0.0f, 1.0f, "%.2f");

            static bool statTrak = false;
            CustomCheckbox("StatTrak", &statTrak);

            ImGui::Text("StatTrak Count");
            static int statTrakCount = 1337;
            CustomSliderInt("##StatTrakCount", &statTrakCount, 0, 9999, "%d");

            static char nameTag[32] = "GEEZY.DIGITAL";
            ImGui::Text("Name Tag");
            ImGui::InputText("##NameTag", nameTag, IM_ARRAYSIZE(nameTag));

            ImGui::EndDisabled();
        }
        EndGroupBox();

        ImGui::Spacing();

        // Crosshair & Config - Büyük kutu altta
        CustomGroupBox("Crosshair & Config", ImVec2(columnWidth, rightGroupHeight2));
        {
            // Crosshair bölümü
            static bool customCrosshair = false;
            CustomCheckbox("Custom Crosshair", &customCrosshair);

            ImGui::BeginDisabled(!customCrosshair);

            static int crosshairType = 0;
            const char* crosshairTypes[] = { "Cross", "Circle", "Dot", "T-Shape", "X-Shape", "Swastika", "Box" };
            ImGui::Text("Type");
            CustomCombo("##CrosshairType", &crosshairType, crosshairTypes, IM_ARRAYSIZE(crosshairTypes));

            ImGui::Text("Size");
            static int crosshairSize = 5;
            CustomSliderInt("##CrosshairSize", &crosshairSize, 1, 20, "%d");

            ImGui::Text("Thickness");
            static int crosshairThickness = 1;
            CustomSliderInt("##CrosshairThickness", &crosshairThickness, 1, 5, "%d");

            ImGui::Text("Gap");
            static int crosshairGap = 3;
            CustomSliderInt("##CrosshairGap", &crosshairGap, 0, 10, "%d");

            ImGui::Text("Color");
            static float crosshairColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            ImGui::ColorEdit4("##CrosshairColor", crosshairColor);

            static bool dynamicCrosshair = false;
            CustomCheckbox("Dynamic Crosshair", &dynamicCrosshair);

            static bool crosshairOutline = false;
            CustomCheckbox("Crosshair Outline", &crosshairOutline);

            ImGui::EndDisabled();

            // Ayýrýcý çizgi
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Config bölümü
            ImGui::Text("Configuration Settings");
            ImGui::Spacing();

            static char configName[32] = "default_config";
            ImGui::Text("Config Name");
            ImGui::InputText("##ConfigName", configName, IM_ARRAYSIZE(configName));

            ImGui::Spacing();
            ImGui::BeginGroup();
            if (CustomButton("Save Config", ImVec2(columnWidth * 0.48f, 25))) {
                // Save config function
            }

            ImGui::SameLine();
            if (CustomButton("Load Config", ImVec2(columnWidth * 0.48f, 25))) {
                // Load config function
            }
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::BeginGroup();
            if (CustomButton("Export Config", ImVec2(columnWidth * 0.48f, 25))) {
                // Export config function
            }

            ImGui::SameLine();
            if (CustomButton("Import Config", ImVec2(columnWidth * 0.48f, 25))) {
                // Import config function
            }
            ImGui::EndGroup();

            ImGui::Spacing();
            if (CustomButton("Reset All Settings", ImVec2(columnWidth - 10, 25))) {
                // Reset all settings function
            }
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    // Info Sekmesi
    void Menu::RenderInfoTab(game::GameInterface* gameInterface) {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun arasý 10px boþluk

        // Grup kutusu yüksekliklerini dinamik hesapla
        const float groupSpacing = 10.0f;

        // Sol sütun, iki kutu
        const float leftGroupHeight1 = contentHeight * 0.35f - groupSpacing / 2;
        const float leftGroupHeight2 = contentHeight * 0.65f - groupSpacing / 2;

        // Sað sütun, iki kutu
        const float rightGroupHeight1 = contentHeight * 0.35f - groupSpacing / 2;
        const float rightGroupHeight2 = contentHeight * 0.65f - groupSpacing / 2;

        // Sol Sütun
        ImGui::BeginGroup();

        // Application Info
        CustomGroupBox("Application Info", ImVec2(columnWidth, leftGroupHeight1));
        {
            ImGui::BulletText("Version: 1.0");
            ImGui::BulletText("Build Date: %s", __DATE__);
            ImGui::BulletText("Build: Release");
            ImGui::BulletText("Developer: GeezyDigital");
            ImGui::BulletText("License: Premium");
            ImGui::BulletText("Framework: ImGui + DirectX 10");
            ImGui::BulletText("Last Update: March 04, 2025");
            ImGui::BulletText("Total Users: 1,532");
        }
        EndGroupBox();

        ImGui::Spacing();

        // Performance Monitoring - Daha büyük kutu
        CustomGroupBox("Performance Monitoring", ImVec2(columnWidth, leftGroupHeight2));
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
            ImGui::BulletText("UI Elements: 427");
            ImGui::BulletText("Threads: 3");
            ImGui::BulletText("ESP Processing Time: 0.42 ms");
            ImGui::BulletText("Aimbot Processing Time: 0.18 ms");

            ImGui::Spacing();
            ImGui::Text("Performance Metrics");

            // FPS grafiði
            ImGui::Text("Frame Times");
            ImGui::PlotLines("##FrameTimes", frameTimes, IM_ARRAYSIZE(frameTimes), frameTimeIdx,
                NULL, 0.0f, 50.0f, ImVec2(columnWidth - 20, 80));

            // CPU ve RAM kullaným grafikleri 
            static float cpuUsage[100] = {};
            static int cpuIdx = 0;
            cpuUsage[cpuIdx] = 5.0f + 2.0f * sin(ImGui::GetTime() * 0.3f); // Simüle edilmiþ deðerler
            cpuIdx = (cpuIdx + 1) % IM_ARRAYSIZE(cpuUsage);

            ImGui::Text("CPU Usage: %.1f%%", cpuUsage[cpuIdx]);
            ImGui::PlotLines("##CPUUsage", cpuUsage, IM_ARRAYSIZE(cpuUsage), cpuIdx,
                NULL, 0.0f, 15.0f, ImVec2(columnWidth - 20, 80));

            // Bellek kullanýmý
            static float memUsage[100] = {};
            static int memIdx = 0;
            memUsage[memIdx] = 30.0f + 5.0f * sin(ImGui::GetTime() * 0.2f); // Simüle edilmiþ deðerler
            memIdx = (memIdx + 1) % IM_ARRAYSIZE(memUsage);

            ImGui::Text("Memory Usage: %.1f MB", memUsage[memIdx]);
            ImGui::PlotLines("##MemUsage", memUsage, IM_ARRAYSIZE(memUsage), memIdx,
                NULL, 0.0f, 50.0f, ImVec2(columnWidth - 20, 80));
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // System Info & Game Info
        CustomGroupBox("System & Game Info", ImVec2(columnWidth, rightGroupHeight1));
        {
            if (gameInterface && gameInterface->IsConnected()) {
                ImGui::Text("Game Connection");
                ImGui::BulletText("Process ID: %u", gameInterface->GetProcessId());
                ImGui::BulletText("Client Module: 0x%llX", gameInterface->GetClientModuleBase());
                ImGui::BulletText("Game Version: 1.39.2.5");
                ImGui::BulletText("Handle Access: Full");

                ImGui::Spacing();
                ImGui::Text("System Information");
                ImGui::BulletText("OS: Windows 10 Pro 64-bit");
                ImGui::BulletText("CPU: Intel Core i7-12700K");
                ImGui::BulletText("GPU: NVIDIA RTX 3080");
                ImGui::BulletText("RAM: 32 GB DDR5");
                ImGui::BulletText("DirectX Version: 12");
            }
            else {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "No game connection!");
                ImGui::BulletText("Launch CS2 to connect");

                ImGui::Spacing();
                ImGui::Text("System Information");
                ImGui::BulletText("OS: Windows 10 Pro 64-bit");
                ImGui::BulletText("CPU: Intel Core i7-12700K");
                ImGui::BulletText("GPU: NVIDIA RTX 3080");
                ImGui::BulletText("RAM: 32 GB DDR5");
                ImGui::BulletText("DirectX Version: 12");
            }
        }
        EndGroupBox();

        ImGui::Spacing();

        // Contact & Support - Daha büyük kutu
        CustomGroupBox("Contact & Support", ImVec2(columnWidth, rightGroupHeight2));
        {
            ImGui::BulletText("Discord: geezydigital");
            ImGui::BulletText("Web: geezy.digital");
            ImGui::BulletText("Support Email: support@geezy.digital");
            ImGui::BulletText("Support Hours: 24/7");
            ImGui::BulletText("Response Time: <24 hours");

            ImGui::Spacing();
            ImGui::Text("Support Information");
            ImGui::TextWrapped("If you encounter any issues with the software, please contact our support team. We're available 24/7 to assist with any problems, bugs, or questions about features.");

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
            if (CustomButton("Visit Forum", ImVec2(columnWidth - 20, 25))) {
                // Visit forum
            }

            ImGui::Spacing();
            if (CustomButton("Discord Community", ImVec2(columnWidth - 20, 25))) {
                // Visit Discord
            }

            ImGui::Spacing();
            ImGui::Text("Changelog");
            ImGui::BeginChild("##Changelog", ImVec2(columnWidth - 20, 80), true);
            ImGui::Text("v1.0.0 - Initial Release (04/03/2025)");
            ImGui::BulletText("Added all core features");
            ImGui::BulletText("Optimized for CS2 latest update");
            ImGui::BulletText("Implemented modern UI with customization");
            ImGui::Text("v0.9.5 - Beta Release (25/02/2025)");
            ImGui::BulletText("Beta testing phase completed");
            ImGui::BulletText("Fixed several performance issues");
            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::Text("Status: %s", "Online");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "?");
        }
        EndGroupBox();

        ImGui::EndGroup();
    }

    // Account Sekmesi
    void Menu::RenderAccountTab() {
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float contentHeight = ImGui::GetContentRegionAvail().y;
        const float columnWidth = (contentWidth - 10) / 2; // Ýki sütun arasý 10px boþluk

        // Grup kutusu yüksekliklerini dinamik hesapla
        const float groupSpacing = 10.0f;

        // Sol sütun, iki kutu
        const float leftGroupHeight1 = contentHeight * 0.6f - groupSpacing / 2;
        const float leftGroupHeight2 = contentHeight * 0.4f - groupSpacing / 2;

        // Sað sütun, iki kutu
        const float rightGroupHeight1 = contentHeight * 0.55f - groupSpacing / 2;
        const float rightGroupHeight2 = contentHeight * 0.45f - groupSpacing / 2;

        // Sol Sütun
        ImGui::BeginGroup();

        // User Information - Büyük kutu
        CustomGroupBox("User Information", ImVec2(columnWidth, leftGroupHeight1));
        {
            static char username[64] = "GeezyUser";
            static char email[128] = "user@geezy.digital";
            static char subscriptionPlan = 'P'; // P: Premium, S: Standard
            static char licenseKey[64] = "GEEZY-XXXX-XXXX-XXXX";
            static char hwid[64] = "HWID-XXXX-XXXX-XXXX";

            // Kullanýcý profil kartý stili
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.13f, 0.15f, 0.8f));
            ImGui::BeginChild("##UserCard", ImVec2(columnWidth - 20, 100), true);

            // Kullanýcý avatar (daire içinde gösterilecek þekilde simüle edildi)
            ImGui::SetCursorPos(ImVec2(15, 15));
            float avatarSize = 70.0f;
            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2(ImGui::GetCursorScreenPos().x + avatarSize / 2, ImGui::GetCursorScreenPos().y + avatarSize / 2),
                avatarSize / 2, ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)));

            // Üyelik seviyesi
            ImGui::GetWindowDrawList()->AddCircle(
                ImVec2(ImGui::GetCursorScreenPos().x + avatarSize / 2, ImGui::GetCursorScreenPos().y + avatarSize / 2),
                avatarSize / 2 + 3, ImGui::GetColorU32(ImVec4(0.9f, 0.6f, 0.1f, 1.0f)), 0, 3.0f);

            // Kullanýcý bilgileri
            ImGui::SetCursorPos(ImVec2(100, 15));
            ImGui::Text("%s", username);

            ImGui::SetCursorPos(ImVec2(100, 35));
            ImGui::TextColored(
                subscriptionPlan == 'P' ? ImVec4(0.9f, 0.6f, 0.1f, 1.0f) : ImVec4(0.1f, 0.6f, 0.9f, 1.0f),
                "%s", subscriptionPlan == 'P' ? "Premium Member" : "Standard Member"
            );

            ImGui::SetCursorPos(ImVec2(100, 55));
            ImGui::Text("Member since: 01/01/2025");

            ImGui::SetCursorPos(ImVec2(100, 75));
            ImGui::Text("Status: Active");

            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Text("Email:");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("%s", email);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Text("License Key:");
            ImGui::InputText("##LicenseKey", licenseKey, IM_ARRAYSIZE(licenseKey), ImGuiInputTextFlags_ReadOnly);

            if (CustomButton("Copy", ImVec2(80, 25))) {
                // Copy function
            }
            ImGui::SameLine();
            if (CustomButton("Refresh", ImVec2(80, 25))) {
                // Refresh function
            }

            ImGui::Spacing();
            ImGui::Text("Hardware ID:");
            ImGui::InputText("##HWID", hwid, IM_ARRAYSIZE(hwid), ImGuiInputTextFlags_ReadOnly);

            if (CustomButton("Copy HWID", ImVec2(80, 25))) {
                // Copy HWID function
            }

            ImGui::Spacing();
            ImGui::Text("Account Statistics");

            static int loginCount = 47;
            static int totalHours = 342;
            static int reportCount = 0;
            static float trustFactor = 95.0f;

            ImGui::BulletText("Total Logins: %d", loginCount);
            ImGui::BulletText("Hours Used: %d", totalHours);
            ImGui::BulletText("Report Count: %d", reportCount);
            ImGui::BulletText("Trust Factor: %.1f%%", trustFactor);
        }
        EndGroupBox();

        ImGui::Spacing();

        // License Validation - Küçük kutu
        CustomGroupBox("License Management", ImVec2(columnWidth, leftGroupHeight2));
        {
            static char newLicenseKey[64] = "";
            ImGui::Text("Add New License Key:");
            ImGui::InputText("##NewLicenseKey", newLicenseKey, IM_ARRAYSIZE(newLicenseKey));

            if (CustomButton("Validate & Activate", ImVec2(150, 25))) {
                // Validation function
            }

            ImGui::Spacing();
            ImGui::Text("System Status:");

            ImGui::BeginTable("##StatusTable", 2);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Authentication:");
            ImGui::TableNextColumn();
            ImGui::Text("OK");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "License Status:");
            ImGui::TableNextColumn();
            ImGui::Text("Active");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "HWID Check:");
            ImGui::TableNextColumn();
            ImGui::Text("Verified");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Database:");
            ImGui::TableNextColumn();
            ImGui::Text("Connected");

            ImGui::EndTable();

            ImGui::Spacing();
            if (CustomButton("Reset HWID", ImVec2(150, 25))) {
                // HWID Reset function
            }
            ImGui::SameLine();
            if (CustomButton("Transfer License", ImVec2(150, 25))) {
                // License Transfer function
            }
        }
        EndGroupBox();

        ImGui::EndGroup();

        // Sað Sütun
        ImGui::SameLine(0, 10);
        ImGui::BeginGroup();

        // Subscription Details
        CustomGroupBox("Subscription Details", ImVec2(columnWidth, rightGroupHeight1));
        {
            // Test verileri
            static char startDate[32] = "01/01/2025";
            static char endDate[32] = "01/01/2026";
            static int daysLeft = 303;
            static bool autoRenew = false;

            // Abonelik durumu görsel gösterimi
            ImGui::Text("Subscription Status");

            // Ýlerleme çubuðu
            float progress = daysLeft / 365.0f;

            ImGui::ProgressBar(progress, ImVec2(columnWidth - 20, 15), "");

            // Metinler renk kodlu
            ImGui::Text("Time Remaining:");

            // Kalan gün sayýsýna göre renk
            ImGui::SameLine();
            ImGui::TextColored(
                daysLeft > 30 ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) :
                (daysLeft > 7 ? ImVec4(0.9f, 0.6f, 0.1f, 1.0f) : ImVec4(0.9f, 0.1f, 0.1f, 1.0f)),
                "%d days", daysLeft
            );

            ImGui::Spacing();

            ImGui::BeginTable("##SubscriptionDetails", 2);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Start Date:");
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", startDate);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("End Date:");
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", endDate);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Subscription Type:");
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.0f), "Premium");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Billing Cycle:");
            ImGui::TableNextColumn();
            ImGui::Text("Annual");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Next Charge:");
            ImGui::TableNextColumn();
            ImGui::Text("$99.99 on %s", endDate);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Payment Method:");
            ImGui::TableNextColumn();
            ImGui::Text("Visa ****1234");

            ImGui::EndTable();

            ImGui::Spacing();
            CustomCheckbox("Auto-Renew Subscription", &autoRenew);

            ImGui::Spacing();
            if (CustomButton("Renew Now", ImVec2(columnWidth * 0.48f - 5, 25))) {
                // Renew function
            }

            ImGui::SameLine();
            if (CustomButton("Change Plan", ImVec2(columnWidth * 0.48f - 5, 25))) {
                // Change plan function
            }

            ImGui::Spacing();
            if (CustomButton("Payment History", ImVec2(columnWidth - 10, 25))) {
                // Payment history function
            }
        }
        EndGroupBox();

        ImGui::Spacing();

        // Security Settings
        CustomGroupBox("Security Settings", ImVec2(columnWidth, rightGroupHeight2));
        {
            static bool twoFactorEnabled = false;
            CustomCheckbox("Two-Factor Authentication", &twoFactorEnabled);

            if (twoFactorEnabled) {
                ImGui::Spacing();
                // 2FA QR kod gösterimi (temsili)
                ImGui::SetCursorPosX((columnWidth - 100) / 2);
                ImGui::BeginGroup();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y),
                    ImVec2(ImGui::GetCursorScreenPos().x + 100, ImGui::GetCursorScreenPos().y + 100),
                    ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f)));

                // QR kod içeriði temsili
                for (int i = 0; i < 6; i++) {
                    for (int j = 0; j < 6; j++) {
                        if ((i + j) % 2 == 0) {
                            ImGui::GetWindowDrawList()->AddRectFilled(
                                ImVec2(ImGui::GetCursorScreenPos().x + i * 16, ImGui::GetCursorScreenPos().y + j * 16),
                                ImVec2(ImGui::GetCursorScreenPos().x + (i + 1) * 16, ImGui::GetCursorScreenPos().y + (j + 1) * 16),
                                ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f)));
                        }
                    }
                }
                ImGui::Dummy(ImVec2(100, 100));
                ImGui::EndGroup();

                ImGui::Spacing();
                static char backupCode[24] = "ABCD-EFGH-IJKL-MNOP";
                ImGui::Text("Backup Code:");
                ImGui::InputText("##BackupCode", backupCode, IM_ARRAYSIZE(backupCode), ImGuiInputTextFlags_ReadOnly);

                if (CustomButton("Copy Backup Code", ImVec2(150, 25))) {
                    // Copy backup code function
                }
            }

            ImGui::Spacing();
            static bool loginNotifications = false;
            CustomCheckbox("Login Notifications", &loginNotifications);

            ImGui::Spacing();
            static bool ipLock = false;
            CustomCheckbox("IP Lock", &ipLock);

            ImGui::Spacing();
            if (CustomButton("Change Password", ImVec2(columnWidth * 0.48f - 5, 25))) {
                // Password change function
            }

            ImGui::SameLine();
            if (CustomButton("Manage Sessions", ImVec2(columnWidth * 0.48f - 5, 25))) {
                // Session management function
            }

            ImGui::Spacing();
            if (CustomButton("Login History", ImVec2(columnWidth - 10, 25))) {
                // Login history function
            }
        }
        EndGroupBox();

        ImGui::EndGroup();
    }
}  // namespace ui