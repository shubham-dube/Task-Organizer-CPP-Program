#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
static int i = 0;
using namespace std;

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
        cout << "Application Created -> " << ++i << endl;
    }

    const string get_name() const { return name; }

private:
    string name;
};

string get_process_name(DWORD process_id);
BOOL get_all_processes(vector<Application> &all_apps, DWORD &size);

int main() {
    vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    vector<DWORD> process_info;
    DWORD process_id[500];
    DWORD returned_bytes;

    BOOL result = EnumProcesses(process_id, sizeof(process_id), &returned_bytes);

    DWORD real_size = returned_bytes/sizeof(DWORD);
    vector<Application> app;

    if(result){
        cout << "No. Of Processes: " << real_size << endl;
        for(int i=0;i<real_size;i++){
            string process_name = get_process_name(process_id[i]);
            ProcessData P = searchProcess(processData, process_name);
            Application A;
            A.new_app(process_id[i], process_name, P.category, P.app_name);
            app.push_back(A);
        }
    }

    while(1) {
        system("cls");
        DWORD temp = 0;
        BOOL resTemp = EnumProcesses(process_id, sizeof(process_id), &temp);
        if(temp/sizeof(DWORD) != real_size){
            for(int i=0;i<temp/sizeof(DWORD);i++){
                cout << process_id[i] << " : " << app[i].get_process_name() << endl;
            }
            real_size = temp/sizeof(DWORD);
        }
        cout << "Size: "<<temp/sizeof(DWORD) << "  "<< sizeof(process_id) << endl;
        Sleep(10000);
    }

    return 0;
}

BOOL get_all_processes(vector<Application> &all_apps, DWORD &size){
    
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