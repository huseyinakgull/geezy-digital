#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <Psapi.h>
#include "../memory-external/memory/memory.hpp"
#include "signature_scanner.hpp"
#include "offset_manager.hpp"
#include "config.hpp"
#include "weapon_skins.hpp"

#pragma comment(lib, "Psapi.lib")

// Game-specific constants
constexpr const char* GAME_PROCESS = "cs2.exe";
constexpr const char* GAME_CLIENT_MODULE = "client.dll";
constexpr const char* OFFSETS_FILE = "offsets.json";
constexpr const char* CONFIG_FILE = "config.json";

// Forward declarations
void MenuSkinManager(Memory::MemoryManager& memManager, Config::ConfigManager& configManager);

void DisplayMenu() {
    std::cout << "\n=== CS2 Bellek Analiz Araci ===\n";
    std::cout << "1. Oyuna baglan\n";
    std::cout << "2. Imza taramasi yap\n";
    std::cout << "3. Offsetleri guncelle\n";
    std::cout << "4. Bellek analizi baslat\n";
    std::cout << "5. Skin yoneticisi\n";
    std::cout << "6. Cikis\n";
    std::cout << "Seciminiz: ";
}

int main() {
    // Initialize components
    Memory::MemoryManager memoryManager(GAME_PROCESS);
    Offsets::OffsetManager offsetManager(OFFSETS_FILE);
    Config::ConfigManager configManager(CONFIG_FILE);

    std::cout << "=== CS2 Bellek Analiz Araci ===\n";
    std::cout << "Bug Bounty programi icin gelistirilmistir\n";
    std::cout << "Etik amaclar icin kullaniniz!\n\n";

    // Check for config
    if (!configManager.LoadConfig()) {
        std::cout << "[BILGI] Konfigurasyon dosyasi olusturuldu." << std::endl;
        configManager.SaveConfig();
    }

    // Attempt to load offsets
    if (!offsetManager.LoadOffsets()) {
        std::cout << "[BILGI] Offsetler yuklenemedi, guncel offsetleri almak icin secim 3'u secin.\n";
    }

    bool running = true;
    bool gameConnected = false;
    uintptr_t clientModuleBase = 0;

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
                }
                else {
                    std::cout << "[HATA] " << GAME_CLIENT_MODULE << " modulu bulunamadi.\n";
                }
            }
            break;

        case 2: // Run signature scan
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }

            {
                // Get module info for scanning
                MODULEINFO moduleInfo = { 0 };
                HMODULE hModule = reinterpret_cast<HMODULE>(clientModuleBase);
                if (GetModuleInformation(memoryManager.GetProcessHandle(), hModule, &moduleInfo, sizeof(moduleInfo))) {
                    // Create signature scanner with memory manager
                    Scanner::SignatureScanner scanner(memoryManager, clientModuleBase, moduleInfo.SizeOfImage);

                    // Run the scan
                    auto results = scanner.ScanForAllSignatures();

                    // Display and update offsets
                    for (const auto& [name, address] : results) {
                        if (address != 0) {
                            offsetManager.SetOffset(name, address - clientModuleBase); // Store as offset from base
                        }
                    }

                    offsetManager.SaveOffsets();
                }
                else {
                    std::cout << "[HATA] Modul bilgisi alinamadi.\n";
                }
            }
            break;

        case 3: // Update offsets
            if (offsetManager.UpdateOffsetsFromRepository("https://github.com/a2x/cs2-dumper")) {
                // Reload the offsets after update
                offsetManager.LoadOffsets();
            }
            break;

        case 4: // Start memory analysis
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }

            if (!offsetManager.IsBuildCurrent(0)) { // 0 is a placeholder, would need actual build checking
                std::cout << "[UYARI] Offsetler guncel olmayabilir. Devam etmek istiyor musunuz? (E/H): ";
                char confirm;
                std::cin >> confirm;
                if (confirm != 'E' && confirm != 'e') {
                    break;
                }
            }

            std::cout << "[BILGI] Bellek analizi baslatiliyor...\n";

            {
                bool analyzing = true;

                // Example: Get EntityList and LocalPlayer offsets
                uintptr_t entityListOffset = offsetManager.GetOffset("dwEntityList");
                uintptr_t localPlayerOffset = offsetManager.GetOffset("dwLocalPlayer");

                if (entityListOffset == 0 || localPlayerOffset == 0) {
                    std::cout << "[HATA] Gerekli offsetler eksik. Lutfen imza taramasi yapin (secim 2).\n";
                    break;
                }

                // Example memory analysis loop
                while (analyzing) {
                    // Calculate absolute addresses
                    uintptr_t entityListAddr = clientModuleBase + entityListOffset;
                    uintptr_t localPlayerAddr = clientModuleBase + localPlayerOffset;

                    // Read local player pointer
                    uintptr_t localPlayer = memoryManager.Read<uintptr_t>(localPlayerAddr);

                    if (localPlayer != 0) {
                        // Read player health as an example
                        int health = memoryManager.Read<int>(localPlayer + offsetManager.GetOffset("m_iHealth"));
                        std::cout << "[BILGI] Oyuncu can: " << health << std::endl;
                    }

                    // Wait before next check
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    // Check for exit condition (could be improved with proper input handling)
                    std::cout << "Analizi durdurmak icin 'q' tusuna basin: ";
                    char input;
                    if (std::cin.peek() == 'q') {
                        analyzing = false;
                    }
                }
            }
            break;

        case 5: // Skin manager
            if (!gameConnected) {
                std::cout << "[HATA] Once oyuna baglanmalisiniz (secim 1).\n";
                break;
            }
            MenuSkinManager(memoryManager, configManager);
            break;

        case 6: // Exit
            running = false;
            break;

        default:
            std::cout << "[HATA] Gecersiz secim.\n";
            break;
        }
    }

    std::cout << "[BILGI] Program sonlandiriliyor...\n";
    return 0;
}

// Skin Manager Menu implementation
void MenuSkinManager(Memory::MemoryManager& memManager, Config::ConfigManager& configManager) {
    // Create weapon skin manager
    geezy_digital::WeaponSkinManager skinManager(memManager, configManager);

    bool running = true;

    while (running) {
        std::cout << "\n=== Skin Yoneticisi ===\n";
        std::cout << "1. Skinleri yukle\n";
        std::cout << "2. Skin ekle/guncelle\n";
        std::cout << "3. Skinleri uygula\n";
        std::cout << "4. Skin yoneticisini " << (skinManager.IsEnabled() ? "devre disi birak" : "etkinlestir") << "\n";
        std::cout << "5. Geri\n";
        std::cout << "Seciminiz: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1: // Load skins
            skinManager.LoadSkins();
            break;

        case 2: { // Add/update skin
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

            skinManager.SetSkin(weaponID, skinID, wear, seed, statTrak);
            skinManager.SaveSkins();
            break;
        }

        case 3: // Apply skins
            skinManager.ApplySkins();
            break;

        case 4: // Toggle
            skinManager.Enable(!skinManager.IsEnabled());
            break;

        case 5: // Back
            running = false;
            break;

        default:
            std::cout << "[HATA] Gecersiz secim.\n";
            break;
        }
    }
}