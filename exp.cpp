#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <tlhelp32.h>
#include <conio.h>
#include <map>
#include <algorithm>
#include <memory>
using namespace std;
class ProcessData {
public:
    string process_name;
    string app_name;
    string category;
};

class Process {
public:
    Process() {}
    void new_process(const DWORD &process_id, const string &process_name, const string& category) {
        this->process_id = process_id;
        this->process_name = process_name;
        this->category = category;
    }

    const DWORD get_process_id() const { return process_id; }
    const string get_process_name() const { return process_name; }
    const string get_category() const { return category; }

private:
    DWORD process_id;
    string process_name;
    string category;
};

class Application : public Process {
public:
    Application() {}
    void new_app(const DWORD &process_id, const string &process_name, 
                 const string& category, const string &name) {
        new_process(process_id, process_name, category);
        this->name = name;
        this->memory_usage = get_memory_usage(process_id);
        this->cpu_usage = getCpuUsage(process_id);
    }

    const string get_name() const { return name; }
    const SIZE_T get_memory_usage() const { return memory_usage; }
    const double get_cpu_usage() const { return cpu_usage; }

private:
    string name;
    SIZE_T memory_usage;
    double cpu_usage;
};

using Category = std::vector<Application>;
using CategoryMap = std::map<string, Category>;

CategoryMap readProcessDataFromFile(const string& filename);
ProcessData searchProcess(const CategoryMap& processData, const string& process_name);
double get_memory_usage(DWORD process_id);
string get_process_name(DWORD process_id);
double getCpuUsage(DWORD pId);
void print_categories(const CategoryMap& category_apps, int kk);
void add_in_categories(CategoryMap& category_apps, const std::vector<Application>& all_apps);
void export_processes(const CategoryMap& category_apps, std::ofstream& file);

//--------------------------------Main Function-----------------------------------------//
int main() {
    CategoryMap processData = readProcessDataFromFile("app_database.txt");
    std::vector<Application> all_apps;
    int x;

    while (1) {
        std::cout << "1. See live Processes (Category-wise)\n"
                  << "2. See All Process\n"
                  << "3. Export Process Running Data (Category-wise)\n"
                  << "4. Exit\n";
        std::cin >> x;

        switch (x) {
        case 1:
            while (1) {
                std::vector<Application> all_apps_copy(all_apps);
                CategoryMap category_apps = readProcessDataFromFile("app_database.txt");
                add_in_categories(category_apps, all_apps_copy);
                system("cls");
                print_categories(category_apps, 1);

                if (_kbhit()) {
                    char key = _getch();
                    if (key == 'q' || key == 'Q') {
                        break;
                    }
                }
            }
            break;

        case 2:
            system("cls");
            std::cout << "All Processes:\n";
            for (const auto& app : all_apps) {
                std::cout << app.get_process_name() << " ("
                          << app.get_category() << ")\n";
            }
            std::cout << "\nPress any key to continue...";
            _getch();
            break;

        case 3: {
            std::ofstream file("processes.txt");
            if (!file.is_open()) {
                std::cout << "Error: Could not open file for export.\n";
                break;
            }
            CategoryMap category_apps = readProcessDataFromFile("app_database.txt");
            add_in_categories(category_apps, all_apps);
            export_processes(category_apps, file);
            file.close();
            std::cout << "Processes exported to processes.txt\n";
            break;
        }

        case -1:
            return 0;

        default:
            std::cout << "Invalid option. Try again.\n";
            break;
        }
    }

    return 0;
}

CategoryMap readProcessDataFromFile(const string& filename) {
    std::ifstream file(filename);
    CategoryMap processData;
    if (!file.is_open()) {
        std::cout << "Error: Could not open file.\n";
        return processData;
    }

    string line;
    while (getline(file, line)) {
        std::istringstream iss(line);
        string process_name, app_name, category;
        getline(iss, process_name, ':');
        getline(iss, app_name, ',');
        getline(iss, category, '\n');
        processData[process_name] = Category();
        processData[process_name].emplace_back(Application());
        processData[process_name].back().new_app(GetCurrentProcessId(), process_name, category, app_name);
    }

    file.close();
    return processData;
}

ProcessData searchProcess(const CategoryMap& processData, const string& process_name) {
    for (const auto& process : processData) {
        if (process.first == process_name) {
            return { process_name, process.second.back().get_name(), process.second.back().get_category() };
        }
    }
    return { "", "", "" };
}

double get_memory_usage(DWORD process_id) {
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if (h_process == NULL) {
        return 0.0;
    }

    PROCESS_MEMORY_COUNTERS memory_values;
    BOOL result = GetProcessMemoryInfo(h_process, &memory_values, sizeof(memory_values));
    if (result == FALSE) {
        CloseHandle(h_process);
        return 0.0;
    }

    CloseHandle(h_process);
    return (double)memory_values.WorkingSetSize / (1024 * 1024);
}

