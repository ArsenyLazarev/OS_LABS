#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <iomanip> 
#include <windows.h>
struct employee
{
	int num; // идентификационный номер сотрудника
	char name[10]; // имя сотрудника
	double hours; // количество отработанных часов
};
int main(int argc,char* argv[])
{
	if (argc != 4) {//1 - имя файла, 2 - количество звписей(?)
		return 1;
	}

	std::string inputFile = argv[1];

	std::string outputFile = argv[2];

	double hourlyRate = atof(argv[3]);

	std::ifstream inFile(inputFile, std::ios::binary);
	if (!inFile) {
		std::cout << "Can`t find binary file";
		return 1;
	}

	std::ofstream outFile(outputFile);
	if (!outFile) {
		std::cout << "Can`t make output file";
		return 1;
	}

	outFile << "Report for file:" + inputFile << std::endl;

	outFile << std::setw(5) << "ID" << std::setw(12) << "Name"
		<< std::setw(10) << "Hours" << std::setw(12) << "Salary" << std::endl;
	outFile << "----------------------------------------" << std::endl;

	employee emp;

	while (inFile.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
		double salary = emp.hours * hourlyRate; 
		outFile << std::setw(5) << emp.num               
			<< std::setw(12) << emp.name            
			<< std::setw(10) << std::fixed << std::setprecision(2) << emp.hours  
			<< std::setw(12) << std::fixed << std::setprecision(2) << salary     
			<< std::endl;
	}

	inFile.close();  

	outFile.close();

	std::cout << "Report generated: " << outputFile << std::endl;

	return 0;
}