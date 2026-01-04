#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

struct employee {
    int num;
    char name[10];
    double hours;
};

void printBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::cout << "\nContents of " << filename << ":" << std::endl;
    std::cout << "ID\tName\tHours" << std::endl;
    std::cout << "----------------------" << std::endl;

    employee emp;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        std::cout << emp.num << "\t" << emp.name << "\t" << emp.hours << std::endl;
    }
    file.close();
}

void printTextFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::cout << "\nContents of " << filename << ":" << std::endl;
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }
    file.close();
}

int main() {
    
    std::string binaryFile;
    int recordCount;

    std::cout << "Enter binary file name: ";
    std::cin >> binaryFile;
    std::cout << "Enter number of records: ";
    std::cin >> recordCount;

   
    std::string creatorCmd = "Creator.exe" + binaryFile + " " + std::to_string(recordCount);

    STARTUPINFOA siCreator;  
    PROCESS_INFORMATION piCreator;
    ZeroMemory(&siCreator, sizeof(siCreator));
    siCreator.cb = sizeof(siCreator);
    ZeroMemory(&piCreator, sizeof(piCreator));

    
    if (!CreateProcessA(
        NULL,                  
        const_cast<LPSTR>(creatorCmd.c_str()),  
        NULL,                   
        NULL,                   
        FALSE,                  
        0,                   
        NULL,                   
        NULL,                
        &siCreator,         
        &piCreator             
    )) {
        std::cerr << "Error starting Creator: " << GetLastError() << std::endl;
        return 1;
    }

    WaitForSingleObject(piCreator.hProcess, INFINITE);
    CloseHandle(piCreator.hProcess);
    CloseHandle(piCreator.hThread);

    printBinaryFile(binaryFile);

    std::string reportFile;
    double hourlyRate;

    std::cout << "\nEnter report file name: ";
    std::cin >> reportFile;
    std::cout << "Enter hourly rate: ";
    std::cin >> hourlyRate;

    std::string reporterCmd = "Reporter.exe " + binaryFile + " " + reportFile + " " + std::to_string(hourlyRate);

    STARTUPINFOA siReporter; 
    PROCESS_INFORMATION piReporter;
    ZeroMemory(&siReporter, sizeof(siReporter));
    siReporter.cb = sizeof(siReporter);
    ZeroMemory(&piReporter, sizeof(piReporter));

    if (!CreateProcessA( 
        NULL,
        const_cast<LPSTR>(reporterCmd.c_str()),
        NULL, NULL, FALSE, 0, NULL, NULL, &siReporter, &piReporter
    )) {
        std::cerr << "Error starting Reporter: " << GetLastError() << std::endl;
        return 1;
    }

    WaitForSingleObject(piReporter.hProcess, INFINITE);
    CloseHandle(piReporter.hProcess);
    CloseHandle(piReporter.hThread);

    printTextFile(reportFile);

    std::cout << "\nProgram completed successfully!" << std::endl;
    return 0;
}