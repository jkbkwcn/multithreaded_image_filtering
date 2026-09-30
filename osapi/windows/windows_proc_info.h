// Based on:
// https://stackoverflow.com/questions/22949725/how-to-get-thread-state-e-g-suspended-memory-cpu-usage-start-time-priori

#pragma once

#include <winternl.h>
#include <winnt.h>
#include <cassert>

#define STATUS_SUCCESS              ((NTSTATUS) 0x00000000)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS) 0xC0000004)

typedef NTSTATUS (WINAPI* t_NtQueryInfo)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

class WindowsProcInfo
{
public:
    WindowsProcInfo()
    {
        #ifdef WIN64
            assert(sizeof(SYSTEM_THREAD_INFORMATION) == 0x50 && sizeof(SYSTEM_PROCESS_INFORMATION) == 0x100);
        #else
            assert(sizeof(SYSTEM_THREAD_INFORMATION) == 0x40 && sizeof(SYSTEM_PROCESS_INFORMATION) == 0xB8);
        #endif

        mu32_DataSize  = 1000;
        mp_Data        = NULL;
        mf_NtQueryInfo = NULL;
    }
    virtual ~WindowsProcInfo()
    {
        if (mp_Data) LocalFree(mp_Data);
    }

    // Capture all running processes and all their threads.
    // returns an API or NTSTATUS Error code or zero if successfull
    DWORD Capture()
    {
        if (!mf_NtQueryInfo)
        {
            mf_NtQueryInfo = (t_NtQueryInfo)GetProcAddress(GetModuleHandleA("NtDll.dll"), "NtQuerySystemInformation");
            if (!mf_NtQueryInfo)
                return GetLastError();
        }

        // This must run in a loop because in the mean time a new process may have started
        // and we need more buffer than u32_Needed !!
        while (true)
        {
            if (!mp_Data)
            {
                mp_Data = (BYTE*)LocalAlloc(LMEM_FIXED, mu32_DataSize);
                if (!mp_Data)
                    return GetLastError();
            }

            ULONG u32_Needed = 0;
            NTSTATUS s32_Status = mf_NtQueryInfo(SystemProcessInformation, mp_Data, mu32_DataSize, &u32_Needed);

            if (s32_Status == STATUS_INFO_LENGTH_MISMATCH) // The buffer was too small
            {
                mu32_DataSize = u32_Needed + 4000;
                LocalFree(mp_Data);
                mp_Data = NULL;
                continue;
            }
            return s32_Status;
        }
    }

    // Searches a process by a given Process Identifier
    // Capture() must have been called before!
    SYSTEM_PROCESS_INFORMATION* FindProcessByPid(DWORD u32_PID)
    {
        if (!mp_Data)
        {
            assert(mp_Data);
            return NULL;
        }

        SYSTEM_PROCESS_INFORMATION* pk_Proc = (SYSTEM_PROCESS_INFORMATION*)mp_Data;
        while (TRUE)
        {
            if ((DWORD)(DWORD_PTR)pk_Proc->UniqueProcessId == u32_PID)
                return pk_Proc;

            if (!pk_Proc->NextEntryOffset)
                return NULL;

            pk_Proc = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)pk_Proc + pk_Proc->NextEntryOffset);
        }
    }

    SYSTEM_THREAD_INFORMATION* FindThreadByTid(SYSTEM_PROCESS_INFORMATION* pk_Proc, DWORD u32_TID)
    {
        if (!pk_Proc)
        {
            assert(pk_Proc);
            return NULL;
        }

        // The first SYSTEM_THREAD structure comes immediately after the SYSTEM_PROCESS structure
        SYSTEM_THREAD_INFORMATION* pk_Thread = (SYSTEM_THREAD_INFORMATION*)((BYTE*)pk_Proc + sizeof(SYSTEM_PROCESS_INFORMATION));

        for (DWORD i=0; i<pk_Proc->NumberOfThreads; i++)
        {
            if (pk_Thread->ClientId.UniqueThread == (HANDLE)(DWORD_PTR)u32_TID)
                return pk_Thread;

            pk_Thread++;
        }
        return NULL;
    }

    DWORD IsThreadSuspended(SYSTEM_THREAD_INFORMATION* pk_Thread, BOOL* pb_Suspended)
    {
        if (!pk_Thread)
            return ERROR_INVALID_PARAMETER;

        *pb_Suspended = (pk_Thread->ThreadState == StateWait &&
                         pk_Thread->WaitReason  == Suspended);
        return 0;
    }

    static DWORD getTotalStackSize(HANDLE threadHandle) {
        if (threadHandle == nullptr) {
            return 0;
        }
        typedef struct _THREAD_BASIC_INFORMATION {
          NTSTATUS                ExitStatus;
          NT_TIB*                 TebBaseAddress;
          CLIENT_ID               ClientId;
          KAFFINITY               AffinityMask;
          KPRIORITY               Priority;
          KPRIORITY               BasePriority;
        } THREAD_BASIC_INFORMATION;

        NTSTATUS status;
        THREAD_BASIC_INFORMATION basicInfo;
        typedef NTSTATUS ( WINAPI *NQIT )( HANDLE, LONG, PVOID, ULONG, PULONG );

        /* Get the address of NtQueryInformationThread function. */
        NQIT NtQueryInformationThread = ( NQIT )GetProcAddress( GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationThread" );
        /* Get basic thread information */
        status = NtQueryInformationThread(threadHandle, ThreadBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
        unsigned long long stackLimit = 0;
        if (status == STATUS_SUCCESS) {
            stackLimit = (unsigned long long)basicInfo.TebBaseAddress->StackBase - (unsigned long long)basicInfo.TebBaseAddress->StackLimit;
        }

        return stackLimit;
    }

    static DWORD getUsedStackSize(HANDLE threadHandle) {
        return getTotalStackSize(threadHandle);
    }


private:
    BYTE*         mp_Data;
    DWORD         mu32_DataSize;
    t_NtQueryInfo mf_NtQueryInfo;
};

// Based on the 32 bit code of Sven B. Schreiber on:
// http://www.informit.com/articles/article.aspx?p=22442&seqNum=5
