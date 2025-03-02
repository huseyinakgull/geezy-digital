#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <conio.h>
#include "memory/memory.hpp"
#include "memory/handle_hijack.hpp"
#include "config.hpp"
#include "weapon_skins.hpp" // Yeni eklenen header

// Yardým menüsünü göster
void ShowHelp() {
    std::cout << "==================================================" << std::endl;
    std::cout << "           Geezy Digital CS2 Skin Changer         " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Komutlar:" << std::endl;
    std::cout << " 'I' - Aktif silah bilgilerini göster" << std::endl;
    std::cout << " 'L' - Popüler kaplamalarý listele" << std::endl;
    std::cout << " 'W' - Envanterdeki silahlarý listele" << std::endl;
    std::cout << " 'C' - Aktif silah kaplamasýný deðiþtir" << std::endl;
    std::cout << " 'S' - StatTrak deðerini deðiþtir" << std::endl;
    std::cout << " 'N' - Özel isim ayarla" << std::endl;
    std::cout << " 'R' - Aþýnma deðerini deðiþtir (0.0-1.0)" << std::endl;
    std::cout << " 'D' - Desen tohumu deðiþtir" << std::endl;
    std::cout << " 'H' - Bu yardým menüsünü göster" << std::endl;
    std::cout << " 'Q' - Programdan çýk" << std::endl;
    std::cout << "==================================================" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "               Geezy Digital CS2 Tool             " << std::endl;
    std::cout << "==================================================" << std::endl;

    // Yapýlandýrma yöneticisi oluþtur
    geezy_digital::ConfigManager configManager;

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
    auto& appConfig = configManager.GetAppConfig();
    auto& keyBindings = configManager.GetKeyBindings();
    auto& offsets = configManager.GetOffsets();

    // Silah kaplama yöneticisi oluþtur
    geezy_digital::WeaponSkinManager skinManager(processManager, configManager);

    // Kontrol deðerlerini oluþtur
    bool running = true;

    // Komut modu için ekstra deðiþkenler
    bool commandMode = false;

    // Yardým menüsünü göster
    ShowHelp();

    // Ana döngü
    while (running) {
        // Tuþ kontrollerini yap
        if (GetAsyncKeyState(keyBindings.exitKey) & 1) { // END tuþuna basýldýðýnda
            running = false;
            std::cout << "[geezy_digital] Program sonlandýrýlýyor..." << std::endl;
        }

        if (GetAsyncKeyState(keyBindings.toggleMenuKey) & 1) { // INSERT tuþuna basýldýðýnda
            commandMode = !commandMode;
            if (commandMode) {
                std::cout << "[geezy_digital] Komut modu aktif. Komut girin (H: Yardým): ";

                // Komut girdisi için _getch() kullan
                char cmd = _getch();
                std::cout << cmd << std::endl;

                // Yerel oyuncu adresini al
                uintptr_t localPlayerAddress = processManager.GetBaseModule().base + offsets.dwLocalPlayer;

                // Komutu iþle
                switch (toupper(cmd)) {
                case 'H': // Yardým
                    ShowHelp();
                    break;

                case 'I': // Bilgi
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);
                    skinManager.PrintWeaponInfo(info, skin);
                }
                break;

                case 'L': // Kaplamalarý listele
                    skinManager.ListPopularSkins();
                    break;

                case 'W': // Silahlarý listele
                    skinManager.ListPlayerWeapons(localPlayerAddress);
                    break;

                case 'C': // Kaplama deðiþtir
                {
                    skinManager.ListPopularSkins();
                    std::cout << "Yeni kaplama ID'sini girin: ";
                    int paintKit;
                    std::cin >> paintKit;

                    geezy_digital::WeaponSkin newSkin;
                    newSkin.paintKit = paintKit;

                    // Popüler listeden bul
                    auto it = geezy_digital::kPopularSkins.find(paintKit);
                    if (it != geezy_digital::kPopularSkins.end()) {
                        newSkin = it->second;
                    }
                    else {
                        // Varsayýlan deðerleri kullan
                        newSkin.wear = 0.01f;
                        newSkin.seed = 1;
                    }

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, newSkin)) {
                        std::cout << "Kaplama baþarýyla deðiþtirildi!" << std::endl;
                    }
                    else {
                        std::cout << "Kaplama deðiþtirilemedi. Silah aktif edildi mi?" << std::endl;
                    }
                }
                break;

                case 'S': // StatTrak deðiþtir
                {
                    std::cout << "Yeni StatTrak deðerini girin: ";
                    int kills;
                    std::cin >> kills;

                    // Yeni eklediðimiz UpdateActiveWeaponStatTrak metodunu kullanýyoruz
                    if (skinManager.UpdateActiveWeaponStatTrak(localPlayerAddress, kills)) {
                        std::cout << "StatTrak deðeri baþarýyla güncellendi!" << std::endl;
                    }
                    else {
                        std::cout << "StatTrak deðeri güncellenemedi. Aktif silah bulunamadý." << std::endl;
                    }
                }
                break;

                case 'N': // Özel isim ayarla
                {
                    std::cout << "Yeni isim girin (max 32 karakter): ";
                    std::string name;
                    std::cin.ignore();
                    std::getline(std::cin, name);

                    // Yeni eklediðimiz SetActiveWeaponCustomName metodunu kullanýyoruz
                    if (skinManager.SetActiveWeaponCustomName(localPlayerAddress, name)) {
                        std::cout << "Özel isim baþarýyla ayarlandý!" << std::endl;
                    }
                    else {
                        std::cout << "Özel isim ayarlanamadý. Aktif silah bulunamadý." << std::endl;
                    }
                }
                break;

                case 'R': // Aþýnma deðeri deðiþtir
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);

                    std::cout << "Mevcut aþýnma deðeri: " << skin.wear << std::endl;
                    std::cout << "Yeni aþýnma deðerini girin (0.0-1.0): ";
                    float wear;
                    std::cin >> wear;

                    // Deðeri sýnýrla
                    if (wear < 0.0f) wear = 0.0f;
                    if (wear > 1.0f) wear = 1.0f;

                    skin.wear = wear;

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, skin)) {
                        std::cout << "Aþýnma deðeri baþarýyla deðiþtirildi!" << std::endl;
                    }
                    else {
                        std::cout << "Aþýnma deðeri deðiþtirilemedi." << std::endl;
                    }
                }
                break;

                case 'D': // Desen tohumu deðiþtir
                {
                    auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
                    auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);

                    std::cout << "Mevcut desen tohumu: " << skin.seed << std::endl;
                    std::cout << "Yeni desen tohumu girin: ";
                    int seed;
                    std::cin >> seed;

                    skin.seed = seed;

                    if (skinManager.UpdateActiveWeaponSkin(localPlayerAddress, skin)) {
                        std::cout << "Desen tohumu baþarýyla deðiþtirildi!" << std::endl;
                    }
                    else {
                        std::cout << "Desen tohumu deðiþtirilemedi." << std::endl;
                    }
                }
                break;

                case 'Q': // Çýkýþ
                    running = false;
                    std::cout << "[geezy_digital] Program sonlandýrýlýyor..." << std::endl;
                    break;

                default:
                    std::cout << "Geçersiz komut. Yardým için 'H' tuþuna basýn." << std::endl;
                    break;
                }

                commandMode = false;
            }
        }

        // Debug bilgileri göster
        if (appConfig.showDebugInfo && GetAsyncKeyState(VK_F1) & 1) {
            // Yerel oyuncu adresini al
            uintptr_t localPlayerAddress = processManager.GetBaseModule().base + offsets.dwLocalPlayer;

            // Aktif silah bilgilerini göster
            auto info = skinManager.GetActiveWeaponInfo(localPlayerAddress);
            auto skin = skinManager.GetActiveWeaponSkin(localPlayerAddress);
            skinManager.PrintWeaponInfo(info, skin);

            Sleep(150); // Tuþ çakýþmasýný önlemek için
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