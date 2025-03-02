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

// D3D11 ba�lant�lar� i�in gerekli tan�mlar
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

        // Orijinal WndProc ve kancal� fonksiyonlar
        WNDPROC m_originalWndProc = nullptr;
        D3D11PresentHook m_originalPresent = nullptr;
        D3D11ResizeBuffersHook m_originalResizeBuffers = nullptr;

        // Men� durumu
        bool m_showMenu = false;
        bool m_initialized = false;
        std::string m_menuTitle = "Geezy Digital CS2 Tool";

        // Konfig�rasyon y�neticisi
        ConfigManager* m_configManager = nullptr;

        // Men� sekmeleri i�in tan�mlar
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

        // Tema ayarlar�
        bool m_darkTheme = true;
        ImVec4 m_accentColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

        // Callback fonksiyonlar�
        std::function<void()> m_onRenderCallback = nullptr;

        // Silah ve kaplama verileri
        struct WeaponInfo {
            int id;
            std::string name;
            std::vector<std::pair<int, std::string>> skins;
        };

        std::vector<WeaponInfo> m_weaponData;

        // Desteklenen CS2 b��ak modelleri
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

        // Renk kartelas�
        const std::vector<ImVec4> m_colorPalette = {
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // K�rm�z�
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Ye�il
            ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Mavi
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Sar�
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),  // Magenta
            ImVec4(0.0f, 1.0f, 1.0f, 1.0f),  // Cyan
            ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Turuncu
            ImVec4(0.5f, 0.0f, 1.0f, 1.0f),  // Mor
            ImVec4(0.0f, 0.5f, 0.0f, 1.0f),  // Koyu ye�il
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f)   // Gri
        };

        // Silah verileri y�kle
        void GD_LoadWeaponData();

        // ImGui stili ayarla
        void GD_SetupStyle();

        // Men� i�eri�ini olu�tur
        void GD_RenderMainMenu();
        void GD_RenderESPTab();
        void GD_RenderGlowTab();
        void GD_RenderRadarTab();
        void GD_RenderFOVTab();
        void GD_RenderSkinsTab();
        void GD_RenderMiscTab();
        void GD_RenderConfigTab();
        void GD_RenderAboutTab();

        // Helper fonksiyonlar�
        void GD_ColorEdit4(const char* label, GD_Color& color);
        ImVec4 GD_ColorToImVec4(const GD_Color& color);
        GD_Color GD_ImVec4ToColor(const ImVec4& color);

    public:
        MenuSystem() = default;
        ~MenuSystem();

        // Men� sistemini ba�lat
        bool GD_Initialize(HWND hWnd, ConfigManager* configManager);

        // ImGui'yi haz�rla
        bool GD_InitImGui();

        // DirectX hook'lar� kur
        bool GD_HookDirectX();

        // D3D11 i�in kancalar
        static HRESULT __stdcall GD_HookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT __stdcall GD_HookResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

        // WndProc i�in kanca
        static LRESULT WINAPI GD_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Men� g�r�n�rl���n� de�i�tir
        void GD_ToggleMenu() { m_showMenu = !m_showMenu; }

        // Men� durumunu al
        bool GD_IsMenuVisible() const { return m_showMenu; }

        // Men� render callback ayarla
        void GD_SetRenderCallback(const std::function<void()>& callback) { m_onRenderCallback = callback; }

        // Temizleme
        void GD_Shutdown();
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_MENU_HPP_