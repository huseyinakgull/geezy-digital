#include "renderer.hpp"
#include <iostream>

#pragma comment(lib, "d3d10.lib")

namespace core {

    Renderer::Renderer()
        : m_pSwapChain(nullptr)
        , m_pDevice(nullptr)
        , m_pMainRenderTargetView(nullptr)
    {
    }

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(HWND hWnd) {
        return CreateDeviceD3D(hWnd);
    }

    void Renderer::Shutdown() {
        CleanupDeviceD3D();
    }

    bool Renderer::CreateDeviceD3D(HWND hWnd) {
        // Setup swap chain
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        if (D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice) != S_OK) {
            std::cout << "[ERROR] Failed to create DirectX 10 device and swap chain" << std::endl;
            return false;
        }

        // Create render target
        ID3D10Texture2D* pBackBuffer;
        if (m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)) != S_OK) {
            std::cout << "[ERROR] Failed to get back buffer" << std::endl;
            return false;
        }

        if (m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pMainRenderTargetView) != S_OK) {
            pBackBuffer->Release();
            std::cout << "[ERROR] Failed to create render target view" << std::endl;
            return false;
        }

        pBackBuffer->Release();
        return true;
    }

    void Renderer::CleanupDeviceD3D() {
        if (m_pMainRenderTargetView) { m_pMainRenderTargetView->Release(); m_pMainRenderTargetView = nullptr; }
        if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
        if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }
    }

    void Renderer::BeginFrame() {
        // Set render target
        m_pDevice->OMSetRenderTargets(1, &m_pMainRenderTargetView, NULL);

        // Clear the back buffer to transparent
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_pDevice->ClearRenderTargetView(m_pMainRenderTargetView, clearColor);
    }

    bool Renderer::EndFrame() {
        // Present with vsync
        HRESULT hr = m_pSwapChain->Present(1, 0);

        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                std::cout << "[ERROR] GPU device reset or removed" << std::endl;
                return false;
            }
            else {
                std::cout << "[WARNING] SwapChain Present error: 0x" << std::hex << hr << std::dec << std::endl;
            }
        }

        return true;
    }

} // namespace core