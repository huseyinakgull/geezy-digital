#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <tchar.h>
#include <Psapi.h>
#include <dwmapi.h>
#include "memory/memory.hpp"
#include "memory/handle_hijack.hpp"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx10.h"

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dwmapi.lib")

// Global variables
constexpr const char* GAME_PROCESS = "cs2.exe";
constexpr const char* GAME_WINDOW_CLASS_NAMES[] = { "CS2" };
constexpr const char* GAME_CLIENT_MODULE = "client.dll";

// Hotkey IDs
#define HOTKEY_MENU_TOGGLE 1
#define HOTKEY_EXIT       2

Memory::MemoryManager* g_memoryManager = nullptr;
bool g_gameConnected = false;
uintptr_t g_clientModuleBase = 0;
bool g_menuVisible = true;   // Menu visibility flag
bool g_isRunning = true;     // Application running flag
HWND g_gameWindow = NULL;    // Global handle to game window

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_HOTKEY:
        // Process global hotkeys
        if (wParam == HOTKEY_MENU_TOGGLE) {
            g_menuVisible = !g_menuVisible;
            std::cout << "[INFO] Menu " << (g_menuVisible ? "shown" : "hidden") << std::endl;
            return 0;
        }
        else if (wParam == HOTKEY_EXIT) {
            g_isRunning = false;
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_DESTROY:
        UnregisterHotKey(hWnd, HOTKEY_MENU_TOGGLE);
        UnregisterHotKey(hWnd, HOTKEY_EXIT);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Register global hotkeys
bool RegisterGlobalHotkeys(HWND hWnd) {
    bool success = true;

    // Register INSERT key for menu toggle (works globally)
    if (!RegisterHotKey(hWnd, HOTKEY_MENU_TOGGLE, 0, VK_INSERT)) {
        std::cout << "[ERROR] Failed to register INSERT hotkey. Error: " << GetLastError() << std::endl;
        success = false;
    }

    // Register END key for application exit (works globally) 
    if (!RegisterHotKey(hWnd, HOTKEY_EXIT, 0, VK_END)) {
        std::cout << "[ERROR] Failed to register END hotkey. Error: " << GetLastError() << std::endl;
        success = false;
    }

    return success;
}

// Find the game window
HWND FindGameWindow() {
    HWND hwnd = NULL;

    // Try known class names first
    for (const auto& className : GAME_WINDOW_CLASS_NAMES) {
        hwnd = FindWindowA(className, NULL);
        if (hwnd) {
            std::cout << "[SUCCESS] CS2 window found with class name '" << className << "': " << hwnd << std::endl;
            return hwnd;
        }
    }

    // Find windows associated with cs2.exe process
    DWORD processId = 0;
    if (g_memoryManager && g_memoryManager->GetProcessHandle()) {
        processId = GetProcessId(g_memoryManager->GetProcessHandle());
    }

    if (processId == 0) {
        std::cout << "[ERROR] Process ID is zero, cannot use EnumWindows." << std::endl;
        return NULL;
    }

    struct FindWindowData {
        DWORD processId;
        HWND result;
    };

    FindWindowData data = { processId, NULL };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        FindWindowData* data = reinterpret_cast<FindWindowData*>(lParam);
        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId == data->processId) {
            char className[256] = { 0 };
            GetClassNameA(hwnd, className, sizeof(className));

            RECT rect;
            GetWindowRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Only accept windows larger than a minimum size
            if (width > 200 && height > 200) {
                std::cout << "[FOUND] CS2 window (by Process ID): " << hwnd
                    << " Class: " << className
                    << " Size: " << width << "x" << height << std::endl;
                data->result = hwnd;
                return FALSE; // Stop enumeration
            }
        }
        return TRUE; // Continue enumeration
        }, reinterpret_cast<LPARAM>(&data));

    if (data.result) {
        return data.result;
    }

    std::cout << "[ERROR] CS2 window not found. Make sure the game is running." << std::endl;
    return NULL;
}

// Initialize DirectX 10
bool CreateDeviceD3D(HWND hWnd, IDXGISwapChain** ppSwapChain, ID3D10Device** ppDevice, ID3D10RenderTargetView** ppMainRenderTargetView) {
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
    if (D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd, ppSwapChain, ppDevice) != S_OK)
        return false;

    // Create render target
    ID3D10Texture2D* pBackBuffer;
    (*ppSwapChain)->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer == NULL)
        return false;

    (*ppDevice)->CreateRenderTargetView(pBackBuffer, NULL, ppMainRenderTargetView);
    pBackBuffer->Release();

    return true;
}

void CleanupDeviceD3D(IDXGISwapChain* pSwapChain, ID3D10Device* pDevice, ID3D10RenderTargetView* pMainRenderTargetView) {
    if (pMainRenderTargetView) { pMainRenderTargetView->Release(); pMainRenderTargetView = NULL; }
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = NULL; }
    if (pDevice) { pDevice->Release(); pDevice = NULL; }
}

