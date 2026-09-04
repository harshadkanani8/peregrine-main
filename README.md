# 🦅 Peregrine C++ Web Framework

> A lightweight, modular, and high-performance C++11 web framework. Peregrine brings modern, ergonomic web development to the strict and performant world of native C++.

Peregrine is designed to be elegant and intuitive. It scales seamlessly from a single-file microservice to a massive, modular REST API, complete with built-in security, templating, and session management.

---

## ✨ Key Features

- **Header-Only Simplicity**: Just include `<peregrine/peregrine.hpp>` and compile. No complex dependencies required.
- **Intuitive Routing**: Clean, lambda-based routing with dynamic path variables (`<int:id>`, `<string:name>`, `<path:filepath>`).
- **Modular Architecture (Blueprints)**: Organize your application into distinct modules and route groups using `peregrine::Blueprint`.
- **First-Class JSON Support**: Built-in JSON parsing and serialization (`peregrine::Json`) that maps effortlessly to HTTP requests and responses.
- **Stateless Secure Sessions**: Cryptographically signed client cookies (HMAC-SHA256) for session state without server-side memory overhead.
- **CSRF Defense**: Automatic or manual Cross-Site Request Forgery token validation with constant-time cryptographic comparisons.
- **Single Page Application (SPA) Serving**: Zero-config static asset serving designed specifically for React, Vue, and Vite frontends (`app.serve_spa("dist")`).
- **Real-Time Streaming**: Native support for chunked streams and live MJPEG video feeds (`Response::mjpeg`).
- **Protocol Flexibility**: Run on raw HTTP or strong TLS 1.2/1.3 HTTPS with a simple configuration struct.

---

## 📁 Framework Architecture

Peregrine is modular by design. The core engine is built on standard POSIX sockets and `std::thread`, providing a thread-per-connection concurrency model.

```text
include/peregrine/
├── peregrine.hpp    # Master header (includes all modules)
├── app.hpp          # peregrine::Peregrine application coordinator & server loop
├── blueprint.hpp    # peregrine::Blueprint (modular routing & scoped hooks)
├── request.hpp      # Request wrapper (query, form, json, files, cookies, session)
├── response.hpp     # Response wrapper (text, html, json, redirect, stream, mjpeg)
├── routing.hpp      # Regex-based URL routing & parameter extraction
├── session.hpp      # HMAC-SHA256 signed stateless cookie session engine
├── template.hpp     # Fast template engine ({{var}}, {{#each list}})
└── types.hpp        # ServerConfig, UploadedFile, MIME type mapper
```

---

## 🚀 Quick Start

### 1. A Minimal Application (`main.cpp`)

```cpp
#include "peregrine/peregrine.hpp"

using namespace peregrine;

int main() {
    Peregrine app("my_app");
    
    // Enable secure sessions
    app.secret_key = "super_secret_cryptographic_key";

    // A simple HTML route
    app.get("/", [](Request& req) {
        return Response::html("<h1>Welcome to Peregrine!</h1>");
    });

    // A RESTful JSON API route with path variables
    app.get("/api/users/<int:id>", [](Request& req) {
        int user_id = std::stoi(req.path_params["id"]);
        
        Json response = Json::object();
        response["id"] = user_id;
        response["status"] = "active";
        
        return Response::json(response);
    });

    // Start the multi-threaded HTTP server on port 8080
    app.run("0.0.0.0", 8080);
    return 0;
}
```

---

## 🛠️ Build & Run

Peregrine uses standard C++ build systems. It requires **C++11**, **pthreads**, and **OpenSSL** (for cryptography and HTTPS).

### Using CMake

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(PeregrineApp)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(server main.cpp)
target_link_libraries(server OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
```

**Build Commands:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the compiled binary
./build/server
```

---

## 📖 Comprehensive Documentation

Peregrine comes with extensive, deeply detailed documentation located in the `docs/` directory.

- [**Peregrine User Guide**](docs/user_guide.md)
  A complete walkthrough covering Installation, Routing, Requests/Responses, Sessions, and Blueprints.
- [**Peregrine API Reference**](docs/api_reference.md)
  The exhaustive technical manual documenting every class, method, and module with full code examples.

---

*Copyright (c) 2026. All rights reserved.*
