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
// Global deðiþkenler (DirectX hook'larý için)
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

    // DirectX kancalarý için yardýmcý fonksiyon - Orijinal Present fonksiyonunu bul
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

    // Render Target'ý yeniden oluþtur
    void CreateRenderTarget() {
        DXGI_SWAP_CHAIN_DESC sd;
        g_pSwapChain->GetDesc(&sd);

        // Create the render target
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    // Render Target'ý temizle
    void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    // DirectX Present fonksiyonu için kanca
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

            // ImGui baþlat
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui_ImplWin32_Init(sd.OutputWindow);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

            // Menü stilini ayarla (eðer MenuSystem nesnesi varsa)
            if (g_menuSystem) {
                g_menuSystem->GD_InitImGui();
            }

            init = true;
        }

        // Yeni frame baþlat
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Menü görünürlük kontrolü ve render
        if (g_menuSystem && g_menuSystem->GD_IsMenuVisible()) {
            ImGui::Begin("Geezy Digital CS2 Tool", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            // Tab bar ve içerik burada çizilecek
            ImGui::Text("Menu implementation goes here");

            ImGui::End();
        }

        // ESP ve diðer 3D renderleme özellikleri için callback
        if (g_menuSystem && g_menuSystem->m_onRenderCallback) {
            g_menuSystem->m_onRenderCallback();
        }

        // ImGui render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Orijinal Present metodunu çaðýr
        return g_originalPresent(pSwapChain, SyncInterval, Flags);
    }

    // DirectX ResizeBuffers fonksiyonu için kanca
    HRESULT __stdcall MenuSystem::GD_HookResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        CleanupRenderTarget();
        HRESULT result = g_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        CreateRenderTarget();
        return result;
    }

    // WndProc için kanca
    LRESULT WINAPI MenuSystem::GD_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        // ImGui giriþ kontrolü
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
            return true;

        // Orijinal WndProc metodunu çaðýr
        return CallWindowProc(g_originalWndProc, hWnd, uMsg, wParam, lParam);
    }

    // Menü Sisteminin Implementasyonu

    MenuSystem::~MenuSystem() {
        GD_Shutdown();
    }

    // Menü sistemini baþlat
    bool MenuSystem::GD_Initialize(HWND hWnd, ConfigManager* configManager) {
        if (m_initialized)
            return true;

        m_window = hWnd;
        m_configManager = configManager;
        g_menuSystem = this;

        // Silah ve kaplama verilerini yükle
        GD_LoadWeaponData();

        // DirectX hook'larý kur
        if (!GD_HookDirectX()) {
            std::cerr << "DirectX hook'larý kurulamadý!" << std::endl;
            return false;
        }

        // WndProc kancasý
        g_originalWndProc = (WNDPROC)SetWindowLongPtr(m_window, GWLP_WNDPROC, (LONG_PTR)GD_WndProc);

        m_initialized = true;
        return true;
    }

    // ImGui'yi hazýrla
    bool MenuSystem::GD_InitImGui() {
        GD_SetupStyle();
        return true;
    }

    // ImGui stili ayarla
    void MenuSystem::GD_SetupStyle() {
        ImGuiStyle& style = ImGui::GetStyle();

        // Menü temasýný ayarla (koyu veya açýk)
        if (m_darkTheme) {
            ImGui::StyleColorsDark();
        }
        else {
            ImGui::StyleColorsLight();
        }

        // Özel stil ayarlarý
        style.WindowRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;

        // Renk þemasý
        ImVec4* colors = style.Colors;
        if (m_darkTheme) {
            colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 0.55f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

            // Özel aksent rengi
            colors[ImGuiCol_CheckMark] = m_accentColor;
            colors[ImGuiCol_SliderGrab] = m_accentColor;
            colors[ImGuiCol_SliderGrabActive] = ImVec4(m_accentColor.x * 1.2f, m_accentColor.y * 1.2f, m_accentColor.z * 1.2f, m_accentColor.w);
            colors[ImGuiCol_Button] = ImVec4(m_accentColor.x * 0.7f, m_accentColor.y * 0.7f, m_accentColor.z * 0.7f, m_accentColor.w * 0.6f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(m_accentColor.x * 0.9f, m_accentColor.y * 0.9f, m_accentColor.z * 0.9f, m_accentColor.w * 0.8f);
            colors[ImGuiCol_ButtonActive] = m_accentColor;
        }
        else {
            // Açýk tema için renk ayarlarý buraya eklenebilir
        }

        // Font boyutunu ayarla
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.1f;
    }

    // DirectX hook'larý kur
    bool MenuSystem::GD_HookDirectX() {
        DWORD_PTR presentAddr = 0;
        DWORD_PTR resizeBuffersAddr = 0;

        // VMT (Virtual Method Table) offsetleri
        const int PRESENT_OFFSET = 8;            // IDXGISwapChain::Present
        const int RESIZE_BUFFERS_OFFSET = 13;    // IDXGISwapChain::ResizeBuffers

        // DirectX DLL'ini lokasyon
        HMODULE d3d11Module = GetModuleHandleA("d3d11.dll");

        if (!d3d11Module) {
            std::cerr << "d3d11.dll modülü bulunamadý!" << std::endl;
            return false;
        }

        // DirectX fonksiyonlarýnýn adreslerini bulma
        // Not: Gerçek bir implementasyonda daha karmaþýk pattern taramasý kullanýlýr

        // Bu kýsým sadece örnek - gerçek bir uygulamada swap chain bulmak için kendi yöntemlerinizi kullanmanýz gerekir
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        ID3D11Device* pTempDevice = nullptr;
        ID3D11DeviceContext* pTempContext = nullptr;
        IDXGISwapChain* pTempSwapChain = nullptr;

        // Geçici bir DirectX cihazý ve swap chain oluþtur
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
            std::cerr << "DirectX cihazý ve swap chain oluþturulamadý!" << std::endl;
            return false;
        }

        // Sanal tablo adresini al
        void** pVTable = *reinterpret_cast<void***>(pTempSwapChain);

        // Present ve ResizeBuffers fonksiyonlarýnýn adreslerini al
        presentAddr = (DWORD_PTR)pVTable[PRESENT_OFFSET];
        resizeBuffersAddr = (DWORD_PTR)pVTable[RESIZE_BUFFERS_OFFSET];

        // Hook'larý kur
        g_originalPresent = (D3D11PresentHook)presentAddr;
        g_originalResizeBuffers = (D3D11ResizeBuffersHook)resizeBuffersAddr;

        // Hook kurulumu için sanal korumayý deðiþtir
        DWORD oldProtect;
        VirtualProtect((LPVOID)presentAddr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<D3D11PresentHook*>(presentAddr) = &GD_HookPresent;
        VirtualProtect((LPVOID)presentAddr, sizeof(void*), oldProtect, &oldProtect);

        VirtualProtect((LPVOID)resizeBuffersAddr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<D3D11ResizeBuffersHook*>(resizeBuffersAddr) = &GD_HookResizeBuffers;
        VirtualProtect((LPVOID)resizeBuffersAddr, sizeof(void*), oldProtect, &oldProtect);

        // Geçici kaynaklarý temizle
        if (pTempSwapChain) pTempSwapChain->Release();
        if (pTempContext) pTempContext->Release();
        if (pTempDevice) pTempDevice->Release();

        return true;
    }

    // Silah verileri yükle
    void MenuSystem::GD_LoadWeaponData() {
        // Bu fonksiyon, desteklenen silahlarý ve kaplamalarýný yükler
        // Gerçek bir uygulamada, bu veriler bir dosyadan okunabilir

        // Örnek silah: AK-47
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

        // Örnek silah: M4A4
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

        // Örnek silah: AWP
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

        // Diðer silahlar da buraya eklenebilir
    }

    // Renk düzenleme yardýmcý fonksiyonlarý
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

    // Menü içeriði render fonksiyonlarý (ana implementasyon)
    void MenuSystem::GD_RenderMainMenu() {
        // Menü baþlýðý
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

        if (ImGui::CollapsingHeader("ESP Özellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Kutu Göster", &espConfig.showBox);
            ImGui::Checkbox("Saðlýk Göster", &espConfig.showHealth);
            ImGui::Checkbox("Ýsim Göster", &espConfig.showName);
            ImGui::Checkbox("Mesafe Göster", &espConfig.showDistance);
            ImGui::Checkbox("Silah Göster", &espConfig.showWeapons);
            ImGui::Checkbox("Takým Arkadaþlarýný Göster", &espConfig.showTeammates);
            ImGui::Checkbox("Düþmanlarý Göster", &espConfig.showEnemies);
        }

        if (ImGui::CollapsingHeader("ESP Renkleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            GD_ColorEdit4("Düþman Rengi", espConfig.enemyColor);
            GD_ColorEdit4("Takým Arkadaþý Rengi", espConfig.teammateColor);
            GD_ColorEdit4("Defuse Rengi", espConfig.defusingColor);
            GD_ColorEdit4("Düþük Saðlýk Rengi", espConfig.lowHealthColor);
        }

        if (ImGui::CollapsingHeader("ESP Ayarlarý", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Kutu Kalýnlýðý", &espConfig.boxThickness, 1, 5);
            ImGui::SliderInt("Yazý Boyutu", &espConfig.textSize, 8, 24);
            ImGui::SliderFloat("Max Render Mesafesi", &espConfig.maxRenderDistance, 100.0f, 3000.0f, "%.0f");
        }
    }

    // Glow sekmesi
    void MenuSystem::GD_RenderGlowTab() {
        if (!m_configManager) return;

        auto& glowConfig = m_configManager->GetGlowConfig();

        ImGui::Checkbox("Glow Aktif", &glowConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Glow Özellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Düþmanlarý Göster", &glowConfig.showEnemies);
            ImGui::Checkbox("Takým Arkadaþlarýný Göster", &glowConfig.showTeammates);
            ImGui::Checkbox("Silahlarý Göster", &glowConfig.showWeapons);
            ImGui::Checkbox("Bombayý Göster", &glowConfig.showBomb);
        }

        if (ImGui::CollapsingHeader("Glow Renkleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            GD_ColorEdit4("Düþman Rengi", glowConfig.enemyColor);
            GD_ColorEdit4("Takým Arkadaþý Rengi", glowConfig.teammateColor);
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

        if (ImGui::CollapsingHeader("Radar Özellikleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Düþmanlarý Göster", &radarConfig.showEnemies);
            ImGui::Checkbox("Takým Arkadaþlarýný Göster", &radarConfig.showTeammates);
            ImGui::Checkbox("Bombayý Göster", &radarConfig.showBomb);
        }
    }

    // FOV sekmesi
    void MenuSystem::GD_RenderFOVTab() {
        if (!m_configManager) return;

        auto& fovConfig = m_configManager->GetFOVConfig();

        ImGui::Checkbox("FOV Deðiþtirici Aktif", &fovConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("FOV Ayarlarý", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Varsayýlan FOV", &fovConfig.defaultFOV, 60.0f, 120.0f, "%.1f");
            ImGui::SliderFloat("Özel FOV", &fovConfig.customFOV, 60.0f, 150.0f, "%.1f");
            ImGui::Checkbox("Dinamik FOV", &fovConfig.dynamicFOV);

            if (fovConfig.dynamicFOV) {
                ImGui::SliderFloat("Zoom Faktörü", &fovConfig.zoomFactor, 0.1f, 1.0f, "%.2f");
            }
        }
    }

    // Skin deðiþtirici sekmesi
    void MenuSystem::GD_RenderSkinsTab() {
        if (!m_configManager) return;

        auto& skinConfig = m_configManager->GetSkinChangerConfig();

        ImGui::Checkbox("Skin Deðiþtirici Aktif", &skinConfig.enabled);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Skin Ayarlarý", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Sadece Spawn Olunca Güncelle", &skinConfig.updateOnlyOnSpawn);
            ImGui::Checkbox("Býçak Skin Deðiþtirici", &skinConfig.knifeSkinChanger);

            if (skinConfig.knifeSkinChanger) {
                // Býçak modeli seçici
                if (ImGui::BeginCombo("Býçak Modeli", m_knifeModels[skinConfig.knifeModel].c_str())) {
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

                // Býçak skin ID'si
                ImGui::InputInt("Býçak Skin ID", &skinConfig.knifeSkin);
            }
        }

        if (ImGui::CollapsingHeader("Silah Skinleri", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Silah skin listesi burada yönetilecek
            ImGui::Text("Silah skin listesi");

            // Örnek bir skin konfigurasyon arayüzü
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

            // Seçilen silahýn skin konfigürasyonu
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

            ImGui::SliderFloat("Aþýnma", &wear, 0.0f, 1.0f, "%.3f");
            ImGui::InputInt("Desen Tohumu", &seed);
            ImGui::InputInt("StatTrak (-1 = kapalý)", &statTrak);
            ImGui::InputText("Ýsim Etiketi", nameTag, sizeof(nameTag));

            if (ImGui::Button("Ekle/Güncelle")) {
                // Skin konfigürasyonunu listeye ekle/güncelle
                const int weaponID = selectedWeapon.id;
                const int paintKit = selectedWeapon.skins[selectedSkinIndex].first;

                // Mevcut skin'i güncelle veya yeni ekle
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

            // Yapýlandýrýlmýþ skinlerin listesi
            ImGui::Separator();
            ImGui::Text("Yapýlandýrýlmýþ Skinler");

            for (size_t i = 0; i < skinConfig.weaponSkins.size(); i++) {
                const auto& skin = skinConfig.weaponSkins[i];

                // Silah adýný bul
                std::string weaponName = "Unknown";
                for (const auto& weapon : m_weaponData) {
                    if (weapon.id == skin.weaponID) {
                        weaponName = weapon.name;
                        break;
                    }
                }

                // Skin adýný bul
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

                // Liste öðesi
                std::string itemLabel = weaponName + " | " + skinName;
                if (ImGui::Selectable(itemLabel.c_str())) {
                    // Seçilen skini düzenleme için yükle
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
                    i--; // Indeksi bir azalt, çünkü eleman silindi
                }
            }
        }
    }

    // Çeþitli ayarlar sekmesi
    void MenuSystem::GD_RenderMiscTab() {
        if (!m_configManager) return;

        // Tema ayarlarý
        if (ImGui::CollapsingHeader("Menü Ayarlarý", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool themeChanged = false;

            themeChanged |= ImGui::Checkbox("Koyu Tema", &m_darkTheme);

            ImGui::Text("Aksent Rengi");
            themeChanged |= ImGui::ColorEdit3("##AccentColor", (float*)&m_accentColor, ImGuiColorEditFlags_NoInputs);

            if (themeChanged) {
                GD_SetupStyle();
            }
        }

        // Tuþ atamalarý
        if (ImGui::CollapsingHeader("Tuþ Bindlarý", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& keyBindings = m_configManager->GetKeyBindings();

            // Tuþ atama UI'larý
            ImGui::Text("ESP Aç/Kapat:");
            ImGui::SameLine();
            ImGui::Text("F1 (VK: %d)", keyBindings.toggleEspKey);

            ImGui::Text("Menü Aç/Kapat:");
            ImGui::SameLine();
            ImGui::Text("INSERT (VK: %d)", keyBindings.toggleMenuKey);

            ImGui::Text("Programdan Çýk:");
            ImGui::SameLine();
            ImGui::Text("END (VK: %d)", keyBindings.exitKey);
        }

        if (ImGui::CollapsingHeader("Geliþmiþ Ayarlar", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Bu kýsým henüz uygulanmadý.");
        }
    }

    // Konfigürasyon sekmesi
    void MenuSystem::GD_RenderConfigTab() {
        if (!m_configManager) return;

        ImGui::Text("Yapýlandýrma Yönetimi");
        ImGui::Separator();

        if (ImGui::Button("Yapýlandýrmayý Kaydet")) {
            if (m_configManager->GD_SaveConfig()) {
                ImGui::OpenPopup("SavedPopup");
            }
            else {
                ImGui::OpenPopup("SaveErrorPopup");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Yapýlandýrmayý Yükle")) {
            if (m_configManager->GD_LoadConfig()) {
                ImGui::OpenPopup("LoadedPopup");
            }
            else {
                ImGui::OpenPopup("LoadErrorPopup");
            }
        }

        // Kaydetme baþarýlý popup
        if (ImGui::BeginPopupModal("SavedPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yapýlandýrma baþarýyla kaydedildi!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Kaydetme hatasý popup
        if (ImGui::BeginPopupModal("SaveErrorPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yapýlandýrma kaydedilirken hata oluþtu!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Yükleme baþarýlý popup
        if (ImGui::BeginPopupModal("LoadedPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yapýlandýrma baþarýyla yüklendi!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Yükleme hatasý popup
        if (ImGui::BeginPopupModal("LoadErrorPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Yapýlandýrma yüklenirken hata oluþtu!");
            if (ImGui::Button("Tamam")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text("Varsayýlan Ayarlar");

        if (ImGui::Button("Tüm Ayarlarý Sýfýrla")) {
            ImGui::OpenPopup("ResetConfirmPopup");
        }

        // Reset onay popup
        if (ImGui::BeginPopupModal("ResetConfirmPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Bu iþlem tüm ayarlarý varsayýlanlara sýfýrlayacaktýr.");
            ImGui::Text("Emin misiniz?");

            if (ImGui::Button("Evet")) {
                // Yeni bir ConfigManager örneði oluþturarak varsayýlan deðerleri al
                geezy_digital::ConfigManager defaultConfig;

                // Mevcut offset'leri koru
                auto offsets = m_configManager->GetOffsets();

                // Varsayýlan deðerleri mevcut config'e kopyala
                *m_configManager = defaultConfig;

                // Offset'leri geri yükle
                m_configManager->GetOffsets() = offsets;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Hayýr")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    // Hakkýnda sekmesi
    void MenuSystem::GD_RenderAboutTab() {
        ImGui::Text("Geezy Digital CS2 Tool");
        ImGui::Text("Versiyon: 1.0.0");
        ImGui::Separator();

        ImGui::Text("Bu tool, CS2 için etik raporlama amacýyla geliþtirilmiþtir.");
        ImGui::Text("Bug bounty programlarý çerçevesinde kullanýlmak içindir.");

        ImGui::Separator();

        ImGui::Text("Özellikler:");
        ImGui::BulletText("ESP (Wall-hack)");
        ImGui::BulletText("Glow Effect");
        ImGui::BulletText("Radar Hack");
        ImGui::BulletText("FOV Deðiþtirici");
        ImGui::BulletText("Skin Changer");

        ImGui::Separator();

        if (m_configManager && m_configManager->AreOffsetsLoaded()) {
            ImGui::Text("Game Build: %u", m_configManager->GetOffsets().build_number);
        }

        ImGui::Text("Geliþtirici: Geezy Digital");
    }

    // Kaynaklarý temizle
    void MenuSystem::GD_Shutdown() {
        // ImGui temizleme
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        // DirectX kaynaklarý temizle
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }

        // Kancalarý kaldýr
        if (m_window && g_originalWndProc) {
            SetWindowLongPtr(m_window, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
        }

        // Deðiþkenleri sýfýrla
        m_window = NULL;
        m_initialized = false;
        m_configManager = nullptr;
        g_menuSystem = nullptr;
    }

} // namespace geezy_digital