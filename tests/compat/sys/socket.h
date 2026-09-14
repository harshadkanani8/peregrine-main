#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
typedef int ssize_t;
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

inline int setsockopt_compat(SOCKET s, int level, int optname, const void* optval, int optlen) {
    return ::setsockopt(s, level, optname, reinterpret_cast<const char*>(optval), optlen);
}
#define setsockopt setsockopt_compat

#else
#include_next <sys/socket.h>
#endif
