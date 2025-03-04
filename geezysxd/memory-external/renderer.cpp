#include "renderer.hpp"
#include <iostream>
#include "logger.hpp"

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
        // Get window dimensions
        RECT rect;
        GetClientRect(hWnd, &rect);
        UINT width = rect.right - rect.left;
        UINT height = rect.bottom - rect.top;

        utils::LogInfo("Renderer initializing with resolution: " + std::to_string(width) + "x" + std::to_string(height));

        // Setup swap chain
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
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
        // Uncomment for debugging
        // createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;

        HRESULT hr = D3D10CreateDeviceAndSwapChain(
            NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
            D3D10_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice);

        if (FAILED(hr)) {
            utils::LogError("Failed to create DirectX 10 device and swap chain. HRESULT: " + std::to_string(hr));

            // Try with WARP driver as fallback
            utils::LogWarning("Attempting to create device with WARP driver...");
            hr = D3D10CreateDeviceAndSwapChain(
                NULL, D3D10_DRIVER_TYPE_WARP, NULL, createDeviceFlags,
                D3D10_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice);

            if (FAILED(hr)) {
                utils::LogError("Failed to create WARP device. HRESULT: " + std::to_string(hr));
                return false;
            }
        }

        // Create render target
        ID3D10Texture2D* pBackBuffer;
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (FAILED(hr)) {
            utils::LogError("Failed to get back buffer. HRESULT: " + std::to_string(hr));
            return false;
        }

        hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pMainRenderTargetView);
        pBackBuffer->Release();

        if (FAILED(hr)) {
            utils::LogError("Failed to create render target view. HRESULT: " + std::to_string(hr));
            return false;
        }

        // Set up the viewport
        D3D10_VIEWPORT vp;
        vp.Width = width;
        vp.Height = height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        m_pDevice->RSSetViewports(1, &vp);

        utils::LogSuccess("DirectX 10 renderer initialized successfully");
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
        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_pDevice->ClearRenderTargetView(m_pMainRenderTargetView, clearColor);
    }

    bool Renderer::EndFrame() {
        // Present with vsync
        HRESULT hr = m_pSwapChain->Present(1, 0);

        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                utils::LogError("GPU device reset or removed. HRESULT: " + std::to_string(hr));
                return false;
            }
            else {
                utils::LogWarning("SwapChain Present error: 0x" + std::to_string(hr));
            }
        }

        return true;
    }

} // namespace core