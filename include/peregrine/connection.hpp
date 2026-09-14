 /*
 * =========================================================================
 *     ____                              _             __ __ 
 *    /  __\___  ________  ____ ________(_)_  _____   / // / 
 *   / /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \/ -_) / // /_ 
 *  / .___/\__//_/  \__/\_, /_/  /_/_/_/_//_/\__/ /__  __/ 
 * /_/                 /___/                        /_/    
 *
 *  Peregrine++ Web Application Framework
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

// ============================================================================
// peregrine/connection.hpp
//
// Protocol-agnostic socket/TLS connection wrapper.
// Provides unified read/write/close primitives over POSIX sockets (HTTP) and
// OpenSSL SSL* streams (HTTPS) with zero overhead.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <sys/socket.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace peregrine {

class Connection {
public:
    // Plain HTTP connection
    explicit Connection(int fd) : fd_(fd), ssl_(nullptr) {}

    // HTTPS connection (wraps socket fd + completed TLS handshake SSL*)
    Connection(int fd, SSL* ssl) : fd_(fd), ssl_(ssl) {}

    ssize_t read(char* buf, size_t len) {
        if (ssl_) {
            return SSL_read(ssl_, buf, static_cast<int>(len));
        }
        return recv(fd_, buf, len, 0);
    }

    ssize_t write(const char* buf, size_t len) {
        if (ssl_) {
            return SSL_write(ssl_, buf, static_cast<int>(len));
        }
#ifdef MSG_NOSIGNAL
        return send(fd_, buf, len, MSG_NOSIGNAL);
#else
        return send(fd_, buf, len, 0);
#endif
    }

    void close_conn() {
        if (ssl_) {
            SSL_shutdown(ssl_);
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    bool is_tls() const { return ssl_ != nullptr; }
    int fd() const { return fd_; }
    SSL* ssl() const { return ssl_; }

private:
    int fd_ = -1;
    SSL* ssl_ = nullptr;
};

}  // namespace peregrine
