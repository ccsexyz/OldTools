#include <Windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <processthreadsapi.h>

#define processName "GWX.exe"

void kill(DWORD dwProcessId)
{
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessId);
    if (process != INVALID_HANDLE_VALUE) {
        TerminateProcess(process, 0);
        CloseHandle(process);
        return;
    }
}

int killGWX()
{
    PROCESSENTRY32 p;
    PROCESSENTRY32 *info = &p;
    info->dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    BOOL bMore = Process32First(hProcessSnap, info);
    BOOL terminate = FALSE;
    while (bMore != FALSE) {
        if (!_tcscmp(processName, info->szExeFile)) {
            kill(info->th32ProcessID);
            return 1;
        }
        bMore = Process32Next(hProcessSnap, info);
    }
    CloseHandle(hProcessSnap);
    return 0;
}

int main(void)
{
    for (int i = 0; ; Sleep(100))
        if (killGWX()) 
            break;
    return 0;
}