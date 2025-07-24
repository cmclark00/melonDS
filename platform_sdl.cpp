/*
    melonDS Platform Implementation for SDL Frontend
    Copyright (C) 2024 melonDS team
    
    This file is part of melonDS.
    
    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.
    
    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <functional>

#include "Platform.h"

namespace melonDS::Platform
{

void SignalStop(StopReason reason, void* userdata)
{
    // For our simple frontend, we can just exit
    exit(0);
}

FileHandle* OpenFile(const std::string& path, FileMode mode)
{
    std::string modeStr;
    
    // Convert FileMode to standard C mode string
    if ((mode & FileMode::Read) && (mode & FileMode::Write)) {
        if (mode & FileMode::Append) {
            modeStr = "a+";
        } else if (mode & FileMode::Preserve) {
            modeStr = "r+";
        } else {
            modeStr = "w+";
        }
    } else if (mode & FileMode::Write) {
        if (mode & FileMode::Append) {
            modeStr = "a";
        } else if (mode & FileMode::Preserve) {
            modeStr = "r+";
        } else {
            modeStr = "w";
        }
    } else {
        modeStr = "r";
    }
    
    if (!(mode & FileMode::Text)) {
        modeStr += "b";
    }
    
    FILE* file = fopen(path.c_str(), modeStr.c_str());
    return reinterpret_cast<FileHandle*>(file);
}

std::string GetLocalFilePath(const std::string& filename)
{
    // For simplicity, just use current directory
    return filename;
}

FileHandle* OpenLocalFile(const std::string& path, FileMode mode)
{
    return OpenFile(GetLocalFilePath(path), mode);
}

bool FileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool LocalFileExists(const std::string& name)
{
    return FileExists(GetLocalFilePath(name));
}

bool CheckFileWritable(const std::string& filepath)
{
    FILE* file = fopen(filepath.c_str(), "a");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

bool CheckLocalFileWritable(const std::string& filepath)
{
    return CheckFileWritable(GetLocalFilePath(filepath));
}

bool CloseFile(FileHandle* file)
{
    return fclose(reinterpret_cast<FILE*>(file)) == 0;
}

bool IsEndOfFile(FileHandle* file)
{
    return feof(reinterpret_cast<FILE*>(file)) != 0;
}

bool FileReadLine(char* str, int count, FileHandle* file)
{
    return fgets(str, count, reinterpret_cast<FILE*>(file)) != nullptr;
}

bool FileSeek(FileHandle* file, s64 offset, FileSeekOrigin origin)
{
    int whence;
    switch (origin) {
        case FileSeekOrigin::Start: whence = SEEK_SET; break;
        case FileSeekOrigin::Current: whence = SEEK_CUR; break;
        case FileSeekOrigin::End: whence = SEEK_END; break;
    }
    return fseek(reinterpret_cast<FILE*>(file), offset, whence) == 0;
}

void FileRewind(FileHandle* file)
{
    rewind(reinterpret_cast<FILE*>(file));
}

u64 FileRead(void* data, u64 size, u64 count, FileHandle* file)
{
    return fread(data, size, count, reinterpret_cast<FILE*>(file));
}

bool FileFlush(FileHandle* file)
{
    return fflush(reinterpret_cast<FILE*>(file)) == 0;
}

u64 FileWrite(const void* data, u64 size, u64 count, FileHandle* file)
{
    return fwrite(data, size, count, reinterpret_cast<FILE*>(file));
}

u64 FileWriteFormatted(FileHandle* file, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(reinterpret_cast<FILE*>(file), fmt, args);
    va_end(args);
    return ret >= 0 ? ret : 0;
}

u64 FileLength(FileHandle* file)
{
    FILE* f = reinterpret_cast<FILE*>(file);
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, pos, SEEK_SET);
    return len >= 0 ? len : 0;
}

void Log(LogLevel level, const char* fmt, ...)
{
    const char* prefix;
    switch (level) {
        case LogLevel::Debug: prefix = "[DEBUG] "; break;
        case LogLevel::Info: prefix = "[INFO] "; break;
        case LogLevel::Warn: prefix = "[WARN] "; break;
        case LogLevel::Error: prefix = "[ERROR] "; break;
        default: prefix = "[LOG] "; break;
    }
    
    printf("%s", prefix);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    fflush(stdout);
}

// Thread implementation
struct ThreadData {
    std::function<void()> func;
    pthread_t thread;
};

static void* ThreadProc(void* data)
{
    ThreadData* td = static_cast<ThreadData*>(data);
    td->func();
    return nullptr;
}

Thread* Thread_Create(std::function<void()> func)
{
    ThreadData* td = new ThreadData;
    td->func = func;
    
    if (pthread_create(&td->thread, nullptr, ThreadProc, td) != 0) {
        delete td;
        return nullptr;
    }
    
    return reinterpret_cast<Thread*>(td);
}

void Thread_Free(Thread* thread)
{
    ThreadData* td = reinterpret_cast<ThreadData*>(thread);
    pthread_cancel(td->thread);
    delete td;
}

void Thread_Wait(Thread* thread)
{
    ThreadData* td = reinterpret_cast<ThreadData*>(thread);
    pthread_join(td->thread, nullptr);
}

// Semaphore implementation
Semaphore* Semaphore_Create()
{
    sem_t* sem = new sem_t;
    if (sem_init(sem, 0, 0) != 0) {
        delete sem;
        return nullptr;
    }
    return reinterpret_cast<Semaphore*>(sem);
}

void Semaphore_Free(Semaphore* sema)
{
    sem_t* sem = reinterpret_cast<sem_t*>(sema);
    sem_destroy(sem);
    delete sem;
}

void Semaphore_Reset(Semaphore* sema)
{
    sem_t* sem = reinterpret_cast<sem_t*>(sema);
    int val;
    while (sem_getvalue(sem, &val) == 0 && val > 0) {
        sem_wait(sem);
    }
}

void Semaphore_Wait(Semaphore* sema)
{
    sem_wait(reinterpret_cast<sem_t*>(sema));
}

bool Semaphore_TryWait(Semaphore* sema, int timeout_ms)
{
    sem_t* sem = reinterpret_cast<sem_t*>(sema);
    
    if (timeout_ms == 0) {
        return sem_trywait(sem) == 0;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    return sem_timedwait(sem, &ts) == 0;
}

void Semaphore_Post(Semaphore* sema, int count)
{
    sem_t* sem = reinterpret_cast<sem_t*>(sema);
    for (int i = 0; i < count; i++) {
        sem_post(sem);
    }
}

// Mutex implementation
Mutex* Mutex_Create()
{
    pthread_mutex_t* mutex = new pthread_mutex_t;
    if (pthread_mutex_init(mutex, nullptr) != 0) {
        delete mutex;
        return nullptr;
    }
    return reinterpret_cast<Mutex*>(mutex);
}

void Mutex_Free(Mutex* mutex)
{
    pthread_mutex_t* m = reinterpret_cast<pthread_mutex_t*>(mutex);
    pthread_mutex_destroy(m);
    delete m;
}

void Mutex_Lock(Mutex* mutex)
{
    pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mutex));
}

void Mutex_Unlock(Mutex* mutex)
{
    pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mutex));
}

bool Mutex_TryLock(Mutex* mutex)
{
    return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(mutex)) == 0;
}

void Sleep(u64 usecs)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

u64 GetMSCount()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

u64 GetUSCount()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

// Stub implementations for functions we don't need in basic frontend
void WriteDateTime(int year, int month, int day, int hour, int minute, int second, void* userdata) {}
void WriteNDSSave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen, void* userdata) {}
void WriteGBASave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen, void* userdata) {}
void WriteFirmware(const Firmware& firmware, u32 writeoffset, u32 writelen, void* userdata) {}
void WriteGBAAddonSave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen, void* userdata) {}

// Network stubs
void MP_Begin(void* userdata) {}
void MP_End(void* userdata) {}
int MP_SendPacket(u8* data, int len, u64 timestamp, void* userdata) { return 0; }
int MP_SendCmd(u8* data, int len, u64 timestamp, void* userdata) { return 0; }
int MP_SendReply(u8* data, int len, u64 timestamp, u16 aid, void* userdata) { return 0; }
int MP_SendAck(u8* data, int len, u64 timestamp, void* userdata) { return 0; }
int MP_RecvPacket(u8* data, u64* timestamp, void* userdata) { return 0; }
int MP_RecvHostPacket(u8* data, u64* timestamp, void* userdata) { return 0; }
u16 MP_RecvReplies(u8* data, u64 timestamp, u16 aidmask, void* userdata) { return 0; }

int Net_SendPacket(u8* data, int len, void* userdata) { return 0; }
int Net_RecvPacket(u8* data, void* userdata) { return 0; }

// Camera stubs
void Camera_Start(int num, void* userdata) {}
void Camera_Stop(int num, void* userdata) {}
void Camera_CaptureFrame(int num, u32* frame, int width, int height, bool yuv, void* userdata) {}

} 