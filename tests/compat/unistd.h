#pragma once
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <winsock2.h>
inline int close(int fd) {
    return closesocket(fd);
}
#else
#include_next <unistd.h>
#endif
