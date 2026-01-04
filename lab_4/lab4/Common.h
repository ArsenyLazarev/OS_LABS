#pragma once
// Common.h
#ifndef COMMON_H
#define COMMON_H

#include <windows.h>

const int MESSAGE_SIZE = 20;

struct QueueHeader {
    int readIndex;
    int writeIndex;
    int messageCount;
    int maxMessages;
};

// Имена объектов синхронизации
const wchar_t* MUTEX_NAME = L"QueueMutex";
const wchar_t* FULL_SEMAPHORE = L"FullSemaphore";
const wchar_t* EMPTY_SEMAPHORE = L"EmptySemaphore";
const wchar_t* READY_SEMAPHORE = L"ReadySemaphore";
const wchar_t* FILE_MAPPING_NAME = L"MessageQueue";

#endif