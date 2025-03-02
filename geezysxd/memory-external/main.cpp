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

    // Yapýlandýrma yöneticisi oluþtur
    geezy_digital::ConfigManager configManager;

    // Offsets'leri kontrol et
    if (!configManager.AreOffsetsLoaded()) {
        std::cout << "[geezy_digital] Hata: Offsetler yüklenemedi. Lütfen offsets.json dosyasýný kontrol edin." << std::endl;
        std::cout << "Çýkmak için Enter tuþuna basýn..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Process Manager örneði oluþtur
    geezy_digital::ProcessManager processManager;

    // CS2'ye Handle Hijacking ile baðlan (anti-cheat tespitini önlemek için)
    std::cout << "[geezy_digital] CS2'ye baðlanýlýyor..." << std::endl;
    if (!processManager.GD_AttachToProcessWithHijacking("cs2.exe")) {
        std::cout << "[geezy_digital] Hata: CS2'ye baðlanýlamadý! Oyunun çalýþtýðýndan emin olun." << std::endl;
        std::cout << "Çýkmak için Enter tuþuna basýn..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "[geezy_digital] CS2'ye baþarýyla baðlandý!" << std::endl;
    std::cout << "[geezy_digital] Process ID: " << processManager.GetProcessId() << std::endl;
    std::cout << "[geezy_digital] Taban Modül Adresi: 0x" << std::hex << processManager.GetBaseModule().base << std::dec << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "[geezy_digital] Bellek manipülasyonu baþlatýldý..." << std::endl;

    // Yapýlandýrma deðerlerini al
    auto& espConfig = configManager.GetESPConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // Kontrol deðerlerini oluþtur
    bool running = true;

    // Ana döngü
    while (running) {
        // Tuþ kontrollerini yap
        if (GetAsyncKeyState(keyBindings.toggleEspKey) & 1) { // F1 tuþuna basýldýðýnda
            espConfig.enabled = !espConfig.enabled;
            std::cout << "[geezy_digital] ESP: " << (espConfig.enabled ? "Açýk" : "Kapalý") << std::endl;
            Sleep(150); // Tuþ çakýþmasýný önlemek için
        }

        if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tuþuna basýldýðýnda
            running = false;
            std::cout << "[geezy_digital] Program sonlandýrýlýyor..." << std::endl;
        }

        // ESP iþlevselliði burada implemente edilecek
        if (espConfig.enabled) {
            // Bellek manipülasyonu kodlarý buraya...
        }

        // CPU kullanýmýný azaltmak için kýsa bir bekleme
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Programdan çýkmadan önce yapýlandýrmayý kaydet
    configManager.GD_SaveConfig();

    // Kaynaklarý temizle
    processManager.GD_CloseProcess();

    std::cout << "[geezy_digital] Program sonlandýrýldý." << std::endl;
    return 0;
}