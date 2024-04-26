#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;
int count = 1;

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

private:
    string name;
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
void print_categories(vector<Category> &category_apps);

int main() {
    vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    vector<Application> all_apps;

    while(1) {
        static DWORD size=0;
        DWORD new_size;
        BOOL result = get_all_processes(all_apps, new_size, processData);
        // vector<Category> C;
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
        // for(int i=0;i<new_size;i++){
        //     if(all_apps[i].get_category() == " Office Softwares "){
        //         C[0].add_app(all_apps[i]);
        //     }
        //     else if(all_apps[i].get_category() == " Development Tool "){
        //         C[1].add_app(all_apps[i]);
        //     }
        //     else if(all_apps[i].get_category() == " Web Browser "){
        //         C[2].add_app(all_apps[i]);
        //     }
        //     else if(all_apps[i].get_category() == " Graphic Design "){
        //         C[3].add_app(all_apps[i]);
        //     }
        //     else if(all_apps[i].get_category() == " System Process "){
        //         C[4].add_app(all_apps[i]);
        //     }
        //     else{
        //         C[5].add_app(all_apps[i]);
        //     }
        // }

        if(size != new_size){
            // for(int i=0;i<new_size;i++){
            //     cout << all_apps[i].get_process_id() << " : " << all_apps[i].get_process_name() << endl;
            // }
            print_categories(C);
            cout << "Size : " << all_apps.size() << endl;
            size = new_size;
        }
        C.clear();
        all_apps.clear();
        Sleep(1000);
    }

    return 0;
}

void print_categories(vector<Category> &category_apps){
    const int COL_WIDTH = 33;
    int shubham = 1;

    for(int i=0;i<category_apps.size();i=i+2){

       cout << " |                    Category :- " << left << setw(47) << category_apps[i].category
            << "|    |                    Category :- " << left << setw(47) << category_apps[i+1].category
            << "\n | " << left << setw(5) << "S no."
            << " | " << left << setw(COL_WIDTH) << "Process"
            << " | " << left << setw(COL_WIDTH) << "App Name"

            <<  " |    |" << left << setw(5) << " S no."
            << " | " << left << setw(30) << "Process"
            << " | " << left << setw(30) << "App Name                       |"
            << "\n";

        for(int j=0;j<category_apps[i].get_app().size();j++){
           cout << "\n | " << left << setw(5) << shubham
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_process_name()
                << " | " << left << setw(COL_WIDTH) << category_apps[i].get_app()[j].get_name()

                <<  " |    |" << left << setw(5) << shubham
                << " | " << left << setw(30) << category_apps[i+1].get_app()[j].get_process_name()
                << " | " << left << setw(30) << category_apps[i+1].get_app()[j].get_name()
                << "\n";
        }
        cout << " ------------------------------------------------------------------------"
                << "----------------------------------------------------------------------------------------------------------\n\n\n";
    }

    // cout << "\n|---------------------------------------------------------------------------------------------|";

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