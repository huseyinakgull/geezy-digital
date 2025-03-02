#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include "memory/memory.hpp"
#include "memory/handle_hijack.hpp"
#include "config.hpp"

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "               Geezy Digital CS2 ESP              " << std::endl;
    std::cout << "==================================================" << std::endl;

    // Yap�land�rma y�neticisi olu�tur
    geezy_digital::ConfigManager configManager;

    // Offsets'leri kontrol et
    if (!configManager.AreOffsetsLoaded()) {
        std::cout << "[geezy_digital] Hata: Offsetler y�klenemedi. L�tfen offsets.json dosyas�n� kontrol edin." << std::endl;
        std::cout << "��kmak i�in Enter tu�una bas�n..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Process Manager �rne�i olu�tur
    geezy_digital::ProcessManager processManager;

    // CS2'ye Handle Hijacking ile ba�lan (anti-cheat tespitini �nlemek i�in)
    std::cout << "[geezy_digital] CS2'ye ba�lan�l�yor..." << std::endl;
    if (!processManager.GD_AttachToProcessWithHijacking("cs2.exe")) {
        std::cout << "[geezy_digital] Hata: CS2'ye ba�lan�lamad�! Oyunun �al��t���ndan emin olun." << std::endl;
        std::cout << "��kmak i�in Enter tu�una bas�n..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "[geezy_digital] CS2'ye ba�ar�yla ba�land�!" << std::endl;
    std::cout << "[geezy_digital] Process ID: " << processManager.GetProcessId() << std::endl;
    std::cout << "[geezy_digital] Taban Mod�l Adresi: 0x" << std::hex << processManager.GetBaseModule().base << std::dec << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "[geezy_digital] Bellek manip�lasyonu ba�lat�ld�..." << std::endl;

    // Yap�land�rma de�erlerini al
    auto& espConfig = configManager.GetESPConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // Kontrol de�erlerini olu�tur
    bool running = true;

    // Ana d�ng�
    while (running) {
        // Tu� kontrollerini yap
        if (GetAsyncKeyState(keyBindings.toggleEspKey) & 1) { // F1 tu�una bas�ld���nda
            espConfig.enabled = !espConfig.enabled;
            std::cout << "[geezy_digital] ESP: " << (espConfig.enabled ? "A��k" : "Kapal�") << std::endl;
            Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
        }

        if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tu�una bas�ld���nda
            running = false;
            std::cout << "[geezy_digital] Program sonland�r�l�yor..." << std::endl;
        }

        // ESP i�levselli�i burada implemente edilecek
        if (espConfig.enabled) {
            // Bellek manip�lasyonu kodlar� buraya...
        }

        // CPU kullan�m�n� azaltmak i�in k�sa bir bekleme
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Programdan ��kmadan �nce yap�land�rmay� kaydet
    configManager.GD_SaveConfig();

    // Kaynaklar� temizle
    processManager.GD_CloseProcess();

    std::cout << "[geezy_digital] Program sonland�r�ld�." << std::endl;
    return 0;
}