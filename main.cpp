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
void searchProcess(const std::vector<ProcessData>& processData, const std::string& process_name);

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

    Application app[returned_bytes/sizeof(DWORD)];

    if(result){
        std::cout << "No. Of Processes: " << returned_bytes/sizeof(DWORD);
        for(int i=0;i<returned_bytes/sizeof(DWORD);i++){
            
        }
    }

    return 0;
}

std::string get_process_name(DWORD process_id){
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if(h_process == NULL){
        
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
            std::cout << process_name << " " << app_name << " " << category << std::endl;
            processData.push_back({process_name,app_name, category});
        }
        file.close();
    }
    return processData;
}

void searchProcess(const std::vector<ProcessData>& processData, const std::string& process_name) {
    for (const auto& process : processData) {
        if (process.process_name == process_name) {
            std::cout << process.process_name << ": " << process.app_name << ", " << process.category << std::endl;
            return;
        }
    }
    std::cout << "Process not found." << std::endl;
}