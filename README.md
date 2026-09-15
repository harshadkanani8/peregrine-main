<!-- <p align="center">
  <img src="website/assets/logo.svg" width="60" height="60" alt="Logo">
  &nbsp;
  <strong style="font-size: 32px;">Peregrine++</strong>
</p> -->

# 🦅 Peregrine C++ Web Framework

[![Standard](https://img.shields.io/badge/C%2B%2B-11%20Standard-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B11)
[![Design](https://img.shields.io/badge/Architecture-Header--Only-success.svg)](#-framework-architecture)
[![Tests](https://img.shields.io/badge/Tests-52%2F52%20Passed-brightgreen.svg)](tests/)
[![Platforms](https://img.shields.io/badge/Platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#-build--installation)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

> A lightweight, modular, and high-performance C++11 web application framework. Peregrine brings modern, ergonomic web development to the strict and performant world of native C++.

Peregrine scales effortlessly from a single-file microservice to a massive, modular RESTful application, complete with built-in security, templating, and stateless session management.

---

## 📋 Table of Contents

- [✨ Key Features](#-key-features)
- [📁 Repository Architecture](#-repository-architecture)
- [🚀 60-Second Quick Start](#-60-second-quick-start)
- [🛠️ Build & Installation](#️-build--installation)
  - [Using CMake](#using-cmake)
  - [Direct Compiler Command (Linux & macOS)](#direct-compiler-command-linux--macos)
  - [Direct Compiler Command (Windows)](#direct-compiler-command-windows)
- [💡 Ready-to-Run Examples](#-ready-to-run-examples)
- [🧪 Unit Tests & Verification Suite](#-unit-tests--verification-suite)
- [📖 Documentation Hub](#-documentation-hub)

---

## ✨ Key Features

- **Header-Only Simplicity**: Include `<peregrine/peregrine.hpp>` and compile. Zero mandatory third-party package managers or complex build steps.
- **Ergonomic Routing**: Clean, lambda-based URL routing with dynamic typed path variables (`<int:id>`, `<string:name>`, `<path:filepath>`).
- **Modular Architecture (Blueprints)**: Organize routes, middlewares, and error handlers into decoupled modules using `peregrine::Blueprint`.
- **First-Class JSON AST Engine**: Built-in recursive-descent JSON parser and serializer (`peregrine::Json`) mapping directly to HTTP requests and responses.
- **Stateless Secure Sessions**: Cryptographically signed client cookies (HMAC-SHA256) for session persistence with zero server-side memory overhead.
- **Multi-Vector CSRF Protection**: Automatic or manual Cross-Site Request Forgery token validation across form bodies, custom headers, and query strings.
- **Built-in Security Defenses**: Native protection against Path Traversal attacks (CWE-22), timing attacks (constant-time XOR comparisons), and session tampering.
- **Single Page Application (SPA) Serving**: Zero-config static asset serving and client-side fallback routing designed for React, Vue, and Vite frontends (`app.serve_spa("dist")`).
- **In-Memory Testing Client**: Includes `TestClient` for socket-free, sub-millisecond integration and unit testing.

---

## 📁 Repository Architecture

```text
peregrine/
├── include/peregrine/        # Master header-only framework source
│   ├── peregrine.hpp         # Master header (includes all modules)
│   ├── app.hpp               # Peregrine Application coordinator & server loop
│   ├── blueprint.hpp         # Modular routing, prefixes & scoped hooks
│   ├── common.hpp            # String utilities, Base64URL, URL encoding, CSPRNG
│   ├── config.hpp            # Application settings dictionary & type coercion
│   ├── connection.hpp        # Sockets, HTTP wire parsing, multipart parser
│   ├── csrf.hpp              # Cross-Site Request Forgery token generator & validator
│   ├── helpers.hpp           # make_response, jsonify, redirect, abort, send_file
│   ├── json.hpp              # PIMPL recursive-descent JSON parser & serializer
│   ├── request.hpp           # HTTP Request wrapper (query, form, json, files, cookies, session)
│   ├── response.hpp          # HTTP Response wrapper (status, headers, cookies, serializers)
│   ├── routing.hpp           # Dynamic regex URL router & 405 Method Not Allowed handler
│   ├── server.hpp            # HTTP wire protocol & multipart MIME boundary parser
│   ├── session.hpp           # HMAC-SHA256 signed stateless cookie session engine
│   ├── template.hpp          # Fast mustache templating ({{var}}, {{#each list}})
│   ├── thread_pool.hpp       # Concurrency worker pool with bounded queue & overload rejection
│   ├── types.hpp             # ServerConfig, UploadedFile, MIME type mapper
│   └── view.hpp              # Class-based RESTful MethodView dispatching
├── examples/                 # Ready-to-run example applications & tutorials
│   ├── README.md             # Master guide for all examples
│   ├── CMakeLists.txt        # Unified CMake configuration for examples
│   ├── 01_hello_world/       # Minimal single-file server with text/HTML/dynamic routing
│   ├── 02_rest_api/          # Full RESTful JSON CRUD API using Blueprints
│   ├── 03_auth_sessions/     # Stateless signed cookie authentication & protected dashboard
│   ├── 04_template_rendering/# Mustache template engine with scalar vars and loops
│   └── 05_file_upload/       # Multipart file upload handling with disk persistence
├── tests/                    # Zero-dependency unit test & verification suite
│   ├── README.md             # In-memory testing user manual & TestClient guide
│   ├── test_framework.hpp    # Header-only C++11 test harness with microsecond timing
│   ├── test_client.hpp       # In-memory mock HTTP client with automated Cookie Jar
│   ├── test_main.cpp         # Main test executable entry point
│   ├── *.cpp                 # 12 modular unit test suites (52 passing tests)
│   └── compat/               # Portability layer (standalone HMAC-SHA256 & Winsock)
├── docs/                     # Comprehensive framework guides
│   ├── user_guide.md         # In-depth architectural walkthrough and tutorials
│   └── api_reference.md      # Exhaustive API reference for all classes and functions
├── CMakeLists.txt            # Modern CMake target exporter
└── CMAKE_README.md           # Step-by-step CMake configuration breakdown
```

---

## 🚀 60-Second Quick Start

Save the following code as `main.cpp`:

```cpp
#include "peregrine/peregrine.hpp"

using namespace peregrine;

int main() {
    App app("my_web_service");

    // 1. Enable secure signed sessions
    app.secret_key = "super_secret_cryptographic_key";

    // 2. Simple HTML endpoint
    app.get("/", [](Request& req) {
        return Response::html("<h1>🦅 Welcome to Peregrine!</h1>");
    });

    // 3. Dynamic RESTful JSON endpoint
    app.get("/api/users/<int:id>", [](Request& req) {
        int user_id = std::stoi(req.path_params["id"]);

        Json user = Json::object();
        user["id"] = user_id;
        user["name"] = "Developer";
        user["status"] = "active";

        return Response::json(user);
    });

    // 4. Start multi-threaded HTTP server on port 8080
    app.run("127.0.0.1", 8080);
    return 0;
}
```

---

## 🛠️ Build & Installation

Peregrine requires a standard **C++11** compiler, **OpenSSL**, and **pthreads** (plus `ws2_32` on Windows).

### Using CMake

Create a `CMakeLists.txt` in your application directory:

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyPeregrineApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(my_service main.cpp)
target_link_libraries(my_service PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(my_service PRIVATE ws2_32)
endif()
```

Build and run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/my_service
```

### Direct Compiler Command (Linux & macOS)

```bash
g++ -std=c++11 -I include main.cpp -lssl -lcrypto -lpthread -o my_service
./my_service
```

### Direct Compiler Command (Windows)

```powershell
g++ -std=c++11 -I include main.cpp -lssl -lcrypto -lpthread -lws2_32 -o my_service.exe
.\my_service.exe
```

---

## 💡 Ready-to-Run Examples

The [`examples/`](examples/) directory contains 5 complete, standalone applications with dedicated documentation:

| Example | Path | Key Concepts Demonstrated |
| :--- | :--- | :--- |
| **01. Hello World** | [`examples/01_hello_world/`](examples/01_hello_world/) | Basic server, text & HTML endpoints, path parameter extraction (`<string:name>`). |
| **02. RESTful Task API** | [`examples/02_rest_api/`](examples/02_rest_api/) | Modular `Blueprint` routing, JSON AST parsing/dumping, full CRUD, HTTP status codes. |
| **03. Auth & Sessions** | [`examples/03_auth_sessions/`](examples/03_auth_sessions/) | Stateless signed cookie sessions, login/logout, route protection, tamper rejection. |
| **04. HTML Templates** | [`examples/04_template_rendering/`](examples/04_template_rendering/) | Dynamic mustache template rendering, scalar `{{var}}`, collection loops `{{#each}}`. |
| **05. File Upload** | [`examples/05_file_upload/`](examples/05_file_upload/) | RFC 7578 `multipart/form-data`, binary disk saving, Path Traversal defense (CWE-22). |

To build all examples at once using CMake:
```bash
cmake -B build -S examples
cmake --build build
```

---

## 🧪 Unit Tests & Verification Suite

Peregrine includes an in-memory testing framework with **52 passing unit tests** across 12 test suites.

```powershell
# Run the entire test suite on Windows (PowerShell)
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_common.cpp tests/test_config.cpp tests/test_types.cpp tests/test_json.cpp tests/test_template.cpp tests/test_routing.cpp tests/test_blueprint.cpp tests/test_view.cpp tests/test_session.cpp tests/test_csrf.cpp tests/test_helpers.cpp tests/test_server.cpp -lws2_32 -o test_suite.exe; .\test_suite.exe
```

```bash
# Run the entire test suite on Linux & macOS
g++ -std=c++11 -I tests -I include tests/test_main.cpp tests/test_common.cpp tests/test_config.cpp tests/test_types.cpp tests/test_json.cpp tests/test_template.cpp tests/test_routing.cpp tests/test_blueprint.cpp tests/test_view.cpp tests/test_session.cpp tests/test_csrf.cpp tests/test_helpers.cpp tests/test_server.cpp -lssl -lcrypto -lpthread -o test_suite; ./test_suite
```

```text
[==========] Total tests executed: 52 across 24 ms.
[  PASSED  ] 52 tests passed.
[  ALL TESTS PASSED!  ]
```

For complete instructions on writing unit tests and using the in-memory `TestClient`, see the **[Peregrine Testing Guide](tests/README.md)**.

---

## 📖 Documentation Hub

- [**Peregrine User Guide**](docs/user_guide.md): Architectural guide covering installation, routing, sessions, and middlewares.
- [**Peregrine API Reference**](docs/api_reference.md): Technical manual documenting classes, methods, and types.
- [**Peregrine Testing Guide**](tests/README.md): In-memory testing manual with `TestClient` API reference and examples.
- [**Application Examples Directory**](examples/README.md): Step-by-step tutorials and runnable sample projects.
- [**Third-Party Licenses**](THIRD_PARTY_LICENSES.md): Details on external dependencies (OpenSSL, pthreads, Winsock).
- [**CMake Guide**](CMAKE_README.md): Section-by-section breakdown of the modern CMake build configuration.

---

## 📄 License

Peregrine C++ Web Framework is open-source software licensed under the **[Apache License, Version 2.0](LICENSE)**.  
See the [LICENSE](LICENSE), [NOTICE](NOTICE), and [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.md) files for details.

*Copyright (c) 2026 Harshad Kanani. All rights reserved.*
