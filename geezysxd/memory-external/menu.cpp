#include "menu.hpp"
#include <iostream>
#include <utility>
#include <algorithm>
#include <thread>
#include <chrono>
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "../memory-external/imgui/backends/imgui_impl_dx11.h"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
// Global de�i�kenler (DirectX hook'lar� i�in)
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static WNDPROC g_originalWndProc = nullptr;
static geezy_digital::MenuSystem* g_menuSystem = nullptr;
static geezy_digital::D3D11PresentHook g_originalPresent = nullptr;
static geezy_digital::D3D11ResizeBuffersHook g_originalResizeBuffers = nullptr;

// Forward declaration
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace geezy_digital {

    // DirectX kancalar� i�in yard�mc� fonksiyon - Orijinal Present fonksiyonunu bul
    DWORD_PTR FindPattern(const char* module, const char* pattern, const char* mask) {
        MODULEINFO modInfo;
        GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(module), &modInfo, sizeof(MODULEINFO));

        DWORD_PTR base = (DWORD_PTR)modInfo.lpBaseOfDll;
        DWORD_PTR size = (DWORD_PTR)modInfo.SizeOfImage;

        DWORD_PTR patternLength = (DWORD_PTR)strlen(mask);

        for (DWORD_PTR i = 0; i < size - patternLength; i++) {
            bool found = true;
            for (DWORD_PTR j = 0; j < patternLength; j++) {
                found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
            }
            if (found) {
                return base + i;
            }
        }
        return 0;
    }

    // Render Target'� yeniden olu�tur
    void CreateRenderTarget() {
        DXGI_SWAP_CHAIN_DESC sd;
        g_pSwapChain->GetDesc(&sd);

        // Create the render target
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    // Render Target'� temizle
    void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    // DirectX Present fonksiyonu i�in kanca
    HRESULT __stdcall MenuSystem::GD_HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        static bool init = false;

        if (!init) {
            // D3D11 nesnelerini al
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);

            g_pSwapChain = pSwapChain;

            pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice);
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);

            CreateRenderTarget();

            // ImGui ba�lat
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui_ImplWin32_Init(sd.OutputWindow);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

            // Men� stilini ayarla (e�er MenuSystem nesnesi varsa)
            if (g_menuSystem) {
                g_menuSystem->GD_InitImGui();
            }

            init = true;
        }

        // Yeni frame ba�lat
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Men� g�r�n�rl�k kontrol� ve render
        if (g_menuSystem && g_menuSystem->GD_IsMenuVisible()) {
            ImGui::Begin("Geezy Digital CS2 Tool", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            // Tab bar ve i�erik burada �izilecek
            ImGui::Text("Menu implementation goes here");

            ImGui::End();
        }

        // ESP ve di�er 3D renderleme �zellikleri i�in callback
        if (g_menuSystem && g_menuSystem->m_onRenderCallback) {
            g_menuSystem->m_onRenderCallback();
        }

        // ImGui render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Orijinal Present metodunu �a��r
        return g_originalPresent(pSwapChain, SyncInterval, Flags);
    }

    // DirectX ResizeBuffers fonksiyonu i�in kanca
    HRESULT __stdcall MenuSystem::GD_HookResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        CleanupRenderTarget();
        HRESULT result = g_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        CreateRenderTarget();
        return result;
    }

    // WndProc i�in kanca
    LRESULT WINAPI MenuSystem::GD_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        // ImGui giri� kontrol�
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
            return true;

        // Orijinal WndProc metodunu �a��r
        return CallWindowProc(g_originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    // Men� Sisteminin Implementasyonu

    MenuSystem::~MenuSystem() {
        GD_Shutdown();
    }

    // Men� sistemini ba�lat
    bool MenuSystem::GD_Initialize(HWND hWnd, ConfigManager* configManager) {
        if (m_initialized)
            return true;

        m_window = hWnd;
        m_configManager = configManager;
        g_menuSystem = this;

        // Silah ve kaplama verilerini y�kle
        GD_LoadWeaponData();

        // DirectX hook'lar� kur
        if (!GD_HookDirectX()) {
            std::cerr << "DirectX hook'lar� kurulamad�!" << std::endl;
            return false;
        }

        // WndProc kancas�
        g_originalWndProc = (WNDPROC)SetWindowLongPtr(m_window, GWLP_WNDPROC, (LONG_PTR)GD_WndProc);

        m_initialized = true;
        return true;
    }

    // ImGui'yi haz�rla
    bool MenuSystem::GD_InitImGui() {
        GD_SetupStyle();
        return true;
    }

    // ImGui stili ayarla
    void MenuSystem::GD_SetupStyle() {
        ImGuiStyle& style = ImGui::GetStyle();

        // Men� temas�n� ayarla (koyu veya a��k)
        if (m_darkTheme) {
            ImGui::StyleColorsDark();
        }
        else {
            ImGui::StyleColorsLight();
        }

        // �zel stil ayarlar�
        style.WindowRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;

        // Renk �emas�
        ImVec4* colors = style.Colors;
        if (m_darkTheme) {
            colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 0.55f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

            // �zel aksent rengi
            colors[ImGuiCol_CheckMark] = m_accentColor;
            colors[ImGuiCol_SliderGrab] = m_accentColor;
            colors[ImGuiCol_SliderGrabActive] = ImVec4(m_accentColor.x * 1.2f, m_accentColor.y * 1.2f, m_accentColor.z * 1.2f, m_accentColor.w);
            colors[ImGuiCol_Button] = ImVec4(m_accentColor.x * 0.7f, m_accentColor.y * 0.7f, m_accentColor.z * 0.7f, m_accentColor.w * 0.6f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(m_accentColor.x * 0.9f, m_accentColor.y * 0.9f, m_accentColor.z * 0.9f, m_accentColor.w * 0.8f);
            colors[ImGuiCol_ButtonActive] = m_accentColor;
        }
        else {
            // A��k tema i�in renk ayarlar� buraya eklenebilir
        }

        // Font boyutunu ayarla
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.1f;
    }

    // DirectX hook'lar� kur
    bool MenuSystem::GD_HookDirectX() {
        DWORD_PTR presentAddr = 0;
        DWORD_PTR resizeBuffersAddr = 0;

        // VMT (Virtual Method Table) offsetleri
        const int PRESENT_OFFSET = 8;            // IDXGISwapChain::Present
        const int RESIZE_BUFFERS_OFFSET = 13;    // IDXGISwapChain::ResizeBuffers

        // DirectX DLL'ini lokasyon
        HMODULE d3d11Module = GetModuleHandleA("d3d11.dll");

        if (!d3d11Module) {
            std::cerr << "d3d11.dll mod�l� bulunamad�!" << std::endl;
            return false;
        }

        // DirectX fonksiyonlar�n�n adreslerini bulma
        // Not: Ger�ek bir implementasyonda daha karma��k pattern taramas� kullan�l�r

        // Bu k�s�m sadece �rnek - ger�ek bir uygulamada swap chain bulmak i�in kendi y�ntemlerinizi kullanman�z gerekir
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        ID3D11Device* pTempDevice = nullptr;
        ID3D11DeviceContext* pTempContext = nullptr;
        IDXGISwapChain* pTempSwapChain = nullptr;

        // Ge�ici bir DirectX cihaz� ve swap chain olu�tur
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = m_window;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;

        if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 1,
            D3D11_SDK_VERSION, &swapChainDesc, &pTempSwapChain, &pTempDevice, nullptr, &pTempContext)))
        {
            std::cerr << "DirectX cihaz� ve swap chain olu�turulamad�!" << std::endl;
            return false;
        }

        // Sanal tablo adresini al
        void** pVTable = *reinterpret_cast<void***>(pTempSwapChain);

        // Present ve ResizeBuffers fonksiyonlar�n�n adreslerini al
        presentAddr = (DWORD_PTR)pVTable[PRESENT_OFFSET];
        resizeBuffersAddr = (DWORD_PTR)pVTable[RESIZE_BUFFERS_OFFSET];

        // Hook'lar� kur
        g_originalPresent = (D3D11PresentHook)presentAddr;
        g_originalResizeBuffers = (D3D11ResizeBuffersHook)resizeBuffersAddr;

        // Hook kurulumu i�in sanal korumay� de�i�tir
        DWORD oldProtect;
        VirtualProtect((LPVOID)presentAddr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<D3D11PresentHook*>(presentAddr) = &GD_HookPresent;
        VirtualProtect((LPVOID)presentAddr, sizeof(void*), oldProtect, &oldProtect);

        VirtualProtect((LPVOID)resizeBuffersAddr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<D3D11ResizeBuffersHook*>(resizeBuffersAddr) = &GD_HookResizeBuffers;
        VirtualProtect((LPVOID)resizeBuffersAddr, sizeof(void*), oldProtect, &oldProtect);

        // Ge�ici kaynaklar� temizle
        if (pTempSwapChain) pTempSwapChain->Release();
        if (pTempContext) pTempContext->Release();
        if (pTempDevice) pTempDevice->Release();

        return true;
    }

    // Silah verileri y�kle
    void MenuSystem::GD_LoadWeaponData() {
        // Bu fonksiyon, desteklenen silahlar� ve kaplamalar�n� y�kler
        // Ger�ek bir uygulamada, bu veriler bir dosyadan okunabilir

        // �rnek silah: AK-47
        WeaponInfo ak47;
        ak47.id = 7;
        ak47.name = "AK-47";
        ak47.skins = {
            {180, "Fire Serpent"},
            {707, "Bloodsport"},
            {524, "Fuel Injector"},
            {302, "Vulcan"},
            {172, "Redline"},
            {639, "Neon Revolution"},
            {675, "The Empress"},
            {422, "Elite Build"}
        };
        m_weaponData.push_back(ak47);

        // �rnek silah: M4A4
        WeaponInfo m4a4;
        m4a4.id = 16;
        m4a4.name = "M4A4";
        m4a4.skins = {
            {255, "Howl"},
            {309, "Asiimov"},
            {588, "Neo-Noir"},
            {400, "Royal Paladin"},
            {155, "X-Ray"},
            {632, "Buzz Kill"},
            {480, "Poseidon"},
            {336, "Desert-Strike"}
        };
        m_weaponData.push_back(m4a4);

        // �rnek silah: AWP
        WeaponInfo awp;
        awp.id = 9;
        awp.name = "AWP";
        awp.skins = {
            {344, "Dragon Lore"},
            {279, "Asiimov"},
            {640, "Oni Taiji"},
            {736, "Neo-Noir"},
            {446, "Medusa"},
            {259, "Redline"},
            {662, "Fever Dream"},
            {475, "Hyper Beast"}
        };
        m_weaponData.push_back(awp);

        // Di�er silahlar da buraya eklenebilir
    }

    // Renk d�zenleme yard�mc� fonksiyonlar�
    void MenuSystem::GD_ColorEdit4(const char* label, GD_Color& color) {
        ImVec4 col = GD_ColorToImVec4(color);
        if (ImGui::ColorEdit4(label, (float*)&col, ImGuiColorEditFlags_AlphaBar)) {
            color = GD_ImVec4ToColor(col);
        }
    }

    ImVec4 MenuSystem::GD_ColorToImVec4(const GD_Color& color) {
        return ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    }

    GD_Color MenuSystem::GD_ImVec4ToColor(const ImVec4& color) {
        return GD_Color(
            static_cast<int>(color.x * 255.0f),
            static_cast<int>(color.y * 255.0f),
            static_cast<int>(color.z * 255.0f),
            static_cast<int>(color.w * 255.0f)
        );
    }

    // Men� i�eri�i render fonksiyonlar� (ana implementasyon)
    void MenuSystem::GD_RenderMainMenu() {
        // Men� ba�l���
        ImGui::Begin(m_menuTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Sekmeler
        if (ImGui::BeginTabBar("GeezyTabBar", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("ESP")) {
                m_currentTab = MenuTab::ESP;
                GD_RenderESPTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Glow")) {
                m_currentTab = MenuTab::GLOW;
                GD_RenderGlowTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Radar")) {
                m_currentTab = MenuTab::RADAR;
                GD_RenderRadarTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("FOV")) {
                m_currentTab = MenuTab::FOV;
                GD_RenderFOVTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Skins")) {
                m_currentTab = MenuTab::SKINS;
                GD_RenderSkinsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Misc")) {
                m_currentTab = MenuTab::MISC;
                GD_RenderMiscTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Config")) {
                m_currentTab = MenuTab::CONFIG;
                GD_RenderConfigTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("About")) {
                m_currentTab = MenuTab::ABOUT;
                GD_RenderAboutTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // ESP sekmesi
    void MenuSystem::GD_RenderESPTab() {
        if (!m_configManager) return;

        auto& espConfig = m_configManager->GetESPConfig();

        ImGui::Checkbox("ESP Aktif", &espConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("ESP �zellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Kutu G�ster", &espConfig.showBox);
            ImGui::Checkbox("Sa�l�k G�ster", &espConfig.showHealth);
            ImGui::Checkbox("�sim G�ster", &espConfig.showName);
            ImGui::Checkbox("Mesafe G�ster", &espConfig.showDistance);
            ImGui::Checkbox("Silah G�ster", &espConfig.showWeapons);
            ImGui::Checkbox("Tak�m Arkada�lar�n� G�ster", &espConfig.showTeammates);
            ImGui::Checkbox("D��manlar� G�ster", &espConfig.showEnemies);
        }

        if (ImGui::CollapsingHeader("ESP Renkleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            GD_ColorEdit4("D��man Rengi", espConfig.enemyColor);
            GD_ColorEdit4("Tak�m Arkada�� Rengi", espConfig.teammateColor);
            GD_ColorEdit4("Defuse Rengi", espConfig.defusingColor);
            GD_ColorEdit4("D���k Sa�l�k Rengi", espConfig.lowHealthColor);
        }

        if (ImGui::CollapsingHeader("ESP Ayarlar�", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Kutu Kal�nl���", &espConfig.boxThickness, 1, 5);
            ImGui::SliderInt("Yaz� Boyutu", &espConfig.textSize, 8, 24);
            ImGui::SliderFloat("Max Render Mesafesi", &espConfig.maxRenderDistance, 100.0f, 3000.0f, "%.0f");
        }
    }

    // Glow sekmesi
    void MenuSystem::GD_RenderGlowTab() {
        if (!m_configManager) return;

        auto& glowConfig = m_configManager->GetGlowConfig();

        ImGui::Checkbox("Glow Aktif", &glowConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Glow �zellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("D��manlar� G�ster", &glowConfig.showEnemies);
            ImGui::Checkbox("Tak�m Arkada�lar�n� G�ster", &glowConfig.showTeammates);
            ImGui::Checkbox("Silahlar� G�ster", &glowConfig.showWeapons);
            ImGui::Checkbox("Bombay� G�ster", &glowConfig.showBomb);
        }

        if (ImGui::CollapsingHeader("Glow Renkleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            GD_ColorEdit4("D��man Rengi", glowConfig.enemyColor);
            GD_ColorEdit4("Tak�m Arkada�� Rengi", glowConfig.teammateColor);
            GD_ColorEdit4("Silah Rengi", glowConfig.weaponColor);
            GD_ColorEdit4("Bomba Rengi", glowConfig.bombColor);
        }
    }

    // Radar sekmesi
    void MenuSystem::GD_RenderRadarTab() {
        if (!m_configManager) return;

        auto& radarConfig = m_configManager->GetRadarConfig();

        ImGui::Checkbox("Radar Aktif", &radarConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Radar �zellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("D��manlar� G�ster", &radarConfig.showEnemies);
            ImGui::Checkbox("Tak�m Arkada�lar�n� G�ster", &radarConfig.showTeammates);
            ImGui::Checkbox("Bombay� G�ster", &radarConfig.showBomb);
        }
    }

    // FOV sekmesi
    void MenuSystem::GD_RenderFOVTab() {
        if (!m_configManager) return;

        auto& fovConfig = m_configManager->GetFOVConfig();

        ImGui::Checkbox("FOV De�i�tirici Aktif", &fovConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("FOV Ayarlar�", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Varsay�lan FOV", &fovConfig.defaultFOV, 60.0f, 120.0f, "%.1f");
            ImGui::SliderFloat("�zel FOV", &fovConfig.customFOV, 60.0f, 150.0f, "%.1f");
            ImGui::Checkbox("Dinamik FOV", &fovConfig.dynamicFOV);

            if (fovConfig.dynamicFOV) {
                ImGui::SliderFloat("Zoom Fakt�r�", &fovConfig.zoomFactor, 0.1f, 1.0f, "%.2f");
            }
        }
    }

    // Skin de�i�tirici sekmesi
    void MenuSystem::GD_RenderSkinsTab() {
        if (!m_configManager) return;

        auto& skinConfig = m_configManager->GetSkinChangerConfig();

        ImGui::Checkbox("Skin De�i�tirici Aktif", &skinConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Skin Ayarlar�", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Sadece Spawn Olunca G�ncelle", &skinConfig.updateOnlyOnSpawn);
            ImGui::Checkbox("B��ak Skin De�i�tirici", &skinConfig.knifeSkinChanger);

            if (skinConfig.knifeSkinChanger) {
                // B��ak modeli se�ici
                if (ImGui::BeginCombo("B��ak Modeli", m_knifeModels[skinConfig.knifeModel].c_str())) {
                    for (const auto& knife : m_knifeModels) {
                        bool isSelected = (skinConfig.knifeModel == knife.first);
                        if (ImGui::Selectable(knife.second.c_str(), isSelected)) {
                            skinConfig.knifeModel = knife.first;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // B��ak skin ID'si
                ImGui::InputInt("B��ak Skin ID", &skinConfig.knifeSkin);
            }
        }

        if (ImGui::CollapsingHeader("Silah Skinleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Silah skin listesi burada y�netilecek
            ImGui::Text("Silah skin listesi");

            // �rnek bir skin konfigurasyon aray�z�
            static int selectedWeaponIndex = 0;

            if (ImGui::BeginCombo("Silah", m_weaponData[selectedWeaponIndex].name.c_str())) {
                for (size_t i = 0; i < m_weaponData.size(); i++) {
                    bool isSelected = (selectedWeaponIndex == i);
                    if (ImGui::Selectable(m_weaponData[i].name.c_str(), isSelected)) {
                        selectedWeaponIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Se�ilen silah�n skin konfig�rasyonu
            static int selectedSkinIndex = 0;
            const auto& selectedWeapon = m_weaponData[selectedWeaponIndex];

            if (ImGui::BeginCombo("Skin", selectedWeapon.skins[selectedSkinIndex].second.c_str())) {
                for (size_t i = 0; i < selectedWeapon.skins.size(); i++) {
                    bool isSelected = (selectedSkinIndex == i);
                    if (ImGui::Selectable(selectedWeapon.skins[i].second.c_str(), isSelected)) {
                        selectedSkinIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Skin parametreleri
            static float wear = 0.01f;
            static int seed = 0;
            static int statTrak = -1;
            static char nameTag[32] = "";

            ImGui::SliderFloat("A��nma", &wear, 0.0f, 1.0f, "%.3f");
            ImGui::InputInt("Desen Tohumu", &seed);
            ImGui::InputInt("StatTrak (-1 = kapal�)", &statTrak);
            ImGui::InputText("�sim Etiketi", nameTag, sizeof(nameTag));

            if (ImGui::Button("Ekle/G�ncelle")) {
                // Skin konfig�rasyonunu listeye ekle/g�ncelle
                const int weaponID = selectedWeapon.id;
                const int paintKit = selectedWeapon.skins[selectedSkinIndex].first;

                // Mevcut skin'i g�ncelle veya yeni ekle
                bool found = false;
                for (auto& skin : skinConfig.weaponSkins) {
                    if (skin.weaponID == weaponID) {
                        skin.paintKit = paintKit;
                        skin.wear = wear;
                        skin.seed = seed;
                        skin.statTrak = statTrak;
                        skin.nameTag = nameTag;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    GD_WeaponSkin newSkin;
                    newSkin.weaponID = weaponID;
                    newSkin.paintKit = paintKit;
                    newSkin.wear = wear;
                    newSkin.seed = seed;
                    newSkin.statTrak = statTrak;
                    newSkin.nameTag = nameTag;
                    skinConfig.weaponSkins.push_back(newSkin);
                }
            }

            // Yap�land�r�lm�� skinlerin listesi
            ImGui::Separator();
            ImGui::Text("Yap�land�r�lm�� Skinler");

            for (size_t i = 0; i < skinConfig.weaponSkins.size(); i++) {
                const auto& skin = skinConfig.weaponSkins[i];

                // Silah ad�n� bul
                std::string weaponName = "Unknown";
                for (const auto& weapon : m_weaponData) {
                    if (weapon.id == skin.weaponID) {
                        weaponName = weapon.name;
                        break;
                    }
                }

                // Skin ad�n� bul
                std::string skinName = "Unknown";
                for (const auto& weapon : m_weaponData) {
                    if (weapon.id == skin.weaponID) {
                        for (const auto& s : weapon.skins) {
                            if (s.first == skin.paintKit) {
                                skinName = s.second;
                                break;
                            }
                        }
                        break;
                    }
                }

                // Liste ��esi
                std::string itemLabel = weaponName + " | " + skinName;
                if (ImGui::Selectable(itemLabel.c_str())) {
                    // Se�ilen skini d�zenleme i�in y�kle
                    for (size_t w = 0; w < m_weaponData.size(); w++) {
                        if (m_weaponData[w].id == skin.weaponID) {
                            selectedWeaponIndex = w;

                            for (size_t s = 0; s < m_weaponData[w].skins.size(); s++) {
                                if (m_weaponData[w].skins[s].first == skin.paintKit) {
                                    selectedSkinIndex = s;
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    wear = skin.wear;
                    seed = skin.seed;
                    statTrak = skin.statTrak;
                    strcpy_s(nameTag, sizeof(nameTag), skin.nameTag.c_str());
                }

                // Silme butonu
                ImGui::SameLine();
                std::string buttonLabel = "X##" + std::to_string(i);
                if (ImGui::Button(buttonLabel.c_str())) {
                    skinConfig.weaponSkins.erase(skinConfig.weaponSkins.begin() + i);
                    i--; // Indeksi bir azalt, ��nk� eleman silindi
                }
            }
        }
    }

    // �e�itli ayarlar sekmesi
    void MenuSystem::GD_RenderMiscTab() {
        if (!m_configManager) return;

        // Tema ayarlar�
        if (ImGui::CollapsingHeader("Men� Ayarlar�", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool themeChanged = false;

            themeChanged |= ImGui::Checkbox("Koyu Tema", &m_darkTheme);

            ImGui::Text("Aksent Rengi");
            themeChanged |= ImGui::ColorEdit3("##AccentColor", (float*)&m_accentColor, ImGuiColorEditFlags_NoInputs);

            if (themeChanged) {
                GD_SetupStyle();
            }
        }

        // Tu� atamalar�
        if (ImGui::CollapsingHeader("Tu� Bindlar�", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& keyBindings = m_configManager->GetKeyBindings();

            // Tu� atama UI'lar�
            ImGui::Text("ESP A�/Kapat:");
            ImGui::SameLine();
            ImGui::Text("F1 (VK: %d)", keyBindings.toggleEspKey);

            ImGui::Text("Men� A�/Kapat:");
            ImGui::SameLine();
            ImGui::Text("INSERT (VK: %d)", keyBindings.toggleMenuKey);

            ImGui::Text("Programdan ��k:");
            ImGui::SameLine();
            ImGui::Text("END (VK: %d)", keyBindings.exitKey);
        }

        if (ImGui::CollapsingHeader("Geli�mi� Ayarlar", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Bu k�s�m hen�z uygulanmad�.");
        }
    }

    // Konfig�rasyon sekmesi
    void MenuSystem::GD_RenderConfigTab() {
        if (!m_configManager) return;

        ImGui::Text("Yap�land�rma Y�netimi");
        ImGui::Separator();

        if (ImGui::Button("Yap�land�rmay� Kaydet")) {
            if (m_configManager->GD_SaveConfig()) {
                ImGui::OpenPopup("SavedPopup");
            }
            else {
                ImGui::OpenPopup("SaveErrorPopup");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Yap�land�rmay� Y�kle")) {
            if (m_configManager->GD_LoadConfig()) {
                ImGui::OpenPopup("LoadedPopup");
            }
            else {
                ImGui::OpenPopup("LoadErrorPopup");
            }
        }

        // Kaydetme ba�ar�l� popup
        if (ImGui::BeginPopupModal("SavedPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yap�land�rma ba�ar�yla kaydedildi!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Kaydetme hatas� popup
        if (ImGui::BeginPopupModal("SaveErrorPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yap�land�rma kaydedilirken hata olu�tu!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Y�kleme ba�ar�l� popup
        if (ImGui::BeginPopupModal("LoadedPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yap�land�rma ba�ar�yla y�klendi!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Y�kleme hatas� popup
        if (ImGui::BeginPopupModal("LoadErrorPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yap�land�rma y�klenirken hata olu�tu!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text("Varsay�lan Ayarlar");

        if (ImGui::Button("T�m Ayarlar� S�f�rla")) {
            ImGui::OpenPopup("ResetConfirmPopup");
        }

        // Reset onay popup
        if (ImGui::BeginPopupModal("ResetConfirmPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Bu i�lem t�m ayarlar� varsay�lanlara s�f�rlayacakt�r.");
            ImGui::Text("Emin misiniz?");

            if (ImGui::Button("Evet")) {
                // Yeni bir ConfigManager �rne�i olu�turarak varsay�lan de�erleri al
                geezy_digital::ConfigManager defaultConfig;

                // Mevcut offset'leri koru
                auto offsets = m_configManager->GetOffsets();

                // Varsay�lan de�erleri mevcut config'e kopyala
                *m_configManager = defaultConfig;

                // Offset'leri geri y�kle
                m_configManager->GetOffsets() = offsets;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Hay�r")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    // Hakk�nda sekmesi
    void MenuSystem::GD_RenderAboutTab() {
        ImGui::Text("Geezy Digital CS2 Tool");
        ImGui::Text("Versiyon: 1.0.0");
        ImGui::Separator();

        ImGui::Text("Bu tool, CS2 i�in etik raporlama amac�yla geli�tirilmi�tir.");
        ImGui::Text("Bug bounty programlar� �er�evesinde kullan�lmak i�indir.");

        ImGui::Separator();

        ImGui::Text("�zellikler:");
        ImGui::BulletText("ESP (Wall-hack)");
        ImGui::BulletText("Glow Effect");
        ImGui::BulletText("Radar Hack");
        ImGui::BulletText("FOV De�i�tirici");
        ImGui::BulletText("Skin Changer");

        ImGui::Separator();

        if (m_configManager && m_configManager->AreOffsetsLoaded()) {
            ImGui::Text("Game Build: %u", m_configManager->GetOffsets().build_number);
        }

        ImGui::Text("Geli�tirici: Geezy Digital");
    }

    // Kaynaklar� temizle
    void MenuSystem::GD_Shutdown() {
        // ImGui temizleme
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        // DirectX kaynaklar� temizle
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }

        // Kancalar� kald�r
        if (m_window && g_originalWndProc) {
            SetWindowLongPtr(m_window, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
        }

        // De�i�kenleri s�f�rla
        m_window = NULL;
        m_initialized = false;
        m_configManager = nullptr;
        g_menuSystem = nullptr;
    }

} // namespace geezy_digital