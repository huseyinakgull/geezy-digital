#pragma once

#include <Windows.h>
#include <d3d10_1.h>
#include <d3d10.h>

namespace core {

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        // Initialize DirectX 10 rendering
        bool Initialize(HWND hWnd);

        // Shutdown and cleanup DirectX resources
        void Shutdown();

        // Begin frame rendering
        void BeginFrame();

        // End frame rendering and present to screen
        bool EndFrame();

        // Get DirectX device
        ID3D10Device* GetDevice() const { return m_pDevice; }

        // Get Render Target View
        ID3D10RenderTargetView* GetRenderTargetView() const { return m_pMainRenderTargetView; }

    private:
        IDXGISwapChain* m_pSwapChain;
        ID3D10Device* m_pDevice;
        ID3D10RenderTargetView* m_pMainRenderTargetView;

        // Create Direct3D device and swap chain
        bool CreateDeviceD3D(HWND hWnd);

        // Cleanup Direct3D resources
        void CleanupDeviceD3D();
    };

} // namespace core