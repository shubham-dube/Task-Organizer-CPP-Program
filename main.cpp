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

class ProcessData {
    public:
    std::string process_name;
    std::string app_name;
    std::string category;
};
std::vector<ProcessData> readProcessDataFromFile(const std::string& filename);
ProcessData searchProcess(const std::vector<ProcessData>& processData, const std::string& process_name);

class Process {
public:
    Process(){}
    void new_process(const DWORD &process_id, const std::string &process_name, const std::string& category){
        this->process_id = process_id;
        this->process_name = process_name;
        this->category = category;
    }

    const DWORD get_process_id() const { return process_id; }
    const std::string get_process_name() const { return process_name; }
    const std::string get_category() const { return category; }

private:
    DWORD process_id;
    std::string process_name;
    std::string category;
};

class Application: public Process {
public:
    Application(){}
    void new_app(const DWORD &process_id, const std::string &process_name, 
    const std::string& category, const std::string &name){
        new_process(process_id, process_name, category);
        this->name = name;
        std::cout << "Application Created -> " << ++i << std::endl;
    }

    const std::string get_name() const { return name; }

private:
    std::string name;
};

std::string get_process_name(DWORD process_id);

int main() {
    std::vector<ProcessData> processData = readProcessDataFromFile("app_database.txt");
    
    DWORD process_id[500];
    DWORD returned_bytes;

    BOOL result = EnumProcesses(process_id, sizeof(process_id), &returned_bytes);

    DWORD real_size = returned_bytes/sizeof(DWORD);
    std::vector<Application> app;

    if(result){
        std::cout << "No. Of Processes: " << real_size << std::endl;
        for(int i=0;i<real_size;i++){

            std::string process_name = get_process_name(process_id[i]);
            ProcessData P = searchProcess(processData, process_name);
            Application A;
            A.new_app(process_id[i], process_name, P.category, P.app_name);
            app.push_back(A);
        }
    }

    for(int i=0;i<real_size;i++){
        std::cout << "Process ID: " << app[i].get_process_id() << " ";
        std::cout << "Process Name: " << app[i].get_process_name() << " ";

        std::cout << "Category: " << app[i].get_category() << " ";
        std::cout << "App Name: " << app[i].get_name() << " ";
        std::cout << std::endl;
    }
    return 0;
}

    std::string get_process_name(DWORD process_id){
        HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

        if(h_process == NULL){
            return "Access Denied";
        }

        TCHAR process_name[1020];

        if(GetModuleBaseName(h_process, NULL, process_name, 1020)){
            CloseHandle(h_process);
            return std::string(process_name);
        }
        else {
            CloseHandle(h_process);
            return "Unknown";
        }
    }

    std::vector<ProcessData> readProcessDataFromFile(const std::string& filename) {
        std::ifstream file(filename);
        std::vector<ProcessData> processData;
        if (file.is_open()) {
            std::string line;
            while (getline(file, line)) {
                std::istringstream iss(line);
                std::string process_name, app_name,category;
                std::getline(iss, process_name, ':');
                std::getline(iss, app_name, ',');
                std::getline(iss, category, '\n');
                processData.push_back({process_name,app_name, category});
            }
            file.close();
        }
        return processData;
    }

    ProcessData searchProcess(const std::vector<ProcessData>& processData, const std::string& process_name) {
        for (const auto& process : processData) {
            if (process.process_name == process_name) {
                return process;
            }
        }
        return {"", "", ""};
    }