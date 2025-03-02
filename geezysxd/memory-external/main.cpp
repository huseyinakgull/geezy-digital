#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <Windows.h>
#include "memory/memory.hpp"
#include "config.hpp"
#include "weapon_skins.hpp"

#pragma comment(lib, "Psapi.lib")

constexpr const char* GAME_PROCESS = "cs2.exe";
constexpr const char* GAME_CLIENT_MODULE = "client.dll";
constexpr const char* CONFIG_FILE = "config.json";
bool g_isAutoUpdateEnabled = false;
bool g_isSkinChangerEnabled = false;
bool g_isRunning = true;


void DisplayMenu() {
    std::cout << "\n=== CS2 Skin Changer ===\n";
    std::cout << "1. Oyuna baglan\n";
    std::cout << "2. Skinleri yukle\n";
    std::cout << "3. Skin ekle/guncelle\n";
    std::cout << "4. Skinleri uygula\n";
    std::cout << "5. Otomatik guncelleyi " << (g_isAutoUpdateEnabled ? "kapat" : "ac") << "\n";
    std::cout << "6. Skin Changer'i " << (g_isSkinChangerEnabled ? "devre disi birak" : "etkinlestir") << "\n";
    std::cout << "7. Cikis\n";
    std::cout << "Seciminiz: ";
}

void AutoUpdateThread(geezy_digital::WeaponSkinManager& skinManager, int updateInterval) {
    while (g_isRunning) {
        if (g_isAutoUpdateEnabled && g_isSkinChangerEnabled) {
            skinManager.ApplySkins();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(updateInterval));
    }
}

int main() {
    // Initialize components
    Memory::MemoryManager memoryManager(GAME_PROCESS);
    Config::ConfigManager configManager(CONFIG_FILE);

    // Check for config
    if (!configManager.LoadConfig()) {
        configManager.SaveConfig();
    }

    bool running = true;
    bool gameConnected = false;
    uintptr_t clientModuleBase = 0;

    // Skin manager (will be created after connecting)
    std::unique_ptr<geezy_digital::WeaponSkinManager> skinManager;

    g_isAutoUpdateEnabled = configManager.GetBool("SkinChanger.AutoUpdate", false);
    g_isSkinChangerEnabled = configManager.GetBool("SkinChanger.Enabled", false);
    int updateInterval = configManager.GetInt("SkinChanger.UpdateInterval", 1000);

    // Auto update thread
    std::thread autoUpdateThread;

    while (running) {
        DisplayMenu();

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1: // Connect to game
            if (memoryManager.AttachToProcess()) {
                clientModuleBase = memoryManager.GetModuleBaseAddress(GAME_CLIENT_MODULE);
                if (clientModuleBase != 0) {
                    std::cout << "[BASARI] " << GAME_CLIENT_MODULE << " modulu bulundu: 0x"
                        << std::hex << clientModuleBase << std::dec << std::endl;
                    gameConnected = true;

                    // Create skin manager after connecting
                    skinManager = std::make_unique<geezy_digital::WeaponSkinManager>(memoryManager, configManager);

                    // Start auto update thread
                    if (!autoUpdateThread.joinable()) {
                        autoUpdateThread = std::thread(AutoUpdateThread, std::ref(*skinManager), updateInterval);
                    }

                    // Apply settings
                    skinManager->Enable(g_isSkinChangerEnabled);
                    if (g_isAutoUpdateEnabled) {
                        skinManager->StartAutoUpdate();
                    }
                }
                else {
                    std::cout << "[HATA] " << GAME_CLIENT_MODULE << " modulu bulunamadi.\n";
                }
            }
            break;

        case 2: // Load skins
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }
            skinManager->LoadSkins();
            break;

        case 3: { // Add/update skin
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }

            int weaponID, skinID, seed, statTrak;
            float wear;

            std::cout << "Weapon ID: ";
            std::cin >> weaponID;

            std::cout << "Skin ID: ";
            std::cin >> skinID;

            std::cout << "Wear (0.0-1.0): ";
            std::cin >> wear;

            std::cout << "Seed: ";
            std::cin >> seed;

            std::cout << "StatTrak (-1 for disabled): ";
            std::cin >> statTrak;

            skinManager->SetSkin(weaponID, skinID, wear, seed, statTrak);
            skinManager->SaveSkins();
            break;
        }

        case 4: // Apply skins
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }
            skinManager->ApplySkins();
            break;

        case 5: // Toggle auto update
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }
            g_isAutoUpdateEnabled = !g_isAutoUpdateEnabled;
            if (g_isAutoUpdateEnabled) {
                skinManager->StartAutoUpdate();
            }
            else {
                skinManager->StopAutoUpdate();
            }
            break;

        case 6: // Toggle skin changer
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }
            g_isSkinChangerEnabled = !g_isSkinChangerEnabled;
            skinManager->Enable(g_isSkinChangerEnabled);
            break;

        case 7: // Exit
            running = false;
            g_isRunning = false;
            break;

        default:
            std::cout << "[HATA] Gecersiz secim.\n";
            break;
        }
    }

    // Wait for auto update thread to finish
    if (autoUpdateThread.joinable()) {
        autoUpdateThread.join();
    }

    std::cout << "[BILGI] Program sonlandiriliyor...\n";
    return 0;
}