#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <conio.h>
#include "memory/memory.hpp"
#include "memory/handle_hijack.hpp"
#include "config.hpp"
#include "weapon_skins.hpp" // Yeni eklenen header

// Yard�m men�s�n� g�ster
void ShowHelp() {
    std::cout << "==================================================" << std::endl;
    std::cout << "           Geezy Digital CS2 Skin Changer         " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Komutlar:" << std::endl;
    std::cout << " 'I' - Aktif silah bilgilerini g�ster" << std::endl;
    std::cout << " 'L' - Pop�ler kaplamalar� listele" << std::endl;
    std::cout << " 'W' - Envanterdeki silahlar� listele" << std::endl;
    std::cout << " 'C' - Aktif silah kaplamas�n� de�i�tir" << std::endl;
    std::cout << " 'S' - StatTrak de�erini de�i�tir" << std::endl;
    std::cout << " 'N' - �zel isim ayarla" << std::endl;
    std::cout << " 'R' - A��nma de�erini de�i�tir (0.0-1.0)" << std::endl;
    std::cout << " 'D' - Desen tohumu de�i�tir" << std::endl;
    std::cout << " 'H' - Bu yard�m men�s�n� g�ster" << std::endl;
    std::cout << " 'Q' - Programdan ��k" << std::endl;
    std::cout << "==================================================" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "               Geezy Digital CS2 Tool             " << std::endl;
    std::cout << "==================================================" << std::endl;

    // Yap�land�rma y�neticisi olu�tur
    geezy_digital::ConfigManager configManager;

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
    auto& appConfig = configManager.GetAppConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // Silah kaplama y�neticisi olu�tur
    geezy_digital::WeaponSkinManager skinManager(processManager, configManager);

    // Kontrol de�erlerini olu�tur
    bool running = true;

    // Komut modu i�in ekstra de�i�kenler
    bool commandMode = false;

    // Yard�m men�s�n� g�ster
    ShowHelp();

    // Ana d�ng�
    while (running) {
        // Tu� kontrollerini yap
        if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tu�una bas�ld���nda
            running = false;
            std::cout << "[geezy_digital] Program sonland�r�l�yor..." << std::endl;
        }

        if (GetAsyncKeyState(keyBindings.toggleMenuKey) & 1) { // INSERT tu�una bas�ld���nda
            commandMode = !commandMode;
            if (commandMode) {
                std::cout << "[geezy_digital] Komut modu aktif. Komut girin (H: Yard�m): ";

                // Komut girdisi i�in _getch() kullan
                char cmd = _getch();
                std::cout << cmd << std::endl;

                // Yerel oyuncu adresini al
                uintptr_t localPlayerAddress = processManager.GetBaseModule().base + offsets.dwLocalPlayer;

                // Komutu i�le
                switch (toupper(cmd)) {
                case 'H': // Yard�m
                    ShowHelp();
                    break;

                case 'I': // Bilgi
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);
                    skinManager.PrintWeaponInfo(info, skin);
                }
                break;

                case 'L': // Kaplamalar� listele
                    skinManager.ListPopularSkins();
                    break;

                case 'W': // Silahlar� listele
                    skinManager.ListPlayerWeapons(localPlayerAddress);
                    break;

                case 'C': // Kaplama de�i�tir
                {
                    skinManager.ListPopularSkins();
                    std::cout << "Yeni kaplama ID'sini girin: ";
                    int paintKit;
                    std::cin >> paintKit;

                    geezy_digital::WeaponSkin newSkin;
                    newSkin.paintKit = paintKit;

                    // Pop�ler listeden bul
                    auto it = geezy_digital::kPopularSkins.find(paintKit);
                    if (it != geezy_digital::kPopularSkins.end()) {
                        newSkin = it->second;
                    }
                    else {
                        // Varsay�lan de�erleri kullan
                        newSkin.wear = 0.01f;
                        newSkin.seed = 1;
                    }

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, newSkin)) {
                        std::cout << "Kaplama ba�ar�yla de�i�tirildi!" << std::endl;
                    }
                    else {
                        std::cout << "Kaplama de�i�tirilemedi. Silah aktif edildi mi?" << std::endl;
                    }
                }
                break;

                case 'S': // StatTrak de�i�tir
                {
                    std::cout << "Yeni StatTrak de�erini girin: ";
                    int kills;
                    std::cin >> kills;

                    // Yeni ekledi�imiz UpdateActiveWeaponStatTrak metodunu kullan�yoruz
                    if (skinManager.UpdateActiveWeaponStatTrak(localPlayerAddress, kills)) {
                        std::cout << "StatTrak de�eri ba�ar�yla g�ncellendi!" << std::endl;
                    }
                    else {
                        std::cout << "StatTrak de�eri g�ncellenemedi. Aktif silah bulunamad�." << std::endl;
                    }
                }
                break;

                case 'N': // �zel isim ayarla
                {
                    std::cout << "Yeni isim girin (max 32 karakter): ";
                    std::string name;
                    std::cin.ignore();
                    std::getline(std::cin, name);

                    // Yeni ekledi�imiz SetActiveWeaponCustomName metodunu kullan�yoruz
                    if (skinManager.SetActiveWeaponCustomName(localPlayerAddress, name)) {
                        std::cout << "�zel isim ba�ar�yla ayarland�!" << std::endl;
                    }
                    else {
                        std::cout << "�zel isim ayarlanamad�. Aktif silah bulunamad�." << std::endl;
                    }
                }
                break;

                case 'R': // A��nma de�eri de�i�tir
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);

                    std::cout << "Mevcut a��nma de�eri: " << skin.wear << std::endl;
                    std::cout << "Yeni a��nma de�erini girin (0.0-1.0): ";
                    float wear;
                    std::cin >> wear;

                    // De�eri s�n�rla
                    if (wear < 0.0f) wear = 0.0f;
                    if (wear > 1.0f) wear = 1.0f;

                    skin.wear = wear;

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, skin)) {
                        std::cout << "A��nma de�eri ba�ar�yla de�i�tirildi!" << std::endl;
                    }
                    else {
                        std::cout << "A��nma de�eri de�i�tirilemedi." << std::endl;
                    }
                }
                break;

                case 'D': // Desen tohumu de�i�tir
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);

                    std::cout << "Mevcut desen tohumu: " << skin.seed << std::endl;
                    std::cout << "Yeni desen tohumu girin: ";
                    int seed;
                    std::cin >> seed;

                    skin.seed = seed;

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, skin)) {
                        std::cout << "Desen tohumu ba�ar�yla de�i�tirildi!" << std::endl;
                    }
                    else {
                        std::cout << "Desen tohumu de�i�tirilemedi." << std::endl;
                    }
                }
                break;

                case 'Q': // ��k��
                    running = false;
                    std::cout << "[geezy_digital] Program sonland�r�l�yor..." << std::endl;
                    break;

                default:
                    std::cout << "Ge�ersiz komut. Yard�m i�in 'H' tu�una bas�n." << std::endl;
                    break;
                }

                commandMode = false;
            }
        }

        // Debug bilgileri g�ster
        if (appConfig.showDebugInfo && GetAsyncKeyState(VK_F1) & 1) {
            // Yerel oyuncu adresini al
            uintptr_t localPlayerAddress = processManager.GetBaseModule().base + offsets.dwLocalPlayer;

            // Aktif silah bilgilerini g�ster
            auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
            auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);
            skinManager.PrintWeaponInfo(info, skin);

            Sleep(150); // Tu� �ak��mas�n� �nlemek i�in
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