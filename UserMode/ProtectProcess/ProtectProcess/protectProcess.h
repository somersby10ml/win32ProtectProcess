#pragma once

#ifndef __PROTECT__PROCESS
#define __PROTECT__PROCESS

#include <windows.h>
#include <thread>
#include <tchar.h>
#include <psapi.h>

#include <vector>
#include <map>

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
const DWORD STATUS_SUCCESS = 0x00000000;
const DWORD STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;


typedef struct _handleData {
    USHORT handle;
    DWORD typeNumber;
} handleData;

std::vector<handleData> vHandle;
std::map<DWORD, std::vector<handleData>> handleMap;


namespace ProtectProcess {

#pragma region API and Flags
    const DWORD SystemHandleInformation = 0x10;
    const DWORD STATUS_INFO_LENGTH_MISMATCH = 0xC0000004L;

    typedef enum _OBJECT_INFORMATION_CLASS {
        ObjectBasicInformation,
        ObjectNameInformation,
        ObjectTypeInformation,
        ObjectAllInformation,
        ObjectDataInformation
    } OBJECT_INFORMATION_CLASS;


    typedef DWORD(WINAPI* fZwQuerySystemInformation)(
        _In_      DWORD SystemInformationClass,
        _Inout_   PVOID SystemInformation,
        _In_      ULONG  SystemInformationLength,
        _Out_opt_ PULONG ReturnLength
        );

    typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
        ULONG UniqueProcessId;
        BYTE ObjectTypeNumber;
        BYTE Flags;
        USHORT Handle;
        PVOID Object;
        ACCESS_MASK GrantedAccess;
        /*PVOID Object;
        HANDLE UniqueProcessId;
        HANDLE HandleValue;
        ACCESS_MASK GrantedAccess;
        USHORT CreatorBackTraceIndex;
        USHORT ObjectTypeIndex;
        ULONG HandleAttributes;
        PVOID Reserved;*/
    } SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

    typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR Buffer;
    } UNICODE_STRING, * PUNICODE_STRING;


    typedef NTSTATUS(NTAPI* fZwQueryObject)(
        HANDLE          Handle,
        OBJECT_INFORMATION_CLASS    ObjectInformationClass,
        PVOID           ObjectInformation,
        ULONG           ObjectInformationLength,
        PULONG          ReturnLength
        );


    typedef struct _OBJECT_NAME_INFORMATION {
        UNICODE_STRING          Name;
        WCHAR                   NameBuffer[1];
    } OBJECT_NAME_INFORMATION, * POBJECT_NAME_INFORMATION;



    typedef struct _SYSTEM_EXTENDED_HANDLE_INFORMATION {
        ULONG NumberOfHandles;
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];

    } SYSTEM_EXTENDED_HANDLE_INFORMATION, * PSYSTEM_EXTENDED_HANDLE_INFORMATION;

    typedef struct __PUBLIC_OBJECT_TYPE_INFORMATION {
        UNICODE_STRING TypeName;
        ULONG          Reserved[22];
    } PUBLIC_OBJECT_TYPE_INFORMATION, * PPUBLIC_OBJECT_TYPE_INFORMATION;

    typedef enum _PROCESSINFOCLASS : DWORD
    {
        ProcessBasicInformation = 0,
        ProcessHandleCount = 0x14,
        ProcessHandleInformation = 0x33,
        ProcessHandleTable = 0x3A,
    } PROCESSINFOCLASS, * PPROCESSINFOCLASS;

    typedef struct _PROCESS_BASIC_INFORMATION {
        PVOID Reserved1;
        PVOID PebBaseAddress;
        PVOID Reserved2[2];
        ULONG_PTR UniqueProcessId;
        PVOID Reserved3;

    } PROCESS_BASIC_INFORMATION, * PPROCESS_BASIC_INFORMATION;

    typedef DWORD(WINAPI* fZwQueryInformationProcess)(
        HANDLE           ProcessHandle,
        PROCESSINFOCLASS ProcessInformationClass,
        PVOID            ProcessInformation,
        ULONG            ProcessInformationLength,
        PULONG           ReturnLength
        );

