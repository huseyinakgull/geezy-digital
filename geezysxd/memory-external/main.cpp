#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include "memory/memory.hpp"
#include "memory/handle_hijack.hpp"
#include "config.hpp"
#include "menu.hpp"
#include "features.hpp"

// Türkçe karakter desteði için konsol ayarlarýný yapýlandýr
void SetupConsoleForTurkishCharacters() {
    // UTF-8 desteði için konsol çýktý kodlamasýný ayarla
    _setmode(_fileno(stdout), _O_U8TEXT);

    // Konsol kodlama sayfasýný Türkçe karakterleri destekleyen 65001 (UTF-8) olarak ayarla
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Modern konsol font için
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;  // Font boyutu
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy(cfi.FaceName, L"Consolas");  // Modern konsollar için uygun font
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

    // Baþlýk ayarla
    SetConsoleTitleW(L"Geezy Digital CS2 - Bug Bounty Tool");

    // Buffer boyutunu ayarla
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    csbi.dwSize.X = 120;  // Geniþlik
    csbi.dwSize.Y = 9000; // Yükseklik (geçmiþ log kayýtlarý için)
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), csbi.dwSize);
}

// Örnek kullanýmla birlikte ana iþlevleri içeren geniþletilmiþ program
int main() {
    // Konsol ayarlarýný yapýlandýr
    SetupConsoleForTurkishCharacters();

    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"               Geezy Digital CS2 Tool             " << std::endl;
    std::wcout << L"==================================================" << std::endl;

    // Yapýlandýrma yöneticisi oluþtur
    geezy_digital::ConfigManager configManager;

    // Offset'leri kontrol et
    if (!configManager.AreOffsetsLoaded()) {
        std::wcout << L"[geezy_digital] Hata: Offset'ler yüklenemedi. Lütfen offsets.json dosyasýný kontrol edin." << std::endl;
        std::wcout << L"Çýkmak için Enter tuþuna basýn..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Process Manager örneði oluþtur
    geezy_digital::ProcessManager processManager;

    // CS2'ye Handle Hijacking ile baðlan (anti-cheat tespitini önlemek için)
    std::wcout << L"[geezy_digital] CS2'ye baðlanýlýyor..." << std::endl;
    if (!processManager.GD_AttachToProcessWithHijacking("cs2.exe")) {
        std::wcout << L"[geezy_digital] Hata: CS2'ye baðlanýlamadý! Oyunun çalýþtýðýndan emin olun." << std::endl;
        std::wcout << L"Çýkmak için Enter tuþuna basýn..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::wcout << L"[geezy_digital] CS2'ye baþarýyla baðlandý!" << std::endl;
    std::wcout << L"Process ID: " << processManager.GetProcessId() << std::endl;
    std::wcout << L"Taban Modül Adresi: 0x" << std::hex << processManager.GetBaseModule().base << std::dec << std::endl;

    // Pencere handle'ýný al
    HWND gameWindow = processManager.GetWindowHandle();
    if (!gameWindow) {
        std::wcout << L"[geezy_digital] Uyarý: CS2 pencere handle'ý bulunamadý." << std::endl;
        std::wcout << L"Bazý özellikler düzgün çalýþmayabilir." << std::endl;
    }

    // Yapýlandýrma deðerlerini al
    auto& espConfig = configManager.GetESPConfig();
    auto& fovConfig = configManager.GetFOVConfig();
    auto& skinChangerConfig = configManager.GetSkinChangerConfig();
    auto& glowConfig = configManager.GetGlowConfig();
    auto& radarConfig = configManager.GetRadarConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // Özellik kontrolcüsünü oluþtur
    geezy_digital::FeatureController featureController(processManager, configManager);

    // Özellikleri baþlat
    std::wcout << L"[geezy_digital] Özellikler baþlatýlýyor..." << std::endl;
    if (!featureController.GD_Initialize()) {
        std::wcout << L"[geezy_digital] Uyarý: Bazý özellikler baþlatýlamadý." << std::endl;
    }

    // Menü sistemini oluþtur
    geezy_digital::MenuSystem menuSystem;

    // Menüyü baþlat (gameWindow varsa)
    if (gameWindow) {
        std::wcout << L"[geezy_digital] Menü sistemi baþlatýlýyor..." << std::endl;
        if (!menuSystem.GD_Initialize(gameWindow, &configManager)) {
            std::wcout << L"[geezy_digital] Uyarý: Menü sistemi baþlatýlamadý." << std::endl;
        }

        // ESP render callback'ini ayarla
        menuSystem.GD_SetRenderCallback([&featureController]() {
            featureController.GD_Render();
            });
    }

    // Yapýlandýrma durumlarýný göster
    std::wcout << L"[geezy_digital] ESP Durumu: " << (espConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;
    std::wcout << L"[geezy_digital] FOV Deðiþtirici: " << (fovConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;
    std::wcout << L"[geezy_digital] Skin Deðiþtirici: " << (skinChangerConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;
    std::wcout << L"[geezy_digital] Glow Efekti: " << (glowConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;
    std::wcout << L"[geezy_digital] Radar Hack: " << (radarConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;

    // Kullaným talimatlarý
    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"           Kullanýlabilir Komutlar:               " << std::endl;
    std::wcout << L"  [F1]     - ESP'yi Aç/Kapat                      " << std::endl;
    std::wcout << L"  [INSERT] - Menüyü Göster/Gizle                  " << std::endl;
    std::wcout << L"  [END]    - Programdan Çýk                       " << std::endl;
    std::wcout << L"==================================================" << std::endl;

    // Ana döngü
    bool running = true;

    // Arka plan iþlemcisi thread'i
    std::thread updateThread([&]() {
        while (running) {
            try {
                // Ana özellik güncellemelerini gerçekleþtir
                featureController.GD_Update();

                // Klavye komutlarýný iþle
                if (GetAsyncKeyState(keyBindings.toggleEspKey) & 1) { // F1 tuþuna basýldýðýnda
                    espConfig.enabled = !espConfig.enabled;
                    std::wcout << L"[geezy_digital] ESP: " << (espConfig.enabled ? L"Açýk" : L"Kapalý") << std::endl;
                    Sleep(150); // Tuþ çakýþmasýný önlemek için
                }

                if (GetAsyncKeyState(keyBindings.toggleMenuKey) & 1) { // INSERT tuþuna basýldýðýnda
                    menuSystem.GD_ToggleMenu();
                    std::wcout << L"[geezy_digital] Menü: " << (menuSystem.GD_IsMenuVisible() ? L"Açýk" : L"Kapalý") << std::endl;
                    Sleep(150); // Tuþ çakýþmasýný önlemek için
                }

                if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tuþuna basýldýðýnda
                    running = false;
                    std::wcout << L"[geezy_digital] Program sonlandýrýlýyor..." << std::endl;
                }

                // CPU kullanýmýný azaltmak için kýsa bir bekleme
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            catch (const std::exception& e) {
                std::cerr << "Hata oluþtu: " << e.what() << std::endl;
            }
        }
        });

    // Ana thread message loop'u sürdürür (ImGui için)
    if (gameWindow) {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));

        while (running && msg.message != WM_QUIT) {
            if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            // Programýn çalýþýp çalýþmadýðýný göstermek için "." yazdýr
            static auto lastPrintTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastPrintTime).count() >= 5) {
                std::wcout << L".";
                lastPrintTime = currentTime;
            }

            // CPU kullanýmýný azaltmak için kýsa bir bekleme
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    else {
        // Pencere yoksa sadece bekle
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Güncelleme thread'ini bekle
    if (updateThread.joinable()) {
        updateThread.join();
    }

    // Programdan çýkmadan önce yapýlandýrmayý kaydet
    configManager.GD_SaveConfig();

    // Kaynaklarý temizle
    featureController.GD_Shutdown();
    menuSystem.GD_Shutdown();
    processManager.GD_CloseProcess();

    std::wcout << L"[geezy_digital] Program sonlandýrýldý." << std::endl;
    return 0;
}