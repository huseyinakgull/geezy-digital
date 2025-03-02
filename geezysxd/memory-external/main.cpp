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
    std::cout << "Process ID: " << processManager.GetProcessId() << std::endl;
    std::cout << "Taban Mod�l Adresi: 0x" << std::hex << processManager.GetBaseModule().base << std::dec << std::endl;

    // Yap�land�rma de�erlerini al
    auto& espConfig = configManager.GetESPConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    std::cout << "[geezy_digital] ESP Durumu: " << (espConfig.enabled ? "A��k" : "Kapal�") << std::endl;
    std::cout << "[geezy_digital] ESP Tu�u: " << keyBindings.toggleEspKey << " (F1 = 112)" << std::endl;
    std::cout << "[geezy_digital] Men� Tu�u: " << keyBindings.toggleMenuKey << " (INSERT = 45)" << std::endl;
    std::cout << "[geezy_digital] ��k�� Tu�u: " << keyBindings.exitKey << " (END = 35)" << std::endl;

    // Kontrol de�erlerini olu�tur
    bool running = true;
    bool showMenu = false;

    // Basit bir konsol men�s�
    std::cout << "==================================================" << std::endl;
    std::cout << "           Kullan�labilir Komutlar:               " << std::endl;
    std::cout << "  [F1] - ESP'yi A�/Kapat                          " << std::endl;
    std::cout << "  [INSERT] - Men�y� G�ster/Gizle                  " << std::endl;
    std::cout << "  [END] - Programdan ��k                          " << std::endl;
    std::cout << "==================================================" << std::endl;

    // Ana d�ng�
    while (running) {
        // Tu� kontrollerini yap
        if (GetAsyncKeyState(keyBindings.toggleEspKey) & 1) { // F1 tu�una bas�ld���nda
            espConfig.enabled = !espConfig.enabled;
            std::cout << "[geezy_digital] ESP: " << (espConfig.enabled ? "A��k" : "Kapal�") << std::endl;
            Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
        }

        if (GetAsyncKeyState(keyBindings.toggleMenuKey) & 1) { // INSERT tu�una bas�ld���nda
            showMenu = !showMenu;
            std::cout << "[geezy_digital] Men�: " << (showMenu ? "A��k" : "Kapal�") << std::endl;
            Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
        }

        if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tu�una bas�ld���nda
            running = false;
            std::cout << "[geezy_digital] Program sonland�r�l�yor..." << std::endl;
        }

        // ESP i�levselli�i burada implemente edilecek
        if (espConfig.enabled) {
            // �rnek olarak oyuncu listesini g�sterelim
            uintptr_t entityListAddress = processManager.GD_Read<uintptr_t>(processManager.GetBaseModule().base + offsets.dwEntityList);
            if (entityListAddress) {
                // Liste bo�luklar� ve i�aret�iler
                uintptr_t listEntry = processManager.GD_Read<uintptr_t>(entityListAddress + 0x10);

                if (listEntry) {
                    // Oyuncu controller'lar� burada i�lenecek
                    // Bu sadece �rnek bir kod - ger�ek implementasyon daha detayl� olacak
                }
            }

            // ViewMatrix g�ncelleme �rne�i
            uintptr_t viewMatrixAddress = processManager.GetBaseModule().base + offsets.dwViewMatrix;
            // Matrix verilerini oku ve ESP hesaplamalar�nda kullan
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