#pragma endregion

   

    void EnableSystemPriv(void);
    unsigned int __stdcall ThreadFunction(PVOID pData);
    BOOL IsElevated();
    bool isRun;


    void Start() {
        EnableSystemPriv();

        int val = 10;
        DWORD dwThreadID;
        HANDLE m_Thread = (HANDLE)_beginthreadex(NULL, NULL, ThreadFunction, &val, 0, (unsigned int*)&dwThreadID);

        if (m_Thread) {
            CloseHandle(m_Thread);
        }
    }

    void Stop() {
        isRun = false;
    }


    // 이쪽으로 넘어옴
    // return true 하면 차단
    // return false 하면 차단안함
    bool checkClose(HANDLE hProcess, ULONG pid) {
        char Buffer[260] = { 0, };
        GetModuleFileNameExA(hProcess, NULL, Buffer, _countof(Buffer));
        printf("PID: %d  FilePath: %s\n", pid, Buffer);

        // ExitProcess(0);
        return true;
    }


    unsigned int __stdcall ThreadFunction(PVOID pData) {

        fZwQuerySystemInformation ZwQuerySystemInformation = (fZwQuerySystemInformation)GetProcAddress(LoadLibraryW(L"ntdll.dll"), "ZwQuerySystemInformation");
        fZwQueryObject ZwQueryObject = (fZwQueryObject)GetProcAddress(LoadLibraryW(L"ntdll.dll"), "ZwQueryObject");
        fZwQueryInformationProcess ZwQueryInformationProcess = (fZwQueryInformationProcess)GetProcAddress(LoadLibraryW(L"ntdll.dll"), "ZwQueryInformationProcess");
        DWORD CurrentProcessId = GetCurrentProcessId();

        isRun = true;
        while (isRun) {

            LPBYTE Memory = new BYTE[1000];
            DWORD MemorySize = 1000;

            DWORD dwReturnSize = 0;
            DWORD Status;

            while(1) {
                Status = ZwQuerySystemInformation(SystemHandleInformation, Memory, MemorySize, &dwReturnSize);
                if (Status == STATUS_SUCCESS) break;
                    
                if (Status == STATUS_INFO_LENGTH_MISMATCH) {
                    delete[] Memory;
                    Memory = new BYTE[dwReturnSize];
                    MemorySize = dwReturnSize;
                }
                else {
                    // other error
                    printf("ZwQuerySystemInformation Error : %08X\n", Status);
                    return 1;
                }
            }

            PSYSTEM_EXTENDED_HANDLE_INFORMATION pInformation = (PSYSTEM_EXTENDED_HANDLE_INFORMATION)Memory;

            handleMap.clear();
            for (ULONG i = 0; i < pInformation->NumberOfHandles; i++) {
                // 해당 핸들을 가진 PID 가 나라면 패스
                if (pInformation->Handles[i].UniqueProcessId == CurrentProcessId)
                    continue;

                handleData obj;
                obj.handle = pInformation->Handles[i].Handle;
                obj.typeNumber = pInformation->Handles[i].ObjectTypeNumber;

                std::map<DWORD, std::vector<handleData>>::iterator iter;
                iter = handleMap.find(pInformation->Handles[i].UniqueProcessId);
                if (iter == handleMap.end())
                {
                    // not found
                    std::vector<handleData> data;
                    data.push_back(obj);
                    handleMap.insert(std::pair<DWORD, std::vector<handleData>>(pInformation->Handles[i].UniqueProcessId, data));
                }
                else {
                    // found
                    iter->second.push_back(obj);
                }

            }

            
            std::map<DWORD, std::vector<handleData>>::iterator iter = handleMap.begin();
            DWORD processObject = -1;

            while (iter != handleMap.end()) {

                DWORD dwProcessID = iter->first;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, dwProcessID);
                if (hProcess) {

                    for (std::vector<handleData>::iterator iterData = iter->second.begin(); iterData != iter->second.end(); iterData++)
                    {
                        // Process Type 을 얻어왔고
                        if (processObject != -1 && processObject != iterData->typeNumber) {
                            continue;
                        }

                        BYTE Buffer[260] = { 0, };
                        DWORD dwObjectResult;

                        HANDLE copyHandle = nullptr; // 복사될핸들
                        bool s = DuplicateHandle(hProcess, (HANDLE)iterData->handle, GetCurrentProcess(),
                            &copyHandle, MAXIMUM_ALLOWED, false, DUPLICATE_SAME_ACCESS);
                        // DUPLICATE_CLOSE_SOURCE DUPLICATE_SAME_ACCESS
                        if (!s) {
                            // 핸들복제실패
                            continue;
                        }
                        ZwQueryObject(copyHandle, ObjectTypeInformation, Buffer, sizeof(Buffer), &dwObjectResult);
                        PPUBLIC_OBJECT_TYPE_INFORMATION pType = (PPUBLIC_OBJECT_TYPE_INFORMATION)Buffer;

                        if (pType->TypeName.Buffer && wcscmp(pType->TypeName.Buffer, L"Process") == 0) {
                            processObject = iterData->typeNumber;
                            // GetHandleInformation  를 사용하면 상속된 핸들이나 보호된 핸들을 알수있음
                            PROCESS_BASIC_INFORMATION PBI = { 0, };
                            DWORD dwBufferSize = 8000;
                            DWORD dwLength = 8000;


                            DWORD status1 = ZwQueryInformationProcess(copyHandle, ProcessBasicInformation, &PBI, sizeof(PBI), &dwLength);
                            if (PBI.UniqueProcessId == CurrentProcessId) {
                                // printf("Detect Process ID : %d\n", pInformation->Handles[i].UniqueProcessId);
                                if (checkClose(hProcess, dwProcessID)) {
                                    CloseHandle(copyHandle);
                                    DuplicateHandle(hProcess, (HANDLE)iterData->handle, GetCurrentProcess(),
                                        &copyHandle, MAXIMUM_ALLOWED, false, DUPLICATE_CLOSE_SOURCE);
                                }

                                CloseHandle(copyHandle);
                            }

                        }

                    }

                    CloseHandle(hProcess);
                    hProcess = NULL;
                }

                iter++;
            }

            delete[] Memory;
            Sleep(10);
        }


        return true;
    }

    void EnableSystemPriv(void) {
        HANDLE  hToken = NULL;
        LUID   luidDebug;
        TOKEN_PRIVILEGES tp;

        if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return;

        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidDebug)) {
            CloseHandle(hToken);
            return;
        }
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luidDebug;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
            CloseHandle(hToken);
        }
        CloseHandle(hToken);
    }

    BOOL IsElevated() {
        BOOL fRet = FALSE;
        HANDLE hToken = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION Elevation;
            DWORD cbSize = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
                fRet = Elevation.TokenIsElevated;
            }
        }
        if (hToken) {
            CloseHandle(hToken);
        }
        return fRet;
    }

};




#endif // !__PROTECT__PROCESS
