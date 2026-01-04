#include <iostream>
#include <string>
#include <windows.h>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

class NamedPipeClient {
private:
    HANDLE hPipe;

public:
    NamedPipeClient() {
        hPipe = CreateFileA(
            "\\\\.\\pipe\\cpp_server_pipe",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            throw runtime_error("Cannot connect to server");
        }
    }

    ~NamedPipeClient() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
        }
    }

    bool send_command(const char* cmd) {
        DWORD bytesWritten;
        return WriteFile(hPipe, cmd, strlen(cmd) + 1, &bytesWritten, NULL);
    }

    bool read_employee(int id, Employee& emp) {
        if (!send_command("read")) return false;

        DWORD bytesWritten, bytesRead;
        WriteFile(hPipe, &id, sizeof(id), &bytesWritten, NULL);

        bool found;
        ReadFile(hPipe, &found, sizeof(found), &bytesRead, NULL);

        if (found) {
            ReadFile(hPipe, &emp, sizeof(emp), &bytesRead, NULL);
        }

        return found;
    }

    bool write_employee(int id, const Employee& emp) {
        if (!send_command("write")) return false;

        DWORD bytesWritten, bytesRead;
        WriteFile(hPipe, &id, sizeof(id), &bytesWritten, NULL);

        bool found;
        ReadFile(hPipe, &found, sizeof(found), &bytesRead, NULL);

        if (found) {
            Employee current;
            ReadFile(hPipe, &current, sizeof(current), &bytesRead, NULL);

            WriteFile(hPipe, &emp, sizeof(emp), &bytesWritten, NULL);
        }

        return found;
    }
};

int main() {
    try {
        NamedPipeClient client;
        string command;

        while (true) {
            cout << "Type command (read/write/exit): ";
            cin >> command;

            if (command == "exit") {
                client.send_command("exit");
                break;
            }
            else if (command == "read") {
                int id;
                cout << "Type employee ID: ";
                cin >> id;

                Employee emp;
                if (client.read_employee(id, emp)) {
                    cout << "Employee: " << emp.num << " "
                        << emp.name << " " << emp.hours << endl;
                }
                else {
                    cout << "Employee not found" << endl;
                }

                cout << "Press any key to continue...";
                cin.ignore();
                cin.get();
            }
            else if (command == "write") {
                int id;
                cout << "Type employee ID: ";
                cin >> id;

                Employee emp;
                if (client.read_employee(id, emp)) {
                    cout << "Current: " << emp.num << " "
                        << emp.name << " " << emp.hours << endl;

                    cout << "Enter new data (num name hours): ";
                    cin >> emp.num >> emp.name >> emp.hours;

                    if (client.write_employee(id, emp)) {
                        cout << "Employee updated" << endl;
                    }
                }
                else {
                    cout << "Employee not found" << endl;
                }

                cout << "Press any key to continue...";
                cin.ignore();
                cin.get();
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        system("pause");
        return 1;
    }

    return 0;
}