# 🗺️ Peregrine Framework Architectural Roadmap & Enhancements

This document outlines the architectural analysis, competitive benchmarking, and future technical roadmap for the **Peregrine C++ Web Framework**.

---

## 📋 Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Competitive Benchmark Analysis](#2-competitive-benchmark-analysis)
   - [Peregrine vs. Crow](#peregrine-vs-crow)
   - [Peregrine vs. Drogon](#peregrine-vs-drogon)
   - [Peregrine vs. Oat++](#peregrine-vs-oat)
   - [Feature Comparison Matrix](#feature-comparison-matrix)
3. [Current Architectural Strengths](#3-current-architectural-strengths)
4. [Tiered Enhancement Roadmap](#4-tiered-enhancement-roadmap)
   - [Tier 1: High-Priority Core Protocol Enhancements (Near-Term)](#tier-1-high-priority-core-protocol-enhancements-near-term)
   - [Tier 2: Asynchronous & Real-Time Engine (Mid-Term)](#tier-2-asynchronous--real-time-engine-mid-term)
   - [Tier 3: Enterprise & Distributed Systems (Long-Term)](#tier-3-enterprise--distributed-systems-long-term)
5. [Implementation Specifications](#5-implementation-specifications)
   - [Keep-Alive Connection Pipeline Specification](#keep-alive-connection-pipeline-specification)
   - [Chunked Transfer Encoding Specification](#chunked-transfer-encoding-specification)
   - [WebSocket Handshake & Frame Engine Specification](#websocket-handshake--frame-engine-specification)
6. [Conclusion](#6-conclusion)

---

## 1. Executive Summary

Peregrine was engineered as a **lightweight, modern, header-only C++11 web framework** providing a high-productivity developer experience akin to Python's Flask, combined with the raw execution speed, determinism, and minimal footprint of native compiled C++.

To ensure Peregrine remains a premier choice for embedded systems, microservices, and high-performance edge computing, this roadmap identifies architectural optimizations and protocol additions inspired by modern networking standards and peer frameworks.

---

## 2. Competitive Benchmark Analysis

### Peregrine vs. Crow
* **Crow Overview**: A C++14/17 microframework inspired by Flask, using Boost.Asio for its asynchronous event loop.
* **Architectural Differences**:
  - *Dependencies*: Crow requires Boost.Asio (or standalone Asio), introducing substantial compile-time overhead and complex template instantiations. Peregrine is **strictly header-only C++11** with zero mandatory third-party dependencies beyond standard POSIX/Winsock sockets and optional OpenSSL.
  - *Portability*: Peregrine compiles cleanly on resource-constrained embedded toolchains (e.g., 32-bit ARM Linux, GCC 4.8+) without triggering ABI warnings (such as GCC's `-Wpsabi` on 32-bit ARM due to Peregrine's PIMPL JSON design).
  - *Opportunities for Peregrine*: Crow supports HTTP/1.1 Keep-Alive connections and basic WebSocket endpoints out-of-the-box.

### Peregrine vs. Drogon
* **Drogon Overview**: An asynchronous C++14/17 framework based on non-blocking I/O (epoll/kqueue) and a reactive actor model.
* **Architectural Differences**:
  - *Concurrency Model*: Drogon uses an event-driven reactor pattern delivering ultra-high throughput under tens of thousands of concurrent idle connections (C10K problem). Peregrine uses a pre-allocated worker thread pool with bounded queues. While Drogon scales further on massive concurrent idle connections, Peregrine offers significantly lower latency for CPU-bound tasks, trivial debugging (synchronous call stacks per request), and zero asynchronous state machine complexity.
  - *Complexity*: Drogon requires CMake code generation, reflection helpers, and extensive runtime libraries. Peregrine requires only `#include <peregrine/peregrine.hpp>`.
  - *Opportunities for Peregrine*: Adding an optional non-blocking reactor engine for edge deployments with high client concurrency.

### Peregrine vs. Oat++
* **Oat++ Overview**: A zero-dependency, object-oriented C++11 web framework with an extensive module ecosystem.
* **Architectural Differences**:
  - *Design Philosophy*: Oat++ uses code-generation macros (`DTO`, `ENDPOINT`) and complex class hierarchies. Peregrine prioritizes standard functional programming: lambdas, standard maps, and intuitive method calls (`app.get(...)`, `res = Response::json(...)`).
  - *Binary Footprint*: Peregrine compiles to ultra-compact binaries (< 500 KB stripped), making it ideal for edge gateways, IoT microcontrollers running Linux, and sidecar proxies.
  - *Opportunities for Peregrine*: Oat++ features native Swagger/OpenAPI documentation generation and chunked body streaming.

### Feature Comparison Matrix

| Feature | Peregrine | Crow | Drogon | Oat++ |
| :--- | :---: | :---: | :---: | :---: |
| **Language Standard** | **C++11** | C++14 / C++17 | C++14 / C++17 | C++11 |
| **Header-Only Architecture** | **Yes** | Header-only (needs Asio) | No (Shared/Static Lib) | No (Multiple Modules) |
| **External Dependencies** | **None** (OpenSSL optional) | Boost.Asio / Asio | trantor, c-ares, jsoncpp | None |
| **PIMPL Zero-ABI JSON Engine** | **Yes** | Built-in / RapidJSON | JsonCpp | Built-in DTOs |
| **Stateless Signed Sessions (HMAC-SHA256)** | **Yes (Built-in)** | No (Custom) | Plugin / Redis | Custom |
| **Multi-Vector CSRF Defense** | **Yes (Built-in)** | No | No | No |
| **Built-in Mustache Templating** | **Yes** | Mustache submodule | CSP Templates | None |
| **Zero-Config SPA Serving** | **Yes** | Manual routing | Static plugin | Manual routing |
| **HTTP/1.1 Keep-Alive** | *Planned (Tier 1)* | Yes | Yes | Yes |
| **Chunked Transfer Encoding** | *Planned (Tier 1)* | Yes | Yes | Yes |
| **WebSocket Protocol (RFC 6455)** | *Planned (Tier 2)* | Yes | Yes | Yes |
| **Non-blocking Event Loop (epoll/kqueue)** | *Planned (Tier 2)* | Asio Reactor | Native epoll/kqueue | Custom EventLoop |

---

## 3. Current Architectural Strengths

Peregrine already excels in several critical areas:
1. **Developer Experience**: Intuitive Flask-style API with dynamic route parameter binding (`<int:id>`, `<string:slug>`, `<path:filepath>`).
2. **Production Security**: Built-in HMAC-SHA256 session tampering prevention, key rotation fallbacks, constant-time token comparison, and strict CWE-22 Path Traversal guards.
3. **Embedded Optimization**: The PIMPL-refactored JSON engine maintains a strict 4-byte/8-byte pointer size, preventing ARM ABI calling convention diagnostics across embedded compiler toolchains.
4. **Single-Page Application (SPA) Integration**: First-class `serve_spa()` handles index fallback, assets, and API pass-through with a single function call.

---

## 4. Tiered Enhancement Roadmap

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                      PEREGRINE ENHANCEMENT ROADMAP                     │
 └────────────────────────────────────────────────────────────────────────┘
       │
       ├─► [ Tier 1: High-Priority Core Protocol (Near-Term) ]
       │     ├── HTTP/1.1 Persistent Connections (Keep-Alive Pipeline)
       │     ├── Chunked Transfer Encoding (Transfer-Encoding: chunked)
       │     ├── Array & Map Query Parameter Parsing (foo[]=1&foo[]=2)
       │     └── Compression Middleware (gzip / deflate via zlib)
       │
       ├─► [ Tier 2: Asynchronous & Real-Time Engine (Mid-Term) ]
       │     ├── WebSocket Protocol Engine (RFC 6455 Full Duplex)
       │     ├── Non-Blocking Event-Driven Polling Engine (epoll/kqueue/IOCP)
       │     ├── Server-Sent Events (SSE) Native Abstraction
       │     └── Automated OpenAPI / Swagger Spec Exporter
       │
       └─► [ Tier 3: Enterprise & Distributed Systems (Long-Term) ]
             ├── HTTP/2 Binary Framing & Multiplexing (RFC 7540)
             ├── Pluggable Distributed Session Stores (Redis / Memcached)
             └── OpenTelemetry Metrics & Distributed Tracing
```

---

### Tier 1: High-Priority Core Protocol Enhancements (Near-Term)

#### 1. HTTP/1.1 Persistent Connections (Keep-Alive Pipelining)
* **Rationale**: Re-establishing TCP handshakes and TLS sessions for every HTTP request degrades latency and throughput on high-traffic connections.
* **Target Design**:
  - Worker threads inspect the `Connection: keep-alive` header (default in HTTP/1.1).
  - Instead of closing the connection immediately after writing the serialized response, the socket is retained in a reuse loop up to `max_keepalive_requests` (default: 100) or until `keepalive_timeout_seconds` (default: 5s) expires.
  - Worker threads yield back to the listener or use non-blocking socket polling between requests.

#### 2. Chunked Transfer Encoding (`Transfer-Encoding: chunked`)
* **Rationale**: Enables streaming arbitrarily large payloads (e.g., big file exports, dynamic datasets, sensor streams) without pre-computing the `Content-Length` header or buffering the entire body in RAM.
* **Target Design**:
  - Introduce `res.write_chunk(const std::string& chunk)`.
  - Format wire transmission as `<hex-length>\r\n<data>\r\n`, terminated with a zero-length chunk `0\r\n\r\n`.

#### 3. Structured Query & Form Parameter Parsing
* **Rationale**: Modern frontend clients frequently send arrays and nested objects via query parameters (e.g., `filter[]=active&filter[]=pending` or `sort[key]=name`).
* **Target Design**:
  - Add helper methods to `Request`: `req.arg_list("filter")` returning `std::vector<std::string>`.
  - Parse repeated query keys into unified collections without breaking backward compatibility with `req.arg()`.

#### 4. Automatic Wire Compression Middleware
* **Rationale**: Compress HTML, JSON, and text responses over the wire to reduce bandwidth consumption by up to 70%.
* **Target Design**:
  - Optional CMake flag `-DPEREGRINE_ENABLE_ZLIB=ON`.
  - Middleware inspects `Accept-Encoding: gzip, deflate`. If the response size exceeds 1024 bytes and the MIME type is compressible, apply gzip compression and inject `Content-Encoding: gzip`.

---

### Tier 2: Asynchronous & Real-Time Engine (Mid-Term)

#### 1. WebSocket Protocol Engine (RFC 6455)
* **Rationale**: Support bidirectional, real-time communication for live dashboards, chat applications, telemetry feeds, and IoT control loops.
* **Target Design**:
  - HTTP route upgrade detection (`Upgrade: websocket`, `Connection: Upgrade`).
  - Calculate `Sec-WebSocket-Accept` using SHA-1 and Base64 of `Sec-WebSocket-Key + GUID`.
  - Expose intuitive WebSocket handler interfaces:
    ```cpp
    app.websocket("/ws/telemetry", [](WebSocketSession& ws) {
        ws.on_open([](WebSocketConnection& conn) {
            conn.send_text("Connected to telemetry feed");
        });
        ws.on_message([](WebSocketConnection& conn, const std::string& msg, bool is_binary) {
            conn.send_text("Echo: " + msg);
        });
        ws.on_close([](WebSocketConnection& conn, int code, const std::string& reason) {
            // Cleanup
        });
    });
    ```

#### 2. Non-Blocking Event-Driven Polling Engine
* **Rationale**: Scale to 50,000+ simultaneous idle connections without allocating one thread per client socket.
* **Target Design**:
  - Abstract socket readiness behind an `EventEngine` interface with platform-specific backends:
    - Linux: `epoll`
    - macOS / BSD: `kqueue`
    - Windows: `IOCP` or `WSAPoll`
  - Allow developers to choose between the **Thread Pool Engine** (default, best for CPU/file workloads) and the **Reactor Engine** (best for ultra-high concurrency microservices).

#### 3. Native Server-Sent Events (SSE)
* **Rationale**: Lightweight alternative to WebSockets for unidirectional server-to-client streaming (e.g., AI model token streaming, live progress notifications).
* **Target Design**:
  - Dedicated response helper `Response::sse(...)`:
    ```cpp
    app.get("/events", [](Request& req) {
        return Response::sse([](SSEStream& sse) {
            sse.send_event("temperature", "23.4 C");
            sse.send_event("status", "nominal");
        });
    });
    ```

---

### Tier 3: Enterprise & Distributed Systems (Long-Term)

#### 1. HTTP/2 Binary Framing & Multiplexing (RFC 7540)
* **Rationale**: Eliminate head-of-line blocking across single TCP connections and allow concurrent stream requests over TLS.
* **Target Design**:
  - Support ALPN negotiation (`h2`) during TLS handshake.
  - Implement HTTP/2 HPACK header compression and frame demultiplexing engine.

#### 2. Pluggable Session Backends
* **Rationale**: Enable horizontal clustering of Peregrine instances behind a reverse proxy (e.g., NGINX / HAProxy) with centralized session state.
* **Target Design**:
  - Introduce `SessionStore` abstract interface.
  - Provide reference implementations for Redis and Memcached alongside the default stateless HMAC-SHA256 cookie engine.

#### 3. OpenTelemetry & Distributed Metrics
* **Rationale**: Cloud-native observability for enterprise deployments.
* **Target Design**:
  - Automatic injection of `traceparent` and correlation IDs.
  - Prometheus-compatible `/metrics` endpoint exporting request counters, p50/p95/p99 latency histograms, and thread pool saturation metrics.

---

## 5. Implementation Specifications

### Keep-Alive Connection Pipeline Specification

```text
 Client                       Worker Thread
   │                                │
   │─────── HTTP/1.1 Request 1 ─────►│ Read request
   │                                │ Dispatch handler & build response
   │◄────── HTTP/1.1 200 OK ────────│ Send response (Connection: keep-alive)
   │        (Socket Kept Open)      │
   │                                │ Worker enters wait loop on socket
   │─────── HTTP/1.1 Request 2 ─────►│ Read request (within timeout window)
   │                                │ Dispatch handler & build response
   │◄────── HTTP/1.1 200 OK ────────│ Send response
   │                                │
   │   (Timeout / Max Requests)     │
   │◄────── Connection: close ──────│ Terminate connection & recycle worker
```

**Algorithm**:
1. After executing `serialize_response()`, check if `req.http_version == "HTTP/1.1"` and `req.header("Connection") != "close"`.
2. If true, set `res.headers["Connection"] = "keep-alive"` and `res.headers["Keep-Alive"] = "timeout=5, max=100"`.
3. Flush response bytes. Reset `req` and loop back to `detail::read_full_request(conn, max_body_size)`.
4. If socket read times out (using `SO_RCVTIMEO`) or returns `0` (client closed), break loop and close socket.

---

### Chunked Transfer Encoding Specification

```text
 HTTP/1.1 200 OK\r\n
 Content-Type: application/octet-stream\r\n
 Transfer-Encoding: chunked\r\n
 \r\n
 1E\r\n
 First dynamic data segment...\r\n
 1F\r\n
 Second dynamic data segment..\r\n
 0\r\n
 \r\n
```

**API Design**:
```cpp
app.get("/stream/large-dataset", [](Request& req) {
    return Response::chunked([](ChunkWriter& writer) {
        for (int i = 0; i < 1000; ++i) {
            writer.write("Row data: " + std::to_string(i) + "\n");
        }
    });
});
```

---

### WebSocket Handshake & Frame Engine Specification

**Handshake Calculation**:
```text
Client Header : Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Magic GUID    : 258EAFA5-E914-47DA-95CA-C5AB0DC85B11
Concat String : dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11
SHA-1 Hash    : b3 7a 4f 2c c0 62 4f 16 90 f6 46 06 cf 38 59 45 b2 be c4 ea
Base64 Result : s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Server Header : Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

Once the handshake response (`101 Switching Protocols`) is sent, the connection enters full-duplex binary/text framing per RFC 6455.

---

## 6. Conclusion

Peregrine already provides an exceptional foundation: it is self-contained, memory-safe, computationally fast, and highly developer-friendly. 

By executing the prioritized roadmap outlined above—starting with **HTTP/1.1 Keep-Alive** and **Chunked Streaming** in Tier 1, followed by **WebSockets** and **Event-Driven Concurrency** in Tier 2—Peregrine will solidify its position as one of the most versatile, high-performance C++11 web frameworks available for embedded and cloud architectures alike.
