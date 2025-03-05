#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <Windows.h>
#include "menu.hpp" 
#include "overlay.hpp"
#include "hotkeys.hpp"
#include "renderer.hpp"
#include "game_interface.hpp"
#include "logger.hpp"

#include "../memory-external/imgui/imgui.h"
#include "../memory-external/imgui/backends/imgui_impl_win32.h"
#include "../memory-external/imgui/backends/imgui_impl_dx10.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool g_isRunning = true;
bool g_menuVisible = true;

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void GameConnectionThread(game::GameInterface* gameInterface) {
    while (g_isRunning) {
        if (!gameInterface->IsConnected()) {
            utils::LogInfo("Attempting to connect to game...");
            if (gameInterface->Initialize("cs2.exe")) {
                utils::LogSuccess("Connected to CS2 successfully");
            }
            else {
                utils::LogError("Failed to connect to CS2");
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else {
            if (!gameInterface->UpdateConnectionStatus()) {
                utils::LogWarning("Game connection lost, will retry");
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_HOTKEY:
        if (wParam == static_cast<int>(core::HotkeyId::MENU_TOGGLE)) {
            g_menuVisible = !g_menuVisible;
            utils::LogInfo(std::string("Menu ") + (g_menuVisible ? "shown" : "hidden"));
            return 0;
        }
        else if (wParam == static_cast<int>(core::HotkeyId::EXIT)) {
            g_isRunning = false;
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        g_isRunning = false;
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main() {
    utils::Logger::GetInstance().InitializeFileOutput("cs2_overlay.log");
    utils::LogInfo("Starting GeezyDigital CS2 Menu");

    game::GameInterface gameInterface;

    // Start game connection thread
    std::thread connectionThread(GameConnectionThread, &gameInterface);

    // Wait for initial connection
    int connectionAttempts = 0;
    while (!gameInterface.IsConnected() && g_isRunning && connectionAttempts < 20) {
        utils::LogInfo("Waiting for game connection... (" + std::to_string(connectionAttempts + 1) + "/20)");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        connectionAttempts++;
    }

    if (!gameInterface.IsConnected()) {
        utils::LogError("Failed to connect to CS2 after multiple attempts, exiting");
        g_isRunning = false;
        connectionThread.join();
        return 1;
    }

    // Find game window
    HWND gameWindow = gameInterface.FindGameWindow();
    if (!gameWindow) {
        utils::LogError("Failed to find CS2 window, exiting");
        g_isRunning = false;
        connectionThread.join();
        return 1;
    }

    // Initialize overlay
    core::Overlay overlay;
    overlay.SetWindowProcedure(CustomWndProc);

    if (!overlay.Initialize(gameWindow)) {
        utils::LogError("Failed to initialize overlay, exiting");
        g_isRunning = false;
        connectionThread.join();
        return 1;
    }

    // Create renderer
    core::Renderer renderer;
    if (!renderer.Initialize(overlay.GetWindowHandle())) {
        utils::LogError("Failed to initialize DirectX renderer, exiting");
        g_isRunning = false;
        connectionThread.join();
        return 1;
    }

    // Initialize UI system
    ui::Menu menu;
    if (!menu.Initialize(overlay.GetWindowHandle(), renderer.GetDevice())) {
        utils::LogError("Failed to initialize ImGui, exiting");
        g_isRunning = false;
        connectionThread.join();
        renderer.Shutdown();
        return 1;
    }

    // Create hotkey manager
    core::HotkeyManager hotkeyManager;

    // Register hotkeys
    hotkeyManager.RegisterHotkey(overlay.GetWindowHandle(), core::HotkeyId::MENU_TOGGLE, 0, 'E');
    hotkeyManager.RegisterHotkey(overlay.GetWindowHandle(), core::HotkeyId::EXIT, 0, VK_END);

    // Set hotkey callbacks
    hotkeyManager.SetHotkeyCallback(core::HotkeyId::MENU_TOGGLE, [&menu]() {
        menu.ToggleVisibility();
        });

    hotkeyManager.SetHotkeyCallback(core::HotkeyId::EXIT, []() {
        g_isRunning = false;
        });

    utils::LogSuccess("Overlay initialized successfully, press E to toggle menu");

    // Main message and render loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT && g_isRunning) {
        // Process Windows messages
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Check if game window is still valid and update overlay
        if (!IsWindow(gameWindow)) {
            gameWindow = gameInterface.FindGameWindow();
            if (!gameWindow) {
                utils::LogWarning("Game window closed or not found");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        // Update overlay position and visibility
        overlay.UpdatePosition(gameWindow);
        overlay.SetVisible(g_menuVisible);

        // Render frame
        menu.NewFrame();
        menu.Render(g_menuVisible, &gameInterface);
        renderer.BeginFrame();
        menu.RenderDrawData();

        if (!renderer.EndFrame()) {
            utils::LogError("Rendering failed, exiting");
            g_isRunning = false;
            break;
        }

        // Prevent CPU usage from being too high
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Cleanup
    utils::LogInfo("Shutting down...");
    g_isRunning = false;

    // Unregister hotkeys
    hotkeyManager.UnregisterAllHotkeys(overlay.GetWindowHandle());
    connectionThread.join();

    // Shutdown systems
    menu.Shutdown();
    renderer.Shutdown();
    overlay.Shutdown();

    utils::LogInfo("Application terminated successfully");

    return 0;
}