// Create overlay window that will be drawn on top of game
HWND CreateOverlayWindow(HWND gameWindow) {
    // Create window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("GeezyDigital Overlay"), NULL };
    RegisterClassEx(&wc);

    RECT gameRect;
    HWND hwnd = NULL;

    if (gameWindow && GetWindowRect(gameWindow, &gameRect)) {
        // Use game window size and position
        hwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
            wc.lpszClassName,
            _T("GeezyDigital CS2 Menu"),
            WS_POPUP,
            gameRect.left, gameRect.top,
            gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
            NULL, // No parent window to enable global hotkeys 
            NULL, NULL, NULL
        );
    }
    else {
        // Default size and position if game window not found
        hwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
            wc.lpszClassName,
            _T("GeezyDigital CS2 Menu"),
            WS_POPUP,
            100, 100, 800, 600,
            NULL, NULL, NULL, NULL
        );
    }

    if (!hwnd) {
        DWORD error = GetLastError();
        std::cout << "[ERROR] Failed to create overlay window. Error code: " << error << std::endl;
        return NULL;
    }

    // Make black background transparent, alpha for menu elements remains
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Position overlay on top of game window
    SetWindowPos(hwnd, HWND_TOPMOST,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
        SWP_SHOWWINDOW);

    // Register global hotkeys
    RegisterGlobalHotkeys(hwnd);

    // Optional: disable DWM blur effect
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = FALSE;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    return hwnd;
}

// Keep overlay aligned with game window
void UpdateOverlayPosition(HWND overlay, HWND gameWindow) {
    if (!IsWindow(gameWindow) || !IsWindow(overlay))
        return;

    RECT gameRect;
    if (!GetWindowRect(gameWindow, &gameRect)) {
        return;
    }

    // Match overlay size and position to game window
    SetWindowPos(
        overlay, HWND_TOPMOST,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
        SWP_NOACTIVATE
    );

    // Toggle transparency based on menu visibility
    LONG ex_style = GetWindowLong(overlay, GWL_EXSTYLE);
    if (g_menuVisible) {
        // When menu is visible, allow mouse input
        ex_style &= ~WS_EX_TRANSPARENT;
    }
    else {
        // When menu is hidden, make window click-through
        ex_style |= WS_EX_TRANSPARENT;
    }
    SetWindowLong(overlay, GWL_EXSTYLE, ex_style);
}

