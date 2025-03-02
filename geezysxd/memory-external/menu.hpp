#ifndef _GEEZY_DIGITAL_MENU_HPP_
#define _GEEZY_DIGITAL_MENU_HPP_

#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include "imgui.h"
#include "../memory-external/imgui/backends/imgui_impl_dx11.h"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
#include "config.hpp"

// D3D11 baðlantýlarý için gerekli tanýmlar
typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* D3D11ResizeBuffersHook) (IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace geezy_digital {

    class MenuSystem {
    private:
        // Pencere ve DirectX nesneleri
        HWND m_window = NULL;
        ID3D11Device* m_pDevice = nullptr;
        ID3D11DeviceContext* m_pContext = nullptr;
        IDXGISwapChain* m_pSwapChain = nullptr;
        ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

        // Orijinal WndProc ve kancalý fonksiyonlar
        WNDPROC m_originalWndProc = nullptr;
        D3D11PresentHook m_originalPresent = nullptr;
        D3D11ResizeBuffersHook m_originalResizeBuffers = nullptr;

        // Menü durumu
        bool m_showMenu = false;
        bool m_initialized = false;
        std::string m_menuTitle = "Geezy Digital CS2 Tool";

        // Konfigürasyon yöneticisi
        ConfigManager* m_configManager = nullptr;

        // Menü sekmeleri için tanýmlar
        enum class MenuTab {
            ESP,
            GLOW,
            RADAR,
            FOV,
            SKINS,
            MISC,
            CONFIG,
            ABOUT
        };

        MenuTab m_currentTab = MenuTab::ESP;

        // Tema ayarlarý
        bool m_darkTheme = true;
        ImVec4 m_accentColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

        // Callback fonksiyonlarý
        std::function<void()> m_onRenderCallback = nullptr;

        // Silah ve kaplama verileri
        struct WeaponInfo {
            int id;
            std::string name;
            std::vector<std::pair<int, std::string>> skins;
        };

        std::vector<WeaponInfo> m_weaponData;

        // Desteklenen CS2 býçak modelleri
        std::map<int, std::string> m_knifeModels = {
            {0, "Default"},
            {507, "Karambit"},
            {508, "M9 Bayonet"},
            {500, "Bayonet"},
            {505, "Flip Knife"},
            {506, "Gut Knife"},
            {509, "Huntsman Knife"},
            {512, "Falchion Knife"},
            {514, "Bowie Knife"},
            {515, "Butterfly Knife"},
            {516, "Shadow Daggers"},
            {520, "Navaja Knife"},
            {522, "Stiletto Knife"},
            {519, "Ursus Knife"},
            {521, "Talon Knife"},
            {517, "Paracord Knife"},
            {518, "Survival Knife"},
            {523, "Nomad Knife"},
            {525, "Skeleton Knife"},
            {503, "Classic Knife"}
        };

        // Renk kartelasý
        const std::vector<ImVec4> m_colorPalette = {
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Kýrmýzý
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Yeþil
            ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Mavi
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Sarý
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),  // Magenta
            ImVec4(0.0f, 1.0f, 1.0f, 1.0f),  // Cyan
            ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Turuncu
            ImVec4(0.5f, 0.0f, 1.0f, 1.0f),  // Mor
            ImVec4(0.0f, 0.5f, 0.0f, 1.0f),  // Koyu yeþil
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f)   // Gri
        };

        // Silah verileri yükle
        void GD_LoadWeaponData();

        // ImGui stili ayarla
        void GD_SetupStyle();

        // Menü içeriðini oluþtur
        void GD_RenderMainMenu();
        void GD_RenderESPTab();
        void GD_RenderGlowTab();
        void GD_RenderRadarTab();
        void GD_RenderFOVTab();
        void GD_RenderSkinsTab();
        void GD_RenderMiscTab();
        void GD_RenderConfigTab();
        void GD_RenderAboutTab();

        // Helper fonksiyonlarý
        void GD_ColorEdit4(const char* label, GD_Color& color);
        ImVec4 GD_ColorToImVec4(const GD_Color& color);
        GD_Color GD_ImVec4ToColor(const ImVec4& color);

    public:
        MenuSystem() = default;
        ~MenuSystem();

        // Menü sistemini baþlat
        bool GD_Initialize(HWND hWnd, ConfigManager* configManager);

        // ImGui'yi hazýrla
        bool GD_InitImGui();

        // DirectX hook'larý kur
        bool GD_HookDirectX();

        // D3D11 için kancalar
        static HRESULT __stdcall GD_HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT __stdcall GD_HookResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

        // WndProc için kanca
        static LRESULT WINAPI GD_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Menü görünürlüðünü deðiþtir
        void GD_ToggleMenu() { m_showMenu = !m_showMenu; }

        // Menü durumunu al
        bool GD_IsMenuVisible() const { return m_showMenu; }

        // Menü render callback ayarla
        void GD_SetRenderCallback(const std::function<void()>& callback) { m_onRenderCallback = callback; }

        // Temizleme
        void GD_Shutdown();
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_MENU_HPP_