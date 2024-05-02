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
using namespace std;

class ProcessData {
    public:
    string process_name;
    string app_name;
    string category;
};

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
    const int get_memory_usage() const { return memory_usage;}
    int memory_usage = 0;
    double cpu_usage = 0.0;

private:
    string name;
};

vector<ProcessData> readProcessDataFromFile(const string& filename);
ProcessData searchProcess(const vector<ProcessData>& processData, const string& process_name);
double get_memory_usage(DWORD process_id);
string get_process_name(DWORD process_id);
BOOL get_all_processes(vector<Application> &all_apps, DWORD &size, vector<ProcessData> processData);
void print_categories(map<string,vector<Application>> &category_apps, int kk);
void add_in_categories(map<string,vector<Application>> &category_apps, vector<Application> &all_apps );
BOOL export_processes(map<string,vector<Application>> &category_apps, int kk, ofstream &outfile);
double getCpuUsage(DWORD pId);


//--------------------------------Main Function-----------------------------------------//
int main() {
    vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    vector<Application> all_apps;
    int x;

    while(1){
        cout << "1. See live Processes (Category-wise)\n"
                  << "2. Export Process Running Data (Category-wise)\n"
                  << "3. Enter -1 to Exit\n";
        cin >> x;

        switch(x){
            case 1:
                while(1) {
                    static DWORD size=0;
                    DWORD new_size;
                    BOOL result = get_all_processes(all_apps, new_size, processData);
                    if(!result){
                        cout << "Processes Retrieval Failed" << endl;
                    }
                    
                    if(size != new_size && result){
                        system("cls");
                        map<string,vector<Application>> C;
                        add_in_categories(C,all_apps);
                        print_categories(C,1);
                        size = new_size;
                        C.clear();
                    }
                    all_apps.clear();
                    Sleep(500);

                    if (_kbhit()) {
                        char key = _getch();
                        if (key == 'q' || key == 'Q') {
                            break;
                        }
                    }
                }
                break;

            case 2: {
                DWORD new_size;
                BOOL result = get_all_processes(all_apps, new_size, processData);
                if(!result){
                    cout << "Processes Retrieval Failed" << endl;
                }
                map<string,vector<Application>> Cm;
                add_in_categories(Cm,all_apps);
                
                ofstream outfile("output.txt"); 
                BOOL isWritten = export_processes(Cm, 1, outfile); 
                outfile.close();
                all_apps.clear();
                if(isWritten) cout << "Processes exported successfully" << endl;
                else cout << "Processes export failed" << endl;
                break;
            }

            case -1:
                cout << "Exiting...\n";
                return 0;

            default:
                cout << "Invalid option. Try again.\n";
                break;
        }
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

double getCpuUsage(DWORD pId){
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,pId);

    FILETIME creationTime,exitTime,kernelTime,userTime;
    ULARGE_INTEGER kernelTimeInt,userTimeInt,totalTimeUsed;
    BOOL result = GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime);
    if(!result) return -1;

    kernelTimeInt.LowPart = kernelTime.dwLowDateTime;
    kernelTimeInt.HighPart = kernelTime.dwHighDateTime;
    userTimeInt.LowPart = userTime.dwLowDateTime;
    userTimeInt.HighPart = userTime.dwHighDateTime;

    FILETIME idleTimeSys,kernelTimeSys,userTimeSys;
    ULARGE_INTEGER idleTimeSysInt,kernelTimeSysInt,userTimeSysInt,totalTimeUsedSys;
    BOOL result1 = GetSystemTimes(&idleTimeSys, &kernelTimeSys, &userTimeSys);
    if(!result1) return -1;
    
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
    return cpuUsage*100.0;
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

double get_memory_usage(DWORD process_id){
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if(h_process == NULL){
        return 0.0;
    }
    PROCESS_MEMORY_COUNTERS memory_values;
    BOOL result = GetProcessMemoryInfo(h_process, &memory_values, sizeof(memory_values));
    if(result == FALSE){
        CloseHandle(h_process);
        return 0.0;
    }
    CloseHandle(h_process);
    return (int)memory_values.WorkingSetSize / 1024;
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

void print_categories(map<string,vector<Application>> &category_apps, int kk){
    const int COL_WIDTH = 33;

    for(auto i=category_apps.begin();i!=category_apps.end();i++){
        vector<Application> apps = i->second;
        cout << "\r |-------------------------------------------------------------------------------------------------------------------------------------------------------|\n";
       cout << " |                    Category :- " << left << setw(47) << i->first
            << "|"
            << "\n | " << left << setw(5) << "S no."
            << " | " << left << setw(COL_WIDTH) << "Process"
            << " | " << left << setw(COL_WIDTH) << "App Name"
            << " | " << left << setw(COL_WIDTH) << "CPU Usage"
            << " | " << left << setw(COL_WIDTH) << "Memory Usage" << " |\n";
        cout << "\r |-------------------------------------------------------------------------------------------------------------------------------------------------------|\n";
        for(int j=0;j<int(apps.size());j++){
           cout << "\n | " << left << setw(5) << kk++
                << " | " << left << setw(COL_WIDTH) << apps[j].get_process_name()
                << " | " << left << setw(COL_WIDTH) << apps[j].get_name()
                << " | " << left << setw(COL_WIDTH) << apps[j].cpu_usage
                << " | " << left << setw(COL_WIDTH) << apps[j].memory_usage << " |";
        }
                cout << "\r |-------------------------------------------------------------------------------------------------------------------------------------------------------|\n";
    }
}

BOOL export_processes(map<string,vector<Application>> &category_apps, int kk, ofstream &outfile){
    const int COL_WIDTH = 33;

    for(auto i=category_apps.begin();i!=category_apps.end();i++){
        vector<Application> apps = i->second;
        outfile << "\r |-------------------------------------------------------------------------------|\n";
        outfile << " |                    Category :- " << left << setw(47) << i->first
                << "|"
                << "\n | " << left << setw(5) << "S no."
                << " | " << left << setw(COL_WIDTH) << "Process"
                << " | " << left << setw(COL_WIDTH) << "App Name"
                << " | " << left << setw(COL_WIDTH) << "CPU Usage"
                << " | " << left << setw(COL_WIDTH) << "Memory Usage" << " |\n";
        outfile << " |-------------------------------------------------------------------------------|";
        for(int j=0;j<int(apps.size());j++){
            outfile << "\n | " << left << setw(5) << kk++
                    << " | " << left << setw(COL_WIDTH) << apps[j].get_process_name()
                    << " | " << left << setw(COL_WIDTH) << apps[j].get_name()
                    << " | " << left << setw(COL_WIDTH) << apps[j].cpu_usage
                    << " | " << left << setw(COL_WIDTH) << apps[j].memory_usage << " |";
        }
        outfile << "\n |-------------------------------------------------------------------------------|\n\n";
    }
    return true;
}

BOOL searchPr(vector<Application> &A, string key){
    for(int i=0;i<int(A.size());i++){
        if(A[i].get_process_name() == key){
            A[i].cpu_usage += getCpuUsage(A[i].get_process_id());
            A[i].memory_usage += get_memory_usage(A[i].get_process_id());
            return true;
        }
    }
    return false;
}

void add_in_categories(map<string,vector<Application>> &C, vector<Application> &all_apps ){

    for(int i=0;i<int(all_apps.size());i++){
        if(all_apps[i].get_category() == " Office Softwares "){
            if(searchPr(C[" Office Softwares "],all_apps[i].get_process_name())) continue;
            C[" Office Softwares "].push_back(all_apps[i]);
            C[" Office Softwares "].back().cpu_usage += getCpuUsage(C[" Office Softwares "].back().get_process_id());
            C[" Office Softwares "].back().memory_usage += (int)get_memory_usage(C[" Office Softwares "].back().get_process_id());
        }
        else if(all_apps[i].get_category() == " Development Tool "){
            if(searchPr(C[" Development Tool "],all_apps[i].get_process_name())) continue;
            C[" Development Tool "].push_back(all_apps[i]);            
            C[" Development Tool "].back().cpu_usage += getCpuUsage(C[" Development Tool "].back().get_process_id());
            C[" Development Tool "].back().memory_usage += (int)get_memory_usage(C[" Development Tool "].back().get_process_id());
        }
        else if(all_apps[i].get_category() == " Web Browser "){
            if(searchPr(C[" Web Browser "],all_apps[i].get_process_name())) continue;
            C[" Web Browser "].push_back(all_apps[i]);
            C[" Web Browser "].back().cpu_usage += getCpuUsage(C[" Web Browser "].back().get_process_id());
            C[" Web Browser "].back().memory_usage += (int)get_memory_usage(C[" Web Browser "].back().get_process_id());
        }
        else if(all_apps[i].get_category() == " Graphic Softwares "){
            if(searchPr(C[" Graphic Softwares "],all_apps[i].get_process_name())) continue;
            C[" Graphic Softwares "].push_back(all_apps[i]);
        }

        else if(all_apps[i].get_category() == " System Process "){
            if(searchPr(C[" System Process "],all_apps[i].get_process_name())) continue;
            C[" System Process "].push_back(all_apps[i]);
            C[" System Process "].back().cpu_usage += getCpuUsage(C[" System Process "].back().get_process_id());
            C[" System Process "].back().memory_usage += (int)get_memory_usage(C[" System Process "].back().get_process_id());
        }

        else{
            if(searchPr(C[" Others "],all_apps[i].get_process_name())) continue;
            C[" Others "].push_back(all_apps[i]);
            C[" Others "].back().cpu_usage += getCpuUsage(C[" Others "].back().get_process_id());
            C[" Others "].back().memory_usage += (int)get_memory_usage(C[" Others "].back().get_process_id());
        }
    }
}