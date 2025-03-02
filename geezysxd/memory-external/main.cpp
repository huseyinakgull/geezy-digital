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

// T�rk�e karakter deste�i i�in konsol ayarlar�n� yap�land�r
void SetupConsoleForTurkishCharacters() {
    // UTF-8 deste�i i�in konsol ��kt� kodlamas�n� ayarla
    _setmode(_fileno(stdout), _O_U8TEXT);

    // Konsol kodlama sayfas�n� T�rk�e karakterleri destekleyen 65001 (UTF-8) olarak ayarla
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Modern konsol font i�in
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;  // Font boyutu
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy(cfi.FaceName, L"Consolas");  // Modern konsollar i�in uygun font
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

    // Ba�l�k ayarla
    SetConsoleTitleW(L"Geezy Digital CS2 - Bug Bounty Tool");

    // Buffer boyutunu ayarla
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    csbi.dwSize.X = 120;  // Geni�lik
    csbi.dwSize.Y = 9000; // Y�kseklik (ge�mi� log kay�tlar� i�in)
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), csbi.dwSize);
}

// �rnek kullan�mla birlikte ana i�levleri i�eren geni�letilmi� program
int main() {
    // Konsol ayarlar�n� yap�land�r
    SetupConsoleForTurkishCharacters();

    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"               Geezy Digital CS2 Tool             " << std::endl;
    std::wcout << L"==================================================" << std::endl;

    // Yap�land�rma y�neticisi olu�tur
    geezy_digital::ConfigManager configManager;

    // Offset'leri kontrol et
    if (!configManager.AreOffsetsLoaded()) {
        std::wcout << L"[geezy_digital] Hata: Offset'ler y�klenemedi. L�tfen offsets.json dosyas�n� kontrol edin." << std::endl;
        std::wcout << L"��kmak i�in Enter tu�una bas�n..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Process Manager �rne�i olu�tur
    geezy_digital::ProcessManager processManager;

    // CS2'ye Handle Hijacking ile ba�lan (anti-cheat tespitini �nlemek i�in)
    std::wcout << L"[geezy_digital] CS2'ye ba�lan�l�yor..." << std::endl;
    if (!processManager.GD_AttachToProcessWithHijacking("cs2.exe")) {
        std::wcout << L"[geezy_digital] Hata: CS2'ye ba�lan�lamad�! Oyunun �al��t���ndan emin olun." << std::endl;
        std::wcout << L"��kmak i�in Enter tu�una bas�n..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::wcout << L"[geezy_digital] CS2'ye ba�ar�yla ba�land�!" << std::endl;
    std::wcout << L"Process ID: " << processManager.GetProcessId() << std::endl;
    std::wcout << L"Taban Mod�l Adresi: 0x" << std::hex << processManager.GetBaseModule().base << std::dec << std::endl;

    // Pencere handle'�n� al
    HWND gameWindow = processManager.GetWindowHandle();
    if (!gameWindow) {
        std::wcout << L"[geezy_digital] Uyar�: CS2 pencere handle'� bulunamad�." << std::endl;
        std::wcout << L"Baz� �zellikler d�zg�n �al��mayabilir." << std::endl;
    }

    // Yap�land�rma de�erlerini al
    auto& espConfig = configManager.GetESPConfig();
    auto& fovConfig = configManager.GetFOVConfig();
    auto& skinChangerConfig = configManager.GetSkinChangerConfig();
    auto& glowConfig = configManager.GetGlowConfig();
    auto& radarConfig = configManager.GetRadarConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // �zellik kontrolc�s�n� olu�tur
    geezy_digital::FeatureController featureController(processManager, configManager);

    // �zellikleri ba�lat
    std::wcout << L"[geezy_digital] �zellikler ba�lat�l�yor..." << std::endl;
    if (!featureController.GD_Initialize()) {
        std::wcout << L"[geezy_digital] Uyar�: Baz� �zellikler ba�lat�lamad�." << std::endl;
    }

    // Men� sistemini olu�tur
    geezy_digital::MenuSystem menuSystem;

    // Men�y� ba�lat (gameWindow varsa)
    if (gameWindow) {
        std::wcout << L"[geezy_digital] Men� sistemi ba�lat�l�yor..." << std::endl;
        if (!menuSystem.GD_Initialize(gameWindow, &configManager)) {
            std::wcout << L"[geezy_digital] Uyar�: Men� sistemi ba�lat�lamad�." << std::endl;
        }

        // ESP render callback'ini ayarla
        menuSystem.GD_SetRenderCallback([&featureController]() {
            featureController.GD_Render();
            });
    }

    // Yap�land�rma durumlar�n� g�ster
    std::wcout << L"[geezy_digital] ESP Durumu: " << (espConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;
    std::wcout << L"[geezy_digital] FOV De�i�tirici: " << (fovConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;
    std::wcout << L"[geezy_digital] Skin De�i�tirici: " << (skinChangerConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;
    std::wcout << L"[geezy_digital] Glow Efekti: " << (glowConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;
    std::wcout << L"[geezy_digital] Radar Hack: " << (radarConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;

    // Kullan�m talimatlar�
    std::wcout << L"==================================================" << std::endl;
    std::wcout << L"           Kullan�labilir Komutlar:               " << std::endl;
    std::wcout << L"  [F1]     - ESP'yi A�/Kapat                      " << std::endl;
    std::wcout << L"  [INSERT] - Men�y� G�ster/Gizle                  " << std::endl;
    std::wcout << L"  [END]    - Programdan ��k                       " << std::endl;
    std::wcout << L"==================================================" << std::endl;

    // Ana d�ng�
    bool running = true;

    // Arka plan i�lemcisi thread'i
    std::thread updateThread([&]() {
        while (running) {
            try {
                // Ana �zellik g�ncellemelerini ger�ekle�tir
                featureController.GD_Update();

                // Klavye komutlar�n� i�le
                if (GetAsyncKeyState(keyBindings.toggleEspKey) & 1) { // F1 tu�una bas�ld���nda
                    espConfig.enabled = !espConfig.enabled;
                    std::wcout << L"[geezy_digital] ESP: " << (espConfig.enabled ? L"A��k" : L"Kapal�") << std::endl;
                    Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
                }

                if (GetAsyncKeyState(keyBindings.toggleMenuKey) & 1) { // INSERT tu�una bas�ld���nda
                    menuSystem.GD_ToggleMenu();
                    std::wcout << L"[geezy_digital] Men�: " << (menuSystem.GD_IsMenuVisible() ? L"A��k" : L"Kapal�") << std::endl;
                    Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
                }

                if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tu�una bas�ld���nda
                    running = false;
                    std::wcout << L"[geezy_digital] Program sonland�r�l�yor..." << std::endl;
                }

                // CPU kullan�m�n� azaltmak i�in k�sa bir bekleme
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            catch (const std::exception& e) {
                std::cerr << "Hata olu�tu: " << e.what() << std::endl;
            }
        }
        });

    // Ana thread message loop'u s�rd�r�r (ImGui i�in)
    if (gameWindow) {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));

        while (running && msg.message != WM_QUIT) {
            if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            // Program�n �al���p �al��mad���n� g�stermek i�in "." yazd�r
            static auto lastPrintTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastPrintTime).count() >= 5) {
                std::wcout << L".";
                lastPrintTime = currentTime;
            }

            // CPU kullan�m�n� azaltmak i�in k�sa bir bekleme
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    else {
        // Pencere yoksa sadece bekle
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // G�ncelleme thread'ini bekle
    if (updateThread.joinable()) {
        updateThread.join();
    }

    // Programdan ��kmadan �nce yap�land�rmay� kaydet
    configManager.GD_SaveConfig();

    // Kaynaklar� temizle
    featureController.GD_Shutdown();
    menuSystem.GD_Shutdown();
    processManager.GD_CloseProcess();

    std::wcout << L"[geezy_digital] Program sonland�r�ld�." << std::endl;
    return 0;
}