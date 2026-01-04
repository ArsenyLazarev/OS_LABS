// Receiver.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Common.h"

using namespace std;

int main() {
    HANDLE hMutex, hFull, hEmpty, hReady;
    HANDLE hFileMapping;
    QueueHeader* header = nullptr;
    char* messages = nullptr;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    wstring fileName;
    int maxMessages, senderCount;

    try {
        // Ввод имени файла и количества сообщений
        wcout << L"Enter binary file name: ";
        wcin >> fileName;
        cout << "Enter maximum number of messages: ";
        cin >> maxMessages;

        if (maxMessages < 1) {
            cout << "Incorrect number of messages" << endl;
            return 1;
        }

        // Создание файла
        hFile = CreateFile(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            cout << "Error creating file. Error: " << GetLastError() << endl;
            return 1;
        }

        // Установка размера файла
        DWORD fileSize = sizeof(QueueHeader) + maxMessages * MESSAGE_SIZE;
        SetFilePointer(hFile, fileSize, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);

        // Создание file mapping
        hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, fileSize, FILE_MAPPING_NAME);
        if (!hFileMapping) {
            cout << "Error creating file mapping. Error: " << GetLastError() << endl;
            throw runtime_error("File mapping error");
        }

        // Отображение в память
        header = (QueueHeader*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(QueueHeader));
        messages = (char*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, sizeof(QueueHeader),
            maxMessages * MESSAGE_SIZE);

        if (!header || !messages) {
            cout << "Error mapping view of file. Error: " << GetLastError() << endl;
            throw runtime_error("Mapping error");
        }

        // Инициализация очереди
        header->readIndex = 0;
        header->writeIndex = 0;
        header->messageCount = 0;
        header->maxMessages = maxMessages;

        // Создание синхронизационных объектов
        hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
        hFull = CreateSemaphore(NULL, 0, maxMessages, FULL_SEMAPHORE);
        hEmpty = CreateSemaphore(NULL, maxMessages, maxMessages, EMPTY_SEMAPHORE);
        hReady = CreateSemaphore(NULL, 0, 100, READY_SEMAPHORE); // До 100 отправителей

        if (!hMutex || !hFull || !hEmpty || !hReady) {
            cout << "Error creating synchronization objects. Error: " << GetLastError() << endl;
            throw runtime_error("Sync objects error");
        }

        // Ввод количества процессов Sender
        cout << "Enter number of Sender processes: ";
        cin >> senderCount;

        if (senderCount < 1 || senderCount > 100) {
            cout << "Incorrect number of senders (1-100)" << endl;
            return 1;
        }

        // Запуск процессов Sender
        vector<PROCESS_INFORMATION> pi(senderCount);
        vector<HANDLE> senderProcesses(senderCount);
        bool createProcessError = false;

        for (int i = 0; i < senderCount; i++) {
            STARTUPINFO si = { sizeof(STARTUPINFO) };
            wstring commandLine = L"Sender.exe " + fileName;

            if (!CreateProcess(NULL, &commandLine[0], NULL, NULL, FALSE,
                CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi[i])) {
                cout << "Error creating Sender process " << i << ". Error: " << GetLastError() << endl;
                createProcessError = true;
                break;
            }
            senderProcesses[i] = pi[i].hProcess;
            cout << "Started Sender process " << i + 1 << endl;
        }

        if (createProcessError) {
            throw runtime_error("Process creation error");
        }

        // Ожидание готовности всех Sender процессов
        cout << "Waiting for all Senders to be ready..." << endl;
        for (int i = 0; i < senderCount; i++) {
            DWORD result = WaitForSingleObject(hReady, 10000); // 10 секунд таймаут
            if (result == WAIT_TIMEOUT) {
                cout << "Timeout waiting for Sender " << i + 1 << endl;
            }
            else {
                cout << "Sender " << i + 1 << " is ready!" << endl;
            }
        }

        // Основной цикл Receiver
        string command;
        while (true) {
            cout << "Enter command (read/exit): ";
            cin >> command;

            if (command == "exit") {
                break;
            }
            else if (command == "read") {
                DWORD result = WaitForSingleObject(hFull, 5000); // 5 секунд таймаут
                if (result == WAIT_TIMEOUT) {
                    cout << "No messages available (timeout)" << endl;
                    continue;
                }

                WaitForSingleObject(hMutex, INFINITE);

                // Чтение сообщения из очереди
                char message[MESSAGE_SIZE + 1] = { 0 };
                int readPos = header->readIndex;

                memcpy(message, &messages[readPos * MESSAGE_SIZE], MESSAGE_SIZE);
                message[MESSAGE_SIZE] = '\0';

                // Обновление индекса чтения
                header->readIndex = (header->readIndex + 1) % maxMessages;
                header->messageCount--;

                cout << "Received message: '" << message << "'" << endl;

                ReleaseMutex(hMutex);
                ReleaseSemaphore(hEmpty, 1, NULL);
            }
            else {
                cout << "Unknown command. Use 'read' or 'exit'" << endl;
            }
        }

        // Завершение работы
        cout << "Shutting down..." << endl;

        // Завершение процессов Sender
        for (int i = 0; i < senderCount; i++) {
            if (pi[i].hProcess) {
                TerminateProcess(pi[i].hProcess, 0);
                WaitForSingleObject(pi[i].hProcess, 1000);
                CloseHandle(pi[i].hThread);
                CloseHandle(pi[i].hProcess);
            }
        }

    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    // Освобождение ресурсов
    if (header) UnmapViewOfFile(header);
    if (messages) UnmapViewOfFile(messages);
    if (hFileMapping) CloseHandle(hFileMapping);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    if (hMutex) CloseHandle(hMutex);
    if (hFull) CloseHandle(hFull);
    if (hEmpty) CloseHandle(hEmpty);
    if (hReady) CloseHandle(hReady);

    cout << "Receiver finished" << endl;
    return 0;
}