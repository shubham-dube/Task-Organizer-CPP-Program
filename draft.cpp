#include <windows.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <algorithm>

class ProcessInfo
{
public:
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
    std::vector<WindowEnumerator> enumerateWindows()
    {
        std::vector<WindowEnumerator> processes;

        EnumWindows([this, &processes](HWND hwnd, LPARAM lParam) { 
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);

            // Retrieve the title of the window
            WCHAR szWindowText[MAX_LOADSTRING];
            GetWindowText(hwnd, szWindowText, MAX_LOADSTRING);

            // Add the process to the list of running processes
            // if it has not been added already.
            if (pid != 0 && !szWindowText[0])
            {
                processes.push_back(*this);
            }

            return TRUE;
            }, NULL);

        return processes;
    }
};

class ProcessEnumerator : public ProcessInfo
{
public:
    std::vector<ProcessEnumerator> enumerateProcesses()
    {
        std::vector<ProcessEnumerator> processes;

        DWORD cbNeeded;
        if (EnumProcesses(NULL, 0, &cbNeeded) == 0)
        {
            return processes;
        }

        processes.resize(cbNeeded / sizeof(DWORD));
        if (EnumProcesses(processes.data(), cbNeeded, &cbNeeded) == 0)
        {
            return processes;
        }

        // Filter out processes that do not have a top-level window
        processes.erase(std::remove_if(processes.begin(), processes.end(), [](const ProcessEnumerator& processInfo)
        {
            return processInfo.title().empty();
        }), processes.end());

        // Sort processes by title
        std::sort(processes.begin(), processes.end(), [](const ProcessEnumerator& a, const ProcessEnumerator& b)
        {
            return a.title() < b.title();
        });

        return processes;
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