string get_process_name(DWORD process_id) {
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if (h_process == NULL) {
        return "";
    }

    TCHAR process_name[1020];

    if (GetModuleBaseName(h_process, NULL, process_name, 1020)) {
        CloseHandle(h_process);
        return string(process_name);
    }
    else {
        CloseHandle(h_process);
        return "";
    }
}

double getCpuUsage(DWORD pId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pId);

    if (hProcess == NULL) {
        return -1.0;
    }

    FILETIME creationTime, exitTime, kernelTime, userTime;
    ULARGE_INTEGER kernelTimeInt, userTimeInt, totalTimeUsed, totalTimeElapsed;
    BOOL result = GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime);
    if (!result) {
        CloseHandle(hProcess);
        return -1.0;
    }

    kernelTimeInt.LowPart = kernelTime.dwLowDateTime;
    kernelTimeInt.HighPart = kernelTime.dwHighDateTime;
    userTimeInt.LowPart = userTime.dwLowDateTime;
    userTimeInt.HighPart = userTime.dwHighDateTime;

    FILETIME idleTimeSys, kernelTimeSys, userTimeSys;
    ULARGE_INTEGER idleTimeSysInt, kernelTimeSysInt, userTimeSysInt, totalTimeUsedSys;
    BOOL result1 = GetSystemTimes(&idleTimeSys, &kernelTimeSys, &userTimeSys);
    if (!result1) {
        CloseHandle(hProcess);
        return -1.0;
    }

    kernelTimeSysInt.LowPart = kernelTimeSys.dwLowDateTime;
    kernelTimeSysInt.HighPart = kernelTimeSys.dwHighDateTime;
    userTimeSysInt.LowPart = userTimeSys.dwLowDateTime;
    userTimeSysInt.HighPart = userTimeSys.dwHighDateTime;
    idleTimeSysInt.LowPart = idleTimeSys.dwLowDateTime;
    idleTimeSysInt.HighPart = idleTimeSys.dwHighDateTime;

    totalTimeUsedSys.QuadPart = kernelTimeSysInt.QuadPart + userTimeSysInt.QuadPart - idleTimeSysInt.QuadPart;
    totalTimeUsed.QuadPart = kernelTimeInt.QuadPart + userTimeInt.QuadPart;

    double cpuUsage = (double)totalTimeUsed.QuadPart / totalTimeUsedSys.QuadPart;

    CloseHandle(hProcess);
    return cpuUsage * 100.0;
}

void print_categories(const CategoryMap& category_apps, int kk) {
    const int COL_WIDTH = 33;

    for (const auto& category : category_apps) {
        std::cout << " |-------------------------------------------------------------------------------|\n";
        std::cout << " |                    Category :- " << std::left << std::setw(47) << category.first
                  << "|\n";
        std::cout << " | " << std::left << std::setw(5) << "S no."
                  << " | " << std::left << std::setw(COL_WIDTH) << "Process"
                  << " | " << std::left << std::setw(COL_WIDTH) << "CPU Usage"
                  << " | " << std::left << std::setw(COL_WIDTH) << "Memory Usage"
                  << " | " << std::left << std::setw(COL_WIDTH) << "App Name" << " |\n";
        std::cout << " |-------------------------------------------------------------------------------|\n";
        int i = 1;
        for (const auto& app : category.second) {
            std::cout << " | " << std::left << std::setw(5) << i++
                      << " | " << std::left << std::setw(COL_WIDTH) << app.get_process_name()
                      << " | " << std::left << std::setw(COL_WIDTH) << std::fixed << std::setprecision(2) << app.get_cpu_usage()
                      << " | " << std::left << std::setw(COL_WIDTH) << std::setprecision(2) << app.get_memory_usage()
                      << " | " << std::left << std::setw(COL_WIDTH) << app.get_name() << " |\n";
        }
        std::cout << " |-------------------------------------------------------------------------------|\n\n";
    }
}

void add_in_categories(CategoryMap& category_apps, const std::vector<Application>& all_apps) {
    for (const auto& app : all_apps) {
        auto it = category_apps.find(app.get_category());
        if (it != category_apps.end()) {
            it->second.emplace_back(app);
        }
    }
}

void export_processes(const CategoryMap& category_apps, std::ofstream& file) {
    for (const auto& category : category_apps) {
        file<< "Category: " << category.first << "\n";
        for (const auto& app : category.second) {
            file << app.get_process_name() << ","
                 << app.get_name() << ","
                 << app.get_category() << "\n";
        }
        file << "\n";
    }
}