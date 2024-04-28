#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <tlhelp32.h>
using namespace std;

class CpuUsage {
private:
    ULARGE_INTEGER _prevSysKernel;
    ULARGE_INTEGER _prevSysUser;
    ULARGE_INTEGER _prevProcKernel;
    ULARGE_INTEGER _prevProcUser;
    ULARGE_INTEGER _procTotal;
    ULARGE_INTEGER _sysTotal;
    double _cpuUsage;
    std::string _processName;

public:
    CpuUsage(const std::string& processName) : _processName(processName) {
        ZeroMemory(&_prevSysKernel, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevSysUser, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevProcKernel, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevProcUser, sizeof(LARGE_INTEGER));
        _cpuUsage = 0.0;
    }
    CpuUsage(){}

    void setProcessName(const std::string& processName)  {
        _processName = processName;
        ZeroMemory(&_prevSysKernel, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevSysUser, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevProcKernel, sizeof(LARGE_INTEGER));
        ZeroMemory(&_prevProcUser, sizeof(LARGE_INTEGER));
        _cpuUsage = 0.0;
    }

    double getCpuUsage() {
        FILETIME creationTime, exitTime, kernelTime, userTime;
        GetProcessTimes(_getProcessByName(_processName), &creationTime, &exitTime, &kernelTime, &userTime);

        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        _procTotal.QuadPart = user.QuadPart + kernel.QuadPart;

        FILETIME sysKernel, sysUser, sysIdle;
        GetSystemTimes(&sysIdle, &sysKernel, &sysUser);

        ULARGE_INTEGER sysKernelLarge, sysUserLarge, sysIdleLarge;
        sysKernelLarge.LowPart = sysKernel.dwLowDateTime;
        sysKernelLarge.HighPart = sysKernel.dwHighDateTime;
        sysUserLarge.LowPart = sysUser.dwLowDateTime;
        sysUserLarge.HighPart = sysUser.dwHighDateTime;
        sysIdleLarge.LowPart = sysIdle.dwLowDateTime;
        sysIdleLarge.HighPart = sysIdle.dwHighDateTime;

        _sysTotal.QuadPart = sysKernelLarge.QuadPart + sysUserLarge.QuadPart;

        double cpuUsage = static_cast<double>(_procTotal.QuadPart - _prevProcKernel.QuadPart - _prevProcUser.QuadPart) /
                          static_cast<double>(_sysTotal.QuadPart - _prevSysKernel.QuadPart - _prevSysUser.QuadPart);

        _prevProcKernel = kernel;
        _prevProcUser = user;
        _prevSysKernel = sysKernelLarge;
        _prevSysUser = sysUserLarge;

        return cpuUsage * 100.0;
    }

private:
    HANDLE _getProcessByName(const std::string& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return NULL;
        }

        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &processEntry)) {
            CloseHandle(hSnapshot);
            return NULL;
        }

        do {
            if (std::string(processEntry.szExeFile) == processName) {
                CloseHandle(hSnapshot);
                return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processEntry.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &processEntry));

        CloseHandle(hSnapshot);
        return NULL;
    }
};

class ProcessData {
    public:
    string process_name;
    string app_name;
    string category;
};
vector<ProcessData> readProcessDataFromFile(const string& filename);
ProcessData searchProcess(const vector<ProcessData>& processData, const string& process_name);

class Process {
public:
    Process(){}
    void new_process(const DWORD &process_id, const string &process_name, const string& category){
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

class Application: public Process {
public:
    Application(){}
    void new_app(const DWORD &process_id, const string &process_name, 
    const string& category, const string &name){
        new_process(process_id, process_name, category);
        this->name = name;
    }

    const string get_name() const { return name; }
    const SIZE_T get_memory_usage() const { return memory_usage;}

private:
    string name;
    SIZE_T memory_usage;
    
};

class Category {
    public:
        Category(){}
        void add_app(const Application &app){
            apps.push_back(app);
        }
        vector<Application> get_app() { return apps; }
        string category;

    private:
        vector<Application> apps;
};

string get_process_name(DWORD process_id);
BOOL get_all_processes(vector<Application> &all_apps, DWORD &size, vector<ProcessData> processData);
void print_categories(vector<Category> &category_apps, int kk);
void add_in_categories(vector<Category> &category_apps, vector<Application> all_apps );


//--------------------------------Main Function-----------------------------------------//
int main() {
    vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    vector<Application> all_apps;

    while(1) {
        static DWORD size=0;
        DWORD new_size;
        BOOL result = get_all_processes(all_apps, new_size, processData);
        if(!result){
            cout << "Processes Retrieval Failed" << endl;
        }
        
        if(size != new_size && result){
            system("cls");
            vector<Category> C;
            add_in_categories(C,all_apps);
            print_categories(C,1);
            size = new_size;
            C.clear();
        }
        all_apps.clear();
        Sleep(500);
    }

    return 0;
}

BOOL get_all_processes(vector<Application> &all_apps, DWORD &size, vector<ProcessData> processData){
    DWORD process_id[1000];
    DWORD returned_bytes;
    BOOL result = EnumProcesses(process_id, sizeof(process_id), &returned_bytes);
    size = returned_bytes/sizeof(DWORD);

    if(result){
        for(int i=0;i<int(size);i++){
            string process_name = get_process_name(process_id[i]);
            ProcessData P = searchProcess(processData, process_name);
            Application A;
            A.new_app(process_id[i], process_name, P.category, P.app_name);
            all_apps.push_back(A);
        }
    }
    return result;
}

string get_process_name(DWORD process_id){
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if(h_process == NULL){
        return "Access Denied";
    }

    TCHAR process_name[1020];

    if(GetModuleBaseName(h_process, NULL, process_name, 1020)){
        CloseHandle(h_process);
        return string(process_name);
    }
    else {
        CloseHandle(h_process);
        return "Unknown";
    }
}

vector<ProcessData> readProcessDataFromFile(const string& filename) {
    ifstream file(filename);
    vector<ProcessData> processData;
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string process_name, app_name,category;
            getline(iss, process_name, ':');
            getline(iss, app_name, ',');
            getline(iss, category, '\n');
            processData.push_back({process_name,app_name, category});
        }
        file.close();
    }
    return processData;
}

