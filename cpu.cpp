#include <windows.h>
#include <iostream>
#include <string>
#include <psapi.h>

const int NUM_SAMPLES = 10; // Number of samples to take
const int SAMPLE_INTERVAL = 100; // Interval between samples (ms)

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <process name>" << std::endl;
        return 1;
    }

    std::string processName(argv[1]);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return 1;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL success = Process32First(hSnapshot, &processEntry);
    if (!success)
    {
        std::cout << "Process32First failed: " << GetLastError() << std::endl;
        CloseHandle(hSnapshot);
        return 1;
    }

    HANDLE hProcess = NULL;
    do
    {
        if (processEntry.szExeFile == processName)
        {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processEntry.th32ProcessID);
            break;
        }

        success = Process32Next(hSnapshot, &processEntry);
    } while (success);

    CloseHandle(hSnapshot);

    if (hProcess == NULL)
    {
        std::cout << "Could not find process: " << processName << std::endl;
        return 1;
    }

    // Get system times before sampling
    FILETIME sysTimeBefore;
    GetSystemTimeAsFileTime(&sysTimeBefore);

    FILETIME creationTime, exitTime, kernelTime1, userTime1, idleTime1;
    GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime1, &userTime1);
    Sleep(SAMPLE_INTERVAL);

    FILETIME kernelTime2, userTime2, idleTime2;
    GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime2, &userTime2);

    // Get system times after sampling
    FILETIME sysTimeAfter;
    GetSystemTimeAsFileTime(&sysTimeAfter);

    // Calculate total elapsed time
    ULARGE_INTEGER sysTimeBeforeLarge, sysTimeAfterLarge, elapsedTimeLarge;
    sysTimeBeforeLarge.QuadPart = sysTimeBefore.dwLowDateTime +
                                   ((ULONG)sysTimeBefore.dwHighDateTime << 32);
    sysTimeAfterLarge.QuadPart = sysTimeAfter.dwLowDateTime +
                                  ((ULONG)sysTimeAfter.dwHighDateTime << 32);
    elapsedTimeLarge.QuadPart = sysTimeAfterLarge.QuadPart - sysTimeBeforeLarge.QuadPart;

    // Calculate CPU time used by the process
    ULARGE_INTEGER kernelTime1Large, userTime1Large, kernelTime2Large, userTime2Large;
    kernelTime1Large.QuadPart = kernelTime1.dwLowDateTime +
                                ((ULONG)kernelTime1.dwHighDateTime << 32);
    userTime1Large.QuadPart = userTime1.dwLowDateTime +
                              ((ULONG)userTime1.dwHighDateTime << 32);
    kernelTime2Large.QuadPart = kernelTime2.dwLowDateTime +
                                ((ULONG)kernelTime2.dwHighDateTime << 32);
    userTime2Large.QuadPart = userTime2.dwLowDateTime +
                              ((ULONG)userTime2.dwHighDateTime << 32);

    ULARGE_INTEGER processCPUTimeBefore, processCPUTimeAfter;
   processCPUTimeBefore.QuadPart = kernelTime1Large.QuadPart + userTime1Large.QuadPart;
    processCPUTimeAfter.QuadPart = kernelTime2Large.QuadPart + userTime2Large.QuadPart;

    // Calculate CPU usage percentage
    double cpuUsage = ((processCPUTimeAfter.QuadPart - processCPUTimeBefore.QuadPart) * 100.0) / elapsedTimeLarge.QuadPart;

    std::cout << "CPU Usage of process " << processName << ": " << cpuUsage << "%" << std::endl;

    CloseHandle(hProcess);

    return 0;
}