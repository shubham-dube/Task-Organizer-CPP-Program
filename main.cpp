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
    const SIZE_T get_memory_usage() const { return memory_usage;}

private:
    string name;
    SIZE_T memory_usage;
    SIZE_T cpu_usage;
};

// class Category {
//     public:
//         Category(){}
//         void add_app(const Application &app){
//             apps.push_back(app);
//         }
//         vector<Application> get_app() { return apps; }
//         string category;

//     private:
//         vector<Application> apps;
// };

vector<ProcessData> readProcessDataFromFile(const string& filename);
ProcessData searchProcess(const vector<ProcessData>& processData, const string& process_name);
double get_memory_usage(DWORD process_id);
string get_process_name(DWORD process_id);
BOOL get_all_processes(vector<Application> &all_apps, DWORD &size, vector<ProcessData> processData);
void print_categories(map<string,vector<Application>> &category_apps, int kk);
void add_in_categories(map<string,vector<Application>> &category_apps, vector<Application> all_apps );
BOOL export_processes(string key);
double getCpuUsage(DWORD pId);


//--------------------------------Main Function-----------------------------------------//
int main() {
    vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    vector<Application> all_apps;
    int x;

    while(1){
        cout << "1. See live Processes (Category-wise)" << endl;
        cout << "2. See All Process" << endl;
        cout << "3. Export Process Running Data (Category-wise)" << endl;
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
    ULARGE_INTEGER kernelTimeInt,userTimeInt,totalTimeUsed, totalTimeElapsed;
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
    return (double)memory_values.WorkingSetSize / 1024.0;
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
            << " | " << left << setw(COL_WIDTH) << "Memory Usage"
            << " | " << left << setw(COL_WIDTH) << "App Name" << " |\n";
        cout << " |-------------------------------------------------------------------------------|";
        for(int j=0;j<int(category_apps[i].get_app().size());j++){
           cout << "\n | " << left << setw(5) << kk++
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_process_name()
                << " | " << left << setw(COL_WIDTH) << getCpuUsage(category_apps[i].get_app()[j].get_process_id())
                << " | " << left << setw(COL_WIDTH) << get_memory_usage(category_apps[i].get_app()[j].get_process_id())/1024
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_name() << " |";
        }
        cout << "\n |-------------------------------------------------------------------------------|\n\n";
        // cout << " O------------------------------------------------------------------------"
        //         << "----------------------------------O\n\n";
    }
}

BOOL export_processes(map<string,vector<Application>> &category_apps, string key){
    if(key == "excel"){

    }
    else if(key == "text"){

    }
}

BOOL searchPr(map<string,vector<Application>> &C, string &category, string &key){

}

void add_in_categories(map<string,vector<Application>> &C, vector<Application> all_apps ){

    for(int i=0;i<int(all_apps.size());i++){
        
    }

    // Category temp; temp.category = " Office Softwares ";
    // C.emplace_back(temp);
    // temp.category = " Development Tool ";
    // C.emplace_back(temp);
    // temp.category = " Web Browser ";
    // C.emplace_back(temp);
    // temp.category = " Graphic Design ";
    // C.emplace_back(temp);
    // temp.category = " System Process ";
    // C.emplace_back(temp);
    // temp.category = " Others ";
    // C.emplace_back(temp);
    // for(int i=0;i<int(all_apps.size());i++){
    //     if(all_apps[i].get_category() == " Office Softwares "){
    //         if(C[0].get_app().size()==0) {
    //             C[0].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[0].get_app()[int(C[0].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[0].add_app(all_apps[i]);
    //     }
    //     else if(all_apps[i].get_category() == " Development Tool "){
    //        if(C[1].get_app().size()==0) {
    //             C[1].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[1].get_app()[int(C[1].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[1].add_app(all_apps[i]);
    //     }
    //     else if(all_apps[i].get_category() == " Web Browser "){
    //         if(C[2].get_app().size()==0) {
    //             C[2].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[2].get_app()[int(C[2].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[2].add_app(all_apps[i]);
    //     }
    //     else if(all_apps[i].get_category() == " Graphic Design "){
    //         if(C[3].get_app().size()==0) {
    //             C[3].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[3].get_app()[int(C[3].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[3].add_app(all_apps[i]);
    //     }
    //     else if(all_apps[i].get_category() == " System Process "){
    //         if(C[4].get_app().size()==0) {
    //             C[4].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[4].get_app()[int(C[4].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[4].add_app(all_apps[i]);
    //     }
    //     else{
    //         if(C[5].get_app().size()==0) {
    //             C[5].add_app(all_apps[i]);
    //             continue;
    //         }
    //         if(C[5].get_app()[int(C[5].get_app().size())-1].get_process_name() != all_apps[i].get_process_name())
    //         C[5].add_app(all_apps[i]);
    //     }
    // }
}