#include <iostream>
#include <fstream> 
#include <string>
#include <windows.h>
struct employee
{
    int num; // идентификационный номер сотрудника
    char name[10]; // имя сотрудника
    double hours; // количество отработанных часов
};

int main(int argc, char* argv[])// argc - количество аргументов, argv - массив аргументов
{
    if (argc != 3) {//1 - имя файла, 2 - количество звписей(?)
        return 1;
   }
    std::string filename = argv[1];
    int recordCount = std::stoi(argv[2]);

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cout << "File hadn`t been created";
        return 1;
    }
    for (int i = 0; i < recordCount; i++) {
        employee emp;
        std::cout << "Record: " << (i + 1) << std::endl;
        std::cout << "ID: ";
        std::cin >> emp.num;
        std::cout << "name (<10): ";
        std::cin >> emp.name;
        std::cout << "hours: ";
        std::cin >> emp.hours;


        outFile.write(reinterpret_cast<char*>(&emp), sizeof(employee));
    }
    outFile.close();
    
    return 0;

}
