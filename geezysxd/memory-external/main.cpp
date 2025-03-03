#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <Windows.h>
#include "memory/memory.hpp"
#include <imgui/imgui.h>

#pragma comment(lib, "Psapi.lib")

constexpr const char* GAME_PROCESS = "cs2.exe";
constexpr const char* GAME_CLIENT_MODULE = "client.dll";

int main() {
    Memory::MemoryManager memoryManager(GAME_PROCESS);
    bool gameConnected = false;
    uintptr_t clientModuleBase = 0;
    std::thread autoUpdateThread;
    memoryManager.AttachToProcess();
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
