# Third-Party Licenses & Dependencies

The **Peregrine C++ Web Framework** may use, link against, or integrate with third-party software components and system libraries.

Third-party components remain subject to their respective licenses. Nothing in the Peregrine license overrides, modifies, or alters the terms of any third-party license.

---

## External Dependencies

### 1. OpenSSL

Peregrine uses OpenSSL for TLS/HTTPS transport security, cryptographic session signing, and CSRF token generation.

- **OpenSSL 3.0 and newer**: Licensed under the **Apache License, Version 2.0** (`Apache-2.0`).
- **OpenSSL 1.1.1 (legacy LTS)**: Licensed under the **Dual OpenSSL and SSLeay License**.
- **Website**: [https://www.openssl.org/](https://www.openssl.org/)
- **Source Repository**: [https://github.com/openssl/openssl](https://github.com/openssl/openssl)

---

### 2. POSIX Threads (pthreads)

On POSIX-compliant operating systems (Linux, macOS, BSD), Peregrine utilizes standard C++11 threading primitives backed by the system's POSIX Threads library (`libpthread`).

- **Linux (glibc)**: Distributed under the **GNU Lesser General Public License v2.1 or later** (`LGPL-2.1-or-later`) with standard runtime system library exception.
- **Windows (MinGW-w64 / winpthreads)**: Distributed under **MIT / BSD-style** permissive licenses.
- **Website**: [https://sourceware.org/pthreads-win32/](https://sourceware.org/pthreads-win32/)

---

### 3. Windows Sockets 2 (ws2_32)

On Microsoft Windows platforms, Peregrine links against the system-provided Winsock2 library (`ws2_32.dll` / `ws2_32.lib`) for network socket communication.

- **Provider**: Microsoft Corporation
- **License**: Standard Microsoft Windows OS System Component