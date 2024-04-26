#include <windows.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <algorithm>
#define MAX_LOADSTRING 1000

class ProcessInfo
{
public:
    ProcessInfo() {}
    ProcessInfo(DWORD pid, const std::wstring& title)
        : pid_(pid), title_(title) {}

    DWORD pid() const { return pid_; }
    const std::wstring& title() const { return title_; }

protected:
    DWORD pid_;
    std::wstring title_;
};

class WindowEnumerator : public ProcessInfo
{
public:
    WindowEnumerator() {}

    static std::vector<WindowEnumerator> enumerateWindows()
    {
        std::vector<WindowEnumerator> processes;

        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&processes));

        return processes;
    }

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        std::vector<WindowEnumerator>* processes = reinterpret_cast<std::vector<WindowEnumerator>*>(lParam);

        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        WCHAR szWindowText[MAX_LOADSTRING];
        int length = GetWindowText(hwnd, szWindowText, MAX_LOADSTRING);
        if (length > 0)
        {
            std::wstring title(szWindowText, length);
            processes->emplace_back(pid, title);
        }

        return TRUE;
    }
};

class ProcessEnumerator : public ProcessInfo
{
public:
    std::vector<ProcessInfo> enumerateProcesses()
    {
        std::vector<DWORD> processIds;
        DWORD cbNeeded;
        if (EnumProcesses(NULL, 0, &cbNeeded) == 0)
        {
            return {};
        }

        processIds.resize(cbNeeded / sizeof(DWORD));
        if (EnumProcesses(processIds.data(), cbNeeded, &cbNeeded) == 0)
        {
            return {};
        }

        std::vector<ProcessInfo> processes;
        for (DWORD pid : processIds)
        {
            std::wstring title;
            if (getProcessTitle(pid, title))
            {
                processes.emplace_back(pid, title);
            }
        }

        std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.title() < b.title();
        });

        return processes;
    }

protected:
bool ProcessEnumerator::getProcessTitle(DWORD pid, std::wstring& title)
{
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,0,pid);
    if (h_process == NULL)
    {
        return false;
    }
    // BOOL result = EnumProcesses(process_id, sizeof(process_id), &returned_bytes);
    WCHAR windowText[MAX_LOADSTRING];
    if (GetWindowText(h_process, windowText, MAX_LOADSTRING) == 0)
    {
        return false;
    }

    title = windowText;
    return true;
} 

};

int main()
{
    WindowEnumerator windowEnumerator;
    std::vector<WindowEnumerator> windows = windowEnumerator.enumerateWindows();

    ProcessEnumerator processEnumerator;
    std::vector<ProcessEnumerator> processes = processEnumerator.enumerateProcesses();

    // Categorize tasks based on their title
    // ...

    return 0;
}