#ifndef _GEEZY_DIGITAL_HANDLE_HIJACK_HPP_
#define _GEEZY_DIGITAL_HANDLE_HIJACK_HPP_

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <memory>

// NT_SUCCESS makrosunu tan�mlayal�m
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace geezy_digital {
    namespace handle_hijack {

        // Handle Hijacking i�in gerekli Windows API makrolar�
#define GD_SeDebugPriv 20
#define GD_NT_SUCCESS(Status) NT_SUCCESS(Status)
#define GD_STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define GD_NtCurrentProcess ((HANDLE)(LONG_PTR) -1)
#define GD_ProcessHandleType 0x7
#define GD_SystemHandleInformation 16

// NtOpenProcess i�in gerekli yap�lar
        typedef struct _GD_UNICODE_STRING {
            USHORT Length;
            USHORT MaximumLength;
            PWCH Buffer;
        } GD_UNICODE_STRING, * PGD_UNICODE_STRING;

        typedef struct _GD_OBJECT_ATTRIBUTES {
            ULONG Length;
            HANDLE RootDirectory;
            PGD_UNICODE_STRING ObjectName;
            ULONG Attributes;
            PVOID SecurityDescriptor;
            PVOID SecurityQualityOfService;
        } GD_OBJECT_ATTRIBUTES, * PGD_OBJECT_ATTRIBUTES;

        typedef struct _GD_CLIENT_ID {
            PVOID UniqueProcess;
            PVOID UniqueThread;
        } GD_CLIENT_ID, * PGD_CLIENT_ID;

        // Handle bilgileri i�in gerekli yap�lar
        typedef struct _GD_SYSTEM_HANDLE_TABLE_ENTRY_INFO {
            ULONG ProcessId;
            BYTE ObjectTypeNumber;
            BYTE Flags;
            USHORT Handle;
            PVOID Object;
            ACCESS_MASK GrantedAccess;
        } GD_SYSTEM_HANDLE;

        typedef struct _GD_SYSTEM_HANDLE_INFORMATION {
            ULONG HandleCount;
            GD_SYSTEM_HANDLE Handles[1];
        } GD_SYSTEM_HANDLE_INFORMATION, * PGD_SYSTEM_HANDLE_INFORMATION;

        // Windows API fonksiyon prototipleri
        typedef NTSTATUS(NTAPI* GD_NtDuplicateObjectFn)(
            HANDLE SourceProcessHandle,
            HANDLE SourceHandle,
            HANDLE TargetProcessHandle,
            PHANDLE TargetHandle,
            ACCESS_MASK DesiredAccess,
            ULONG Attributes,
            ULONG Options
            );

        typedef NTSTATUS(NTAPI* GD_RtlAdjustPrivilegeFn)(
            ULONG Privilege,
            BOOLEAN Enable,
            BOOLEAN CurrentThread,
            PBOOLEAN Enabled
            );

        typedef NTSTATUS(NTAPI* GD_NtOpenProcessFn)(
            PHANDLE ProcessHandle,
            ACCESS_MASK DesiredAccess,
            PGD_OBJECT_ATTRIBUTES ObjectAttributes,
            PGD_CLIENT_ID ClientId
            );

        typedef NTSTATUS(NTAPI* GD_NtQuerySystemInformationFn)(
            ULONG SystemInformationClass,
            PVOID SystemInformation,
            ULONG SystemInformationLength,
            PULONG ReturnLength
            );

        // Handle Hijacking i�in kullan�lan s�n�f
        class GD_HandleHijacker {
        private:
            HANDLE m_processHandle = NULL;
            HANDLE m_targetProcessHandle = NULL;
            PGD_SYSTEM_HANDLE_INFORMATION m_handleInfo = nullptr;

            GD_NtDuplicateObjectFn m_pfnNtDuplicateObject = nullptr;
            GD_RtlAdjustPrivilegeFn m_pfnRtlAdjustPrivilege = nullptr;
            GD_NtOpenProcessFn m_pfnNtOpenProcess = nullptr;
            GD_NtQuerySystemInformationFn m_pfnNtQuerySystemInformation = nullptr;

        public:
            GD_HandleHijacker() {
                // ntdll.dll'den gerekli fonksiyonlar� y�kle
                HMODULE ntdll = GetModuleHandleA("ntdll");
                if (ntdll) {
                    m_pfnNtDuplicateObject = (GD_NtDuplicateObjectFn)GetProcAddress(ntdll, "NtDuplicateObject");
                    m_pfnRtlAdjustPrivilege = (GD_RtlAdjustPrivilegeFn)GetProcAddress(ntdll, "RtlAdjustPrivilege");
                    m_pfnNtOpenProcess = (GD_NtOpenProcessFn)GetProcAddress(ntdll, "NtOpenProcess");
                    m_pfnNtQuerySystemInformation = (GD_NtQuerySystemInformationFn)GetProcAddress(ntdll, "NtQuerySystemInformation");
                }
            }

            ~GD_HandleHijacker() {
                // Kaynaklar� temizle
                if (m_processHandle) CloseHandle(m_processHandle);
                if (m_targetProcessHandle && m_targetProcessHandle != INVALID_HANDLE_VALUE) {
                    CloseHandle(m_targetProcessHandle);
                }
                if (m_handleInfo) delete[](BYTE*)m_handleInfo;
            }

            // Handle ge�erli mi kontrol eder
            bool GD_IsHandleValid(HANDLE handle) const {
                return handle && handle != INVALID_HANDLE_VALUE;
            }

            // ObjectAttributes yap�s�n� ba�lat�r
            GD_OBJECT_ATTRIBUTES GD_InitObjectAttributes(PGD_UNICODE_STRING name, ULONG attributes, HANDLE hRoot, PSECURITY_DESCRIPTOR security) {
                GD_OBJECT_ATTRIBUTES object;
                object.Length = sizeof(GD_OBJECT_ATTRIBUTES);
                object.ObjectName = name;
                object.Attributes = attributes;
                object.RootDirectory = hRoot;
                object.SecurityDescriptor = security;
                return object;
            }

            // Mevcut bir handle'� ele ge�irir
            HANDLE GD_HijackExistingHandle(DWORD targetProcessId) {
                if (!m_pfnNtDuplicateObject || !m_pfnRtlAdjustPrivilege ||
                    !m_pfnNtOpenProcess || !m_pfnNtQuerySystemInformation) {
                    std::cerr << "[geezy_digital] Gerekli NT API fonksiyonlar� bulunamad�!" << std::endl;
                    return NULL;
                }

                // Debug ayr�cal�klar�n� etkinle�tir
                BOOLEAN oldPriv;
                m_pfnRtlAdjustPrivilege(GD_SeDebugPriv, TRUE, FALSE, &oldPriv);

                // Sistem handle bilgilerini alma
                DWORD size = sizeof(GD_SYSTEM_HANDLE_INFORMATION);
                m_handleInfo = (PGD_SYSTEM_HANDLE_INFORMATION) new BYTE[size];
                ZeroMemory(m_handleInfo, size);

                NTSTATUS status;
                do {
                    delete[](BYTE*)m_handleInfo;
                    size *= 1.5;

                    try {
                        m_handleInfo = (PGD_SYSTEM_HANDLE_INFORMATION) new BYTE[size];
                    }
                    catch (std::bad_alloc&) {
                        if (m_processHandle) CloseHandle(m_processHandle);
                        return NULL;
                    }

                    status = m_pfnNtQuerySystemInformation(
                        GD_SystemHandleInformation,
                        m_handleInfo,
                        size,
                        NULL
                    );

                } while (status == GD_STATUS_INFO_LENGTH_MISMATCH);

                if (!GD_NT_SUCCESS(status)) {
                    if (m_processHandle) CloseHandle(m_processHandle);
                    delete[](BYTE*)m_handleInfo;
                    m_handleInfo = nullptr;
                    return NULL;
                }

                // ��lem i�in gerekli yap�lar� haz�rla
                GD_OBJECT_ATTRIBUTES objAttributes = GD_InitObjectAttributes(NULL, 0, NULL, NULL);
                GD_CLIENT_ID clientId = { 0 };

                // T�m kullan�labilir handle'lar� tara
                for (ULONG i = 0; i < m_handleInfo->HandleCount; ++i) {
                    // Sadece s�re� handle'lar� ile ilgileniyoruz
                    if (m_handleInfo->Handles[i].ObjectTypeNumber != GD_ProcessHandleType) {
                        continue;
                    }

                    // Handle ge�erli mi kontrol et
                    if (!GD_IsHandleValid((HANDLE)m_handleInfo->Handles[i].Handle)) {
                        continue;
                    }

                    // Bu handle'� tutan s�rece ba�lan
                    clientId.UniqueProcess = (DWORD*)m_handleInfo->Handles[i].ProcessId;

                    if (m_processHandle) CloseHandle(m_processHandle);

                    status = m_pfnNtOpenProcess(
                        &m_processHandle,
                        PROCESS_DUP_HANDLE,
                        &objAttributes,
                        &clientId
                    );

                    if (!GD_NT_SUCCESS(status) || !GD_IsHandleValid(m_processHandle)) {
                        continue;
                    }

                    // Handle'� �o�alt
                    status = m_pfnNtDuplicateObject(
                        m_processHandle,
                        (HANDLE)m_handleInfo->Handles[i].Handle,
                        GD_NtCurrentProcess,
                        &m_targetProcessHandle,
                        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE,
                        0,
                        0
                    );

                    if (!GD_NT_SUCCESS(status) || !GD_IsHandleValid(m_targetProcessHandle)) {
                        continue;
                    }

                    // Hedef i�lem ID'sini kontrol et
                    if (GetProcessId(m_targetProcessHandle) != targetProcessId) {
                        CloseHandle(m_targetProcessHandle);
                        m_targetProcessHandle = NULL;
                        continue;
                    }

                    // Ba�ar�l� hijacking, ��k�� yap
                    HANDLE result = m_targetProcessHandle;
                    m_targetProcessHandle = NULL; // Sahipli�i aktard�k
                    return result;
                }

                // Ba�ar�s�z oldu
                if (m_processHandle) CloseHandle(m_processHandle);
                m_processHandle = NULL;

                return NULL;
            }
        };

        // ProcessManager s�n�f� i�in Handle Hijacking implementasyonu
        inline HANDLE GD_HijackHandle(DWORD targetProcessId) {
            GD_HandleHijacker hijacker;
            return hijacker.GD_HijackExistingHandle(targetProcessId);
        }

    } // namespace handle_hijack
} // namespace geezy_digital

#endif // _GEEZY_DIGITAL_HANDLE_HIJACK_HPP_