ProcessData searchProcess(const vector<ProcessData>& processData, const string& process_name) {
    for (const auto& process : processData) {
        if (process.process_name == process_name) {
            return process;
        }
    }
    return {"", "", ""};
}

void print_categories(vector<Category> &category_apps, int kk){
    const int COL_WIDTH = 33;

    for(int i=0;i<int(category_apps.size());i=i+1){
        cout << "\r |-------------------------------------------------------------------------------|\n";
       cout << " |                    Category :- " << left << setw(47) << category_apps[i].category
            << "|"
            << "\n | " << left << setw(5) << "S no."
            << " | " << left << setw(COL_WIDTH) << "Process"
            << " | " << left << setw(COL_WIDTH) << "CPU Usage"
            << " | " << left << setw(COL_WIDTH) << "App Name" << " |\n";
        cout << " |-------------------------------------------------------------------------------|";
        for(int j=0;j<int(category_apps[i].get_app().size());j++){
            CpuUsage cpu_usage(category_apps[i].get_app()[j].get_process_name());
           cout << "\n | " << left << setw(5) << kk++
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_process_name()
                << " | " << left << setw(COL_WIDTH) << cpu_usage.getCpuUsage()
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_name() << " |";
        }
        cout << "\n |-------------------------------------------------------------------------------|\n\n";
        // cout << " O------------------------------------------------------------------------"
        //         << "----------------------------------O\n\n";
    }
}

void add_in_categories(vector<Category> &C, vector<Application> all_apps ){

    Category temp; temp.category = " Office Softwares ";
    C.emplace_back(temp);
    temp.category = " Development Tool ";
    C.emplace_back(temp);
    temp.category = " Web Browser ";
    C.emplace_back(temp);
    temp.category = " Graphic Design ";
    C.emplace_back(temp);
    temp.category = " System Process ";
    C.emplace_back(temp);
    temp.category = " Others ";
    C.emplace_back(temp);
    for(int i=0;i<int(all_apps.size());i++){
        if(all_apps[i].get_category() == " Office Softwares "){
            if(C[0].get_app().size()==0) {
                C[0].add_app(all_apps[i]);
                continue;
            }
            if(C[0].get_app()[int(C[0].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[0].add_app(all_apps[i]);
        }
        else if(all_apps[i].get_category() == " Development Tool "){
           if(C[1].get_app().size()==0) {
                C[1].add_app(all_apps[i]);
                continue;
            }
            if(C[1].get_app()[int(C[1].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[1].add_app(all_apps[i]);
        }
        else if(all_apps[i].get_category() == " Web Browser "){
            if(C[2].get_app().size()==0) {
                C[2].add_app(all_apps[i]);
                continue;
            }
            if(C[2].get_app()[int(C[2].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[2].add_app(all_apps[i]);
        }
        else if(all_apps[i].get_category() == " Graphic Design "){
            if(C[3].get_app().size()==0) {
                C[3].add_app(all_apps[i]);
                continue;
            }
            if(C[3].get_app()[int(C[3].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[3].add_app(all_apps[i]);
        }
        else if(all_apps[i].get_category() == " System Process "){
            if(C[4].get_app().size()==0) {
                C[4].add_app(all_apps[i]);
                continue;
            }
            if(C[4].get_app()[int(C[4].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[4].add_app(all_apps[i]);
        }
        else{
            if(C[5].get_app().size()==0) {
                C[5].add_app(all_apps[i]);
                continue;
            }
            if(C[5].get_app()[int(C[5].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
            C[5].add_app(all_apps[i]);
        }
    }
}