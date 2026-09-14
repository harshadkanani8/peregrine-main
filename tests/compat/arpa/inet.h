#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

inline int inet_pton(int af, const char* src, void* dst) {
    sockaddr_in sa;
    int len = sizeof(sa);
    if (WSAStringToAddressA(const_cast<char*>(src), af, NULL, (LPSOCKADDR)&sa, &len) == 0) {
        memcpy(dst, &sa.sin_addr, sizeof(in_addr));
        return 1;
    }
    return 0;
}

inline const char* inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = af;
    memcpy(&sa.sin_addr, src, sizeof(in_addr));
    DWORD s = size;
    if (WSAAddressToStringA((LPSOCKADDR)&sa, sizeof(sa), NULL, dst, &s) == 0) {
        return dst;
    }
    return NULL;
}
#else
#include_next <arpa/inet.h>
#endif