// Game memory management thread
void GameMemoryThread() {
    while (g_isRunning) {
        if (!g_gameConnected) {
            if (g_memoryManager->AttachToProcess()) {
                g_clientModuleBase = g_memoryManager->GetModuleBaseAddress(GAME_CLIENT_MODULE);
                if (g_clientModuleBase != 0) {
                    std::cout << "[SUCCESS] " << GAME_CLIENT_MODULE << " module found: 0x"
                        << std::hex << g_clientModuleBase << std::dec << std::endl;
                    g_gameConnected = true;
                }
                else {
                    std::cout << "[ERROR] " << GAME_CLIENT_MODULE << " module not found.\n";
                }
            }
            else {
                std::cout << "[INFO] Game not found, retrying...\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else {
            // Check if process is still running
            DWORD exitCode = 0;
            if (!GetExitCodeProcess(g_memoryManager->GetProcessHandle(), &exitCode) || exitCode != STILL_ACTIVE) {
                std::cout << "[INFO] Game closed, attempting to reconnect...\n";
                g_gameConnected = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Simple menu interface
void RenderSimpleMenu() {
    // Create ImGui main window
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Create a small window in the top-left corner
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.85f); // Semi-transparent background

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize;

    // Create window with frame and title bar
    if (!ImGui::Begin("GeezyDigital CS2 Menu", &g_menuVisible, window_flags)) {
        ImGui::End();
        return;
    }

    // Title
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "GeezyDigital CS2 Menu");
    ImGui::Separator();

    // Game connection status
    ImGui::TextColored(g_gameConnected ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "Game Status: %s", g_gameConnected ? "Connected" : "Not Connected");

    if (g_gameConnected) {
        ImGui::Text("Process ID: %u", GetProcessId(g_memoryManager->GetProcessHandle()));
        ImGui::Text("Client Module Base: 0x%llX", g_clientModuleBase);
    }

    ImGui::Separator();

    // Buttons and controls
    if (ImGui::Button("Reconnect", ImVec2(135, 0))) {
        g_gameConnected = false; // Trigger reconnection
    }

    ImGui::SameLine();
    if (ImGui::Button("Exit", ImVec2(135, 0))) {
        g_isRunning = false;
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

// Main function
int main() {
    // Console setup
    SetConsoleTitle("GeezyDigital CS2 Menu");
    std::cout << "Starting GeezyDigital CS2 Menu..." << std::endl;

    // Create memory manager
    g_memoryManager = new Memory::MemoryManager(GAME_PROCESS);

    // Start game memory thread
    std::thread memoryThread(GameMemoryThread);

    // First try to connect to game memory
    int memoryAttempts = 0;
    while (!g_gameConnected && g_isRunning && memoryAttempts < 20) {
        std::cout << "Waiting for CS2 process... Attempt " << memoryAttempts + 1 << "/20" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        memoryAttempts++;
    }

    if (!g_isRunning || !g_gameConnected) {
        std::cout << "[ERROR] Failed to connect to CS2 process." << std::endl;
        if (memoryThread.joinable()) {
            memoryThread.join();
        }
        return 1;
    }

    // Find game window after process is found
    g_gameWindow = NULL;
    int windowAttempts = 0;

    while (!g_gameWindow && g_isRunning && windowAttempts < 20) {
        g_gameWindow = FindGameWindow();
        if (!g_gameWindow) {
            std::cout << "Waiting for CS2 window... Attempt " << windowAttempts + 1 << "/20" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            windowAttempts++;
        }
    }

    if (!g_gameWindow) {
        std::cout << "[ERROR] CS2 window not found. Please make sure the game is running in Fullscreen Windowed mode." << std::endl;
        if (memoryThread.joinable()) {
            memoryThread.join();
        }
        return 1;
    }

    std::cout << "[INFO] CS2 window found. Creating overlay..." << std::endl;

    // Create overlay window
    HWND overlayWindow = CreateOverlayWindow(g_gameWindow);
    if (!overlayWindow) {
        std::cout << "[ERROR] Failed to create overlay window." << std::endl;
        if (memoryThread.joinable()) {
            memoryThread.join();
        }
        return 1;
    }

    // Initialize Direct3D
    IDXGISwapChain* swapChain = nullptr;
    ID3D10Device* device = nullptr;
    ID3D10RenderTargetView* mainRenderTargetView = nullptr;
    if (!CreateDeviceD3D(overlayWindow, &swapChain, &device, &mainRenderTargetView)) {
        std::cout << "[ERROR] Failed to initialize DirectX 10." << std::endl;
        CleanupDeviceD3D(swapChain, device, mainRenderTargetView);
        DestroyWindow(overlayWindow);
        UnregisterClass(_T("GeezyDigital Overlay"), GetModuleHandle(NULL));
        if (memoryThread.joinable()) {
            memoryThread.join();
        }
        return 1;
    }

    // Show window
    ShowWindow(overlayWindow, SW_SHOWDEFAULT);
    UpdateWindow(overlayWindow);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Customize style
    ImGui::GetStyle().WindowRounding = 5.0f;
    ImGui::GetStyle().FrameRounding = 3.0f;
    ImGui::GetStyle().GrabRounding = 3.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(overlayWindow);
    ImGui_ImplDX10_Init(device);

    std::cout << "[INFO] System initialized. Use INSERT key to toggle menu (works while CS2 is focused)" << std::endl;

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT && g_isRunning) {
        // Poll and handle messages
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Check if game window is still valid
        if (!IsWindow(g_gameWindow)) {
            g_gameWindow = FindGameWindow();
            if (!g_gameWindow) {
                std::cout << "[ERROR] Game window closed. Exiting program." << std::endl;
                g_isRunning = false;
                break;
            }
        }

        // Update overlay position
        UpdateOverlayPosition(overlayWindow, g_gameWindow);

        // Make sure overlay remains click-through but receives input when menu is visible
        // This is a critical step - we update this every frame to override any focus changes
        if (g_menuVisible) {
            // For safety, we'll also ensure window is topmost
            SetWindowPos(overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        // Start the ImGui frame
        ImGui_ImplDX10_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Menu rendering
        if (g_menuVisible) {
            RenderSimpleMenu();
        }

        // Rendering
        ImGui::Render();
        device->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        ImVec4 clearColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Fully transparent background
        device->ClearRenderTargetView(mainRenderTargetView, (float*)&clearColor);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = swapChain->Present(1, 0); // Present with vsync
        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                std::cout << "[ERROR] GPU device reset or removed. Exiting program." << std::endl;
                g_isRunning = false;
                break;
            }
            else {
                std::cout << "[WARNING] SwapChain Present error: 0x" << std::hex << hr << std::dec << std::endl;
            }
        }
    }

    // Cleanup
    std::cout << "[INFO] Shutting down..." << std::endl;
    g_isRunning = false;

    if (memoryThread.joinable()) {
        memoryThread.join();
    }

    // Unregister hotkeys before destroying window
    UnregisterHotKey(overlayWindow, HOTKEY_MENU_TOGGLE);
    UnregisterHotKey(overlayWindow, HOTKEY_EXIT);

    ImGui_ImplDX10_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D(swapChain, device, mainRenderTargetView);
    DestroyWindow(overlayWindow);
    UnregisterClass(_T("GeezyDigital Overlay"), GetModuleHandle(NULL));

    if (g_memoryManager) {
        delete g_memoryManager;
        g_memoryManager = nullptr;
    }

    return 0;
}