# 📦 Module 01: Application Coordinator & Server Engine

The **Application Coordinator** is the central orchestrator of a Peregrine web application. It encapsulates route dispatching, blueprint registration, lifecycle hooks, static asset delivery, cryptographic session signing, and the multi-threaded POSIX / OpenSSL network listener.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [The `peregrine::App` Class](#2-the-peregrineapp-class)
   - [Constructors & Configuration](#constructors--configuration)
   - [Application Configuration (`peregrine::Config`)](#application-configuration-peregrineconfig)
3. [Server Execution & `ServerConfig`](#3-server-execution--serverconfig)
   - [Basic HTTP Listener](#basic-http-listener)
   - [Production TLS / HTTPS Listener](#production-tls--https-listener)
   - [`ServerConfig` Reference](#serverconfig-reference)
4. [Static File & SPA Engine](#4-static-file--spa-engine)
   - [Static Directory Mapping (`static_dir`)](#static-directory-mapping-static_dir)
   - [Single-Page Application Hosting (`serve_spa`)](#single-page-application-hosting-serve_spa)
5. [Graceful Handling & Overload Protection](#5-graceful-handling--overload-protection)
6. [Complete Code Examples](#6-complete-code-examples)

---

## 1. Architectural Overview

At runtime, `peregrine::App` coordinates incoming TCP connections between the operating system socket layer and your business logic:

```text
  Client Handshake ──► [ Master Socket (accept()) ]
                                │
                                ▼
                       [ ThreadPool Enqueue ]
                                │
               ┌────────────────┴────────────────┐
               ▼                                 ▼
       [ Worker Thread 1 ]               [ Worker Thread N ]
        1. Parse HTTP/TLS Wire            1. Parse HTTP/TLS Wire
        2. Restore Session (HMAC)         2. Restore Session (HMAC)
        3. Execute Before-Hooks           3. Execute Before-Hooks
        4. Match Route & Dispatch         4. Match Route & Dispatch
        5. Execute After-Hooks            5. Execute After-Hooks
        6. Sign Session Cookie            6. Sign Session Cookie
        7. Transmit HTTP Wire Bytes       7. Transmit HTTP Wire Bytes
```

---

## 2. The `peregrine::App` Class

Defined in `include/peregrine/app.hpp`. An alias `using Peregrine = App;` is also provided.

### Constructors & Configuration

```cpp
// Default constructor
peregrine::App app;

// Constructor specifying application name and static folder
peregrine::App app("my_service", "dist", "");
```

#### Public Member Variables:
* `std::string import_name`: Identifier for the application instance (default: `"app"`).
* `std::string secret_key`: Cryptographic secret used for HMAC-SHA256 session cookie signing.
* `std::vector<std::string> secret_key_fallbacks`: Ordered list of older secret keys used for seamless zero-downtime key rotation.
* `std::string session_cookie_name`: Name of the session cookie transmitted to clients (default: `"session"`).
* `int session_lifetime_seconds`: Expiration duration for permanent sessions in seconds (default: `86400` / 24 hours).
* `bool csrf_protect`: Flag indicating whether CSRF protection is active (default: `false`).
* `std::vector<std::string> csrf_exempt_paths`: Paths excluded from CSRF validation.
* `std::string static_folder`: Path to directory containing static assets.
* `std::string static_url_path`: Base URL prefix for static assets.
* `std::string template_folder`: Folder path where templates are located (default: `"templates"`).

---

### Application Configuration (`peregrine::Config`)

Defined in `include/peregrine/config.hpp`. Accessible via `app.config`.

`Config` stores key-value pairs as strings and offers type-safe accessor methods:

```cpp
#include <peregrine/peregrine.hpp>

peregrine::App app;

// 1. Direct key indexing
app.config["DATABASE_URL"] = "postgres://user:pass@localhost:5432/production";
app.config["MAX_UPLOAD_MB"] = "32";

// 2. String accessor with default fallback
std::string db = app.config.get("DATABASE_URL", "sqlite://default.db");

// 3. Integer accessor
int max_mb = app.config.get_int("MAX_UPLOAD_MB", 16);

// 4. Boolean accessor ("true", "1", "yes" -> true)
bool debug = app.config.get_bool("DEBUG", false);

// 5. Load directly from environment variable
app.config.from_env("PORT", "SERVER_PORT");
```

---

## 3. Server Execution & `ServerConfig`

Defined in `include/peregrine/types.hpp`.

### Basic HTTP Listener

The simplest way to start the server is passing the host and port:

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    app.get("/", [](peregrine::Request& req) {
        return peregrine::Response::text("Hello from Peregrine!");
    });

    // Binds to 0.0.0.0:8080 and blocks in the connection loop
    app.run("0.0.0.0", 8080);
    return 0;
}
```

---

### Production TLS / HTTPS Listener

To launch with TLS encryption, configure `ServerConfig` and pass it to `app.run(cfg)`:

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    peregrine::ServerConfig cfg;
    cfg.protocol = peregrine::Protocol::HTTPS;
    cfg.host = "0.0.0.0";
    cfg.port = 8443;
    cfg.cert_file = "certs/server.crt";
    cfg.key_file = "certs/server.key";
    cfg.tls_version = "TLS_1_3";           // "TLS_1_2" or "TLS_1_3"
    cfg.thread_pool_size = 16;             // 16 concurrent worker threads
    cfg.max_queue_size = 128;              // Queue limit before 503 backpressure
    cfg.socket_timeout_seconds = 15;       // Slowloris mitigation timeout

    app.get("/secure", [](peregrine::Request& req) {
        return peregrine::Response::text("Encrypted channel verified.");
    });

    app.run(cfg);
    return 0;
}
```

---

### `ServerConfig` Reference

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `protocol` | `Protocol` | `Protocol::HTTP` | Protocol scheme (`Protocol::HTTP` or `Protocol::HTTPS`). |
| `host` | `std::string` | `"0.0.0.0"` | Network interface address to bind. |
| `port` | `int` | `5000` | TCP port to listen on. |
| `backlog` | `int` | `128` | OS socket listen queue depth (`SOMAXCONN`). |
| `cert_file` | `std::string` | `""` | Path to PEM-encoded TLS certificate file. |
| `key_file` | `std::string` | `""` | Path to PEM-encoded TLS private key file. |
| `tls_version` | `std::string` | `"TLS_1_2"` | Minimum TLS version (`"TLS_1_2"` or `"TLS_1_3"`). |
| `max_body_size`| `size_t` | `16777216` (16 MB) | Maximum permitted incoming request body size in bytes. |
| `thread_pool_size` | `size_t` | `8` | Number of worker threads allocated at startup. |
| `max_queue_size` | `size_t` | `64` | Maximum pending connections queued before rejecting load. |
| `socket_timeout_seconds` | `int` | `10` | Read and write socket timeout in seconds. |

---

## 4. Static File & SPA Engine

### Static Directory Mapping (`static_dir`)

Map a physical directory on disk to a URL route prefix with automatic MIME-type detection and directory traversal protection:

```cpp
// Maps GET /static/* -> files in ./public/*
app.static_dir("/static", "./public");
```

When a user requests `/static/css/theme.css`:
1. Peregrine extracts the `filepath` parameter (`css/theme.css`).
2. Checks for `..` tokens or null-byte characters to prevent Path Traversal attacks.
3. Reads the binary data from disk.
4. Detects the MIME type (`text/css; charset=utf-8`) from the file extension.
5. Returns a 200 response or a 404 response if the file does not exist.

---

### Single-Page Application Hosting (`serve_spa`)

Peregrine includes built-in, zero-configuration support for frontend applications built with React, Vue, Angular, or Vite:

```cpp
// Serves static files from "dist", falling back to "index.html" for HTML5 routing
app.serve_spa("dist", "index.html", "api");
```

#### Resolution Flow:
1. `GET /`: Serves `dist/index.html`.
2. `GET /assets/app.js`: File exists in `dist/assets/app.js` -> serves `dist/assets/app.js` with matching MIME type.
3. `GET /dashboard/settings`: File does **not** exist on disk, and the path does **not** begin with the `api` prefix -> serves `dist/index.html` with status 200, allowing the client-side router to render the view.
4. `GET /api/v1/unknown`: Path begins with the `api` prefix -> does **not** fallback to `index.html`; returns `404 Not Found` JSON error.
5. `GET /.git/config` or `GET /.well-known/*`: Dotfiles and system probes are immediately rejected with `404 Not Found`.

---

## 5. Graceful Handling & Overload Protection

Peregrine guards against denial-of-service and runaway queue memory:

1. **Slowloris Mitigation**: Sockets have `SO_RCVTIMEO` and `SO_SNDTIMEO` applied (`socket_timeout_seconds`). Slow clients holding connections open are aborted.
2. **Task Queue Saturation**: If all `thread_pool_size` workers are busy and `max_queue_size` pending tasks are already queued, subsequent incoming HTTP connections are immediately sent an HTTP 503 response:
   ```text
   HTTP/1.1 503 Service Unavailable
   Content-Type: text/plain; charset=utf-8
   Content-Length: 34
   Connection: close

   503 Service Temporarily Overloaded
   ```
   This prevents unbounded RAM consumption under traffic spikes.

---

## 6. Complete Code Examples

### Production Server Setup

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app("production_service");

    // Cryptographic Session Security
    app.secret_key = "CHANGE_ME_IN_PRODUCTION_SUPER_SECRET_KEY";
    app.session_cookie_name = "app_session";
    app.session_lifetime_seconds = 7 * 24 * 3600; // 7 days

    // Enable CSRF defense
    app.enable_csrf_protection({"/api/webhook", "/api/external"});

    // Application Configuration
    app.config["DATABASE_HOST"] = "127.0.0.1";
    app.config["ENVIRONMENT"] = "production";

    // Application Routes
    app.get("/health", [](peregrine::Request& req) {
        peregrine::Json status = peregrine::Json::object();
        status["status"] = "healthy";
        status["uptime_seconds"] = 1420;
        return peregrine::Response::json(status);
    });

    // Host React frontend
    app.serve_spa("frontend/dist", "index.html", "api");

    // Configure and start server
    peregrine::ServerConfig config;
    config.host = "0.0.0.0";
    config.port = 8080;
    config.thread_pool_size = 12;
    config.max_queue_size = 100;
    config.socket_timeout_seconds = 10;

    std::cout << "Starting server on port " << config.port << "...\n";
    app.run(config);

    return 0;
}
```
