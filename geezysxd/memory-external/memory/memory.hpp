#ifndef _GEEZY_DIGITAL_MEMORY_HPP_
#define _GEEZY_DIGITAL_MEMORY_HPP_

#include <vector>
#include <memory>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

// NT_SUCCESS makrosunu tanýmlayalým
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace geezy_digital {

    // Native Windows API fonksiyon tipleri
    typedef NTSTATUS(WINAPI* NtReadVirtualMemoryFn)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesRead);
    typedef NTSTATUS(WINAPI* NtWriteVirtualMemoryFn)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten);

    // Ýþlem modülü bilgilerini tutan yapý
    struct ModuleInfo {
        uintptr_t base = 0;
        uintptr_t size = 0;

        ModuleInfo() = default;
        ModuleInfo(uintptr_t base, uintptr_t size) : base(base), size(size) {}

        bool IsValid() const { return base != 0 && size != 0; }
    };

    // Hafýza iþlemleri için yardýmcý sýnýf
    class MemoryAccess {
    private:
        NtReadVirtualMemoryFn m_pfnNtReadVirtualMemory;
        NtWriteVirtualMemoryFn m_pfnNtWriteVirtualMemory;

    public:
        MemoryAccess() {
            m_pfnNtReadVirtualMemory = (NtReadVirtualMemoryFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtReadVirtualMemory");
            m_pfnNtWriteVirtualMemory = (NtWriteVirtualMemoryFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
        }

        // Belirli bir bellek adresinden okuma yapar
        template<typename T>
        T GD_Read(HANDLE hProcess, uintptr_t address) {
            T buffer{};
            m_pfnNtReadVirtualMemory(hProcess, (void*)address, &buffer, sizeof(T), 0);
            return buffer;
        }

        // Belirli bir bellek adresine yazma yapar
        template<typename T>
        bool GD_Write(HANDLE hProcess, uintptr_t address, const T& value) {
            NTSTATUS status = m_pfnNtWriteVirtualMemory(hProcess, (void*)address, (void*)&value, sizeof(T), 0);
            return NT_SUCCESS(status);
        }

        // Bir dizi byte'ý belirli bir bellek adresine yazar
        bool GD_WriteBytes(HANDLE hProcess, uintptr_t address, const std::vector<uint8_t>& data) {
            NTSTATUS status = m_pfnNtWriteVirtualMemory(hProcess, (void*)address, (void*)data.data(), data.size(), 0);
            return NT_SUCCESS(status);
        }

        // Ham bellek okumasý yapar
        bool GD_ReadRaw(HANDLE hProcess, uintptr_t address, void* buffer, size_t size) {
            SIZE_T bytesRead;
            NTSTATUS status = m_pfnNtReadVirtualMemory(hProcess, (PVOID)(address), buffer, static_cast<ULONG>(size), (PULONG)&bytesRead);
            if (NT_SUCCESS(status)) {
                return bytesRead == size;
            }
            return false;
        }
    };

    // Ýþlem ve bellek iþlemlerini yöneten ana sýnýf
    class ProcessManager {
    private:
        DWORD m_processId = 0;
        HANDLE m_processHandle = NULL;
        HWND m_windowHandle = NULL;
        ModuleInfo m_baseModule;
        MemoryAccess m_memoryAccess;

    public:
        ProcessManager() = default;
        ~ProcessManager() { GD_CloseProcess(); }

        // Ýþlem ID'si ile bir pencerenin handle'ýný bulur
        HWND GD_FindWindowHandleByProcessId(DWORD processId);

        // Ýþlem adýyla iþlem ID'sini bulur
        DWORD GD_FindProcessIdByName(const char* processName);

        // Pencere adýyla iþlem ID'sini bulur
        DWORD GD_FindProcessIdByWindowName(const char* windowName);

        // Ýþlem adýna göre iþleme baðlanýr
        bool GD_AttachToProcess(const char* processName);

        // Handle Hijacking kullanarak iþleme baðlanýr
        bool GD_AttachToProcessWithHijacking(const char* processName);

        // Pencere adýna göre iþleme baðlanýr
        bool GD_AttachToWindow(const char* windowName);

        // Pencere handle'ýný günceller
        bool GD_UpdateWindowHandle();

        // Ýþlem handle'ýný kapatýr
        void GD_CloseProcess();

        // Ýþlem modülünü isimle alýr
        ModuleInfo GD_GetModuleByName(const char* moduleName);

        bool GD_ReadRaw(uintptr_t address, void* buffer, size_t size) {
            return m_memoryAccess.GD_ReadRaw(m_processHandle, address, buffer, size);
        }

        // Ýþlem için bellek ayýrýr
        LPVOID GD_AllocateMemory(size_t sizeInBytes) {
            return VirtualAllocEx(m_processHandle, NULL, sizeInBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        }

        // Ýmza ile bellek içinde arama yapar
        uintptr_t GD_FindPatternInMemory(const std::vector<uint8_t>& signature, const ModuleInfo& moduleInfo = {});

        // Ýmza sonrasý offset'i okur
        template<typename T>
        uintptr_t GD_ReadOffsetFromSignature(const std::vector<uint8_t>& signature, uint8_t offset) {
            uintptr_t patternAddress = GD_FindPatternInMemory(signature);
            if (!patternAddress) return 0;

            T offsetValue = GD_Read<T>(patternAddress + offset);
            return patternAddress + offsetValue + offset + sizeof(T);
        }

        // Belirli boyutta kod boþluðu arar
        uintptr_t GD_FindCodeCave(uint32_t lengthInBytes) {
            std::vector<uint8_t> cavePattern(lengthInBytes, 0x00);
            return GD_FindPatternInMemory(cavePattern);
        }

        // Bellek adresinden okuma yapar
        template<typename T>
        T GD_Read(uintptr_t address) {
            return m_memoryAccess.GD_Read<T>(m_processHandle, address);
        }

        // Bellek adresine yazma yapar
        template<typename T>
        bool GD_Write(uintptr_t address, const T& value) {
            return m_memoryAccess.GD_Write(m_processHandle, address, value);
        }

        // Byte dizisi yazar
        bool GD_WriteBytes(uintptr_t address, const std::vector<uint8_t>& data) {
            return m_memoryAccess.GD_WriteBytes(m_processHandle, address, data);
        }

        // Çoklu offsetler ile bellek adresi okur
        uintptr_t GD_ReadMultiLevelPointer(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
            uintptr_t address = baseAddress;
            for (size_t i = 0; i < offsets.size(); i++) {
                address = GD_Read<uintptr_t>(address + offsets[i]);
                if (!address) break;
            }
            return address;
        }

        // Çoklu offsetler ile veri okur
        template<typename T>
        T GD_ReadMultiLevel(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
            uintptr_t address = baseAddress;
            for (size_t i = 0; i < offsets.size() - 1; i++) {
                address = GD_Read<uintptr_t>(address + offsets[i]);
                if (!address) return T{};
            }
            return GD_Read<T>(address + offsets.back());
        }

        // Getter metodlarý
        DWORD GetProcessId() const { return m_processId; }
        HANDLE GetProcessHandle() const { return m_processHandle; }
        HWND GetWindowHandle() const { return m_windowHandle; }
        const ModuleInfo& GetBaseModule() const { return m_baseModule; }

    private:
        // Ýþlem baðlantýsý için ortak adýmlarý gerçekleþtirir
        bool GD_InitializeProcess();
    };

} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_MEMORY_HPP_