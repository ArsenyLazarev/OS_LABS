#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <windows.h>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

class ThreadSafeEmployees {
private:
    vector<Employee> employees;
    mutable shared_mutex global_mutex;
    vector<unique_ptr<shared_mutex>> record_mutexes;

public:
    ThreadSafeEmployees() = default;

    ThreadSafeEmployees(int size) {
        record_mutexes.reserve(size);
        for (int i = 0; i < size; ++i) {
            record_mutexes.emplace_back(make_unique<shared_mutex>());
        }
    }

    ThreadSafeEmployees(const ThreadSafeEmployees&) = delete;
    ThreadSafeEmployees& operator=(const ThreadSafeEmployees&) = delete;

    ThreadSafeEmployees(ThreadSafeEmployees&&) = default;
    ThreadSafeEmployees& operator=(ThreadSafeEmployees&&) = default;

    bool read_employee(int id, Employee& emp) {
        shared_lock<shared_mutex> global_lock(global_mutex);

        for (size_t i = 0; i < employees.size(); ++i) {
            if (employees[i].num == id) {
                shared_lock<shared_mutex> record_lock(*record_mutexes[i]);
                emp = employees[i];
                return true;
            }
        }
        return false;
    }

    bool write_employee(int id, const Employee& new_emp) {
        unique_lock<shared_mutex> global_lock(global_mutex);

        for (size_t i = 0; i < employees.size(); ++i) {
            if (employees[i].num == id) {
                unique_lock<shared_mutex> record_lock(*record_mutexes[i]);
                employees[i] = new_emp;
                return true;
            }
        }
        return false;
    }

    void add_employee(const Employee& emp) {
        unique_lock<shared_mutex> lock(global_mutex);
        employees.push_back(emp);
        record_mutexes.push_back(make_unique<shared_mutex>());
    }

    vector<Employee> get_all() const {
        shared_lock<shared_mutex> lock(global_mutex);
        return employees;
    }

    size_t size() const {
        return employees.size();
    }
};

unique_ptr<ThreadSafeEmployees> employees_db;
atomic<bool> server_running{ true };

void client_handler(HANDLE hPipe) {
    char command[20];
    DWORD bytesRead, bytesWritten;

    while (server_running) {
        if (!ReadFile(hPipe, command, sizeof(command), &bytesRead, NULL)) {
            break;
        }

        if (strcmp(command, "read") == 0) {
            int emp_id;
            ReadFile(hPipe, &emp_id, sizeof(emp_id), &bytesRead, NULL);

            Employee emp;
            bool found = employees_db->read_employee(emp_id, emp);

            WriteFile(hPipe, &found, sizeof(found), &bytesWritten, NULL);
            if (found) {
                WriteFile(hPipe, &emp, sizeof(emp), &bytesWritten, NULL);
            }
        }
        else if (strcmp(command, "write") == 0) {
            int emp_id;
            ReadFile(hPipe, &emp_id, sizeof(emp_id), &bytesRead, NULL);

            Employee current_emp;
            bool found = employees_db->read_employee(emp_id, current_emp);

            WriteFile(hPipe, &found, sizeof(found), &bytesWritten, NULL);
            if (found) {
                WriteFile(hPipe, &current_emp, sizeof(current_emp), &bytesWritten, NULL);

                Employee new_emp;
                ReadFile(hPipe, &new_emp, sizeof(new_emp), &bytesRead, NULL);

                employees_db->write_employee(emp_id, new_emp);
            }
        }
        else if (strcmp(command, "exit") == 0) {
            break;
        }
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

int main() {
    int emp_count;
    cout << "Type number of employees: ";
    cin >> emp_count;

    employees_db = make_unique<ThreadSafeEmployees>();

    for (int i = 0; i < emp_count; ++i) {
        Employee emp;
        cout << i + 1 << ") ";
        cin >> emp.num >> emp.name >> emp.hours;
        employees_db->add_employee(emp);
    }

    cout << "\nCreated file contents:" << endl;
    auto all_emps = employees_db->get_all();
    for (const auto& emp : all_emps) {
        cout << emp.num << " " << emp.name << " " << emp.hours << endl;
    }

    int clients_num;
    cout << "\nType number of clients: ";
    cin >> clients_num;

    vector<thread> client_threads;
    vector<HANDLE> pipes;

    for (int i = 0; i < clients_num; ++i) {
        HANDLE hPipe = CreateNamedPipe(
            L"\\\\.\\pipe\\cpp_server_pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            clients_num, 0, 0, INFINITE, NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            cerr << "Pipe creation failed" << endl;
            continue;
        }

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        wchar_t cmdLine[] = L"Client.exe";

        if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }

        if (ConnectNamedPipe(hPipe, NULL)) {
            cout << "Client " << i + 1 << " connected." << endl;
            client_threads.emplace_back(client_handler, hPipe);
            pipes.push_back(hPipe);
        }
    }

    for (auto& t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    for (auto hPipe : pipes) {
        CloseHandle(hPipe);
    }

    cout << "\nModified file contents:" << endl;
    all_emps = employees_db->get_all();
    for (const auto& emp : all_emps) {
        cout << emp.num << " " << emp.name << " " << emp.hours << endl;
    }

    cout << "\nType 'exit' to finish server: ";
    string cmd;
    cin >> cmd;

    server_running = false;

    return 0;
}