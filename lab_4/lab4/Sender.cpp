#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

using namespace std;

const int MESSAGE_SIZE = 20;

struct QueueHeader {
    int readIndex;
    int writeIndex;
    int messageCount;
    int maxMessages;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: Sender.exe <filename>" << endl;
        return 1;
    }

    // Преобразование имени файла в wide string
    wstring fileName;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        fileName += (wchar_t)argv[1][i];
    }

    // Открытие синхронизационных объектов
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"QueueMutex");
    HANDLE hFull = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"FullSemaphore");
    HANDLE hEmpty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"EmptySemaphore");
    HANDLE hReady = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"ReadySemaphore");

    if (!hMutex || !hFull || !hEmpty || !hReady) {
        cout << "Error opening synchronization objects" << endl;
        return 1;
    }

    // Открытие файла отображения в память
    HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error opening file" << endl;
        return 1;
    }

    HANDLE hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"MessageQueue");
    if (!hFileMapping) {
        cout << "Error opening file mapping" << endl;
        CloseHandle(hFile);
        return 1;
    }

    // Отображение в память
    QueueHeader* header = (QueueHeader*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(QueueHeader));
    char* messages = (char*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, sizeof(QueueHeader),
        header->maxMessages * MESSAGE_SIZE);

    // Сигнал о готовности к работе
    ReleaseSemaphore(hReady, 1, NULL);
    cout << "Sender is ready! Commands: send <message>, exit" << endl;

    // Основной цикл Sender
    string command;
    while (true) {
        cout << "Enter command: ";
        cin >> command;

        if (command == "exit") {
            break;
        }
        else if (command == "send") {
            string messageText;
            cin >> messageText;

            // Проверка длины сообщения
            if (messageText.length() > MESSAGE_SIZE) {
                cout << "Message too long. Maximum " << MESSAGE_SIZE << " characters" << endl;
                continue;
            }

            WaitForSingleObject(hEmpty, INFINITE); // Ждем свободного места
            WaitForSingleObject(hMutex, INFINITE); // Захватываем мьютекс

            // Запись сообщения в очередь
            int writePos = header->writeIndex;
            char message[MESSAGE_SIZE] = { 0 };

            // Копирование сообщения с заполнением нулями
            strncpy_s(message, MESSAGE_SIZE, messageText.c_str(), MESSAGE_SIZE);
            memcpy(&messages[writePos * MESSAGE_SIZE], message, MESSAGE_SIZE);

            // Обновление индекса записи
            header->writeIndex = (header->writeIndex + 1) % header->maxMessages;
            header->messageCount++;

            cout << "Message sent: " << messageText << endl;

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hFull, 1, NULL); // Сигнализируем о новом сообщении
        }
        else {
            cout << "Unknown command. Use: send <message> or exit" << endl;
        }
    }

    // Освобождение ресурсов
    UnmapViewOfFile(header);
    UnmapViewOfFile(messages);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    CloseHandle(hMutex);
    CloseHandle(hFull);
    CloseHandle(hEmpty);
    CloseHandle(hReady);

    return 0;
}