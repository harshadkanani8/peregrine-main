# 🔄 Peregrine Architecture & Request Lifecycle (Flow-Wise Guide)

This document traces the complete **end-to-end execution flow** of a request in the **Peregrine C++ Web Framework**—from the moment an operating system socket accepts an incoming TCP packet to the moment the wire response is transmitted back to the client.

---

## 📋 Table of Contents

1. [High-Level Architectural Model](#1-high-level-architectural-model)
2. [End-to-End Request Lifecycle (11-Step Pipeline)](#2-end-to-end-request-lifecycle-11-step-pipeline)
3. [Lifecycle Sequence Diagram](#3-lifecycle-sequence-diagram)
4. [Detailed Breakdown of Each Lifecycle Phase](#4-detailed-breakdown-of-each-lifecycle-phase)
   - [Phase 1: Connection Acceptance & Worker Dispatch](#phase-1-connection-acceptance--worker-dispatch)
   - [Phase 2: Wire Protocol Parsing](#phase-2-wire-protocol-parsing)
   - [Phase 3: Cryptographic Session Verification](#phase-3-cryptographic-session-verification)
   - [Phase 4: Multi-Channel CSRF Interception](#phase-4-multi-channel-csrf-interception)
   - [Phase 5: Global Pre-Request Middlewares](#phase-5-global-pre-request-middlewares)
   - [Phase 6: URL Routing & Parameter Extraction](#phase-6-url-routing--parameter-extraction)
   - [Phase 7: Blueprint Scoped Pre-Hooks](#phase-7-blueprint-scoped-pre-hooks)
   - [Phase 8: Route Handler Execution & Exception Shield](#phase-8-route-handler-execution--exception-shield)
   - [Phase 9: Post-Request Middlewares & Header Injection](#phase-9-post-request-middlewares--header-injection)
   - [Phase 10: Session Signing & Cookie Serialization](#phase-10-session-signing--cookie-serialization)
   - [Phase 11: Wire Serialization & Socket Transmission](#phase-11-wire-serialization--socket-transmission)
5. [Concurrency Architecture & Worker Thread Pool](#5-concurrency-architecture--worker-thread-pool)
6. [Memory Model & State Isolation](#6-memory-model--state-isolation)

---

## 1. High-Level Architectural Model

Peregrine uses a **hybrid listener-worker thread pool architecture**:

```text
  [ Incoming HTTP / HTTPS Clients ]
                 │
                 ▼
      ┌─────────────────────┐
      │  Master Socket Loop │  <-- Single listening thread accepts connections
      │  (app.run())        │
      └──────────┬──────────┘
                 │ Hand-off (Bounded Queue)
                 ▼
  ┌────────────────────────────────────────────────────────┐
  │                 Worker Thread Pool                     │
  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐    │
  │  │ Worker Th. 1 │ │ Worker Th. 2 │ │ Worker Th. N │    │
  │  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘    │
  └─────────┼────────────────┼────────────────┼────────────┘
            │                │                │
            ▼                ▼                ▼
     [ Request 1 ]    [ Request 2 ]    [ Request N ]
```

1. **Master Listener Thread**: Runs inside `app.run()`. Binds to the configured IP/Port, listens for incoming TCP handshakes via `accept()`, wraps the socket file descriptor in a `Connection` object, and immediately enqueues it into the internal thread pool.
2. **Worker Pool Threads**: A pre-allocated pool of standard C++11 worker threads waiting on a condition variable. When a task arrives, an idle worker pops the connection, processes the complete HTTP request/response lifecycle independently on its own call stack, and returns to the pool.
3. **Zero Shared Mutable State**: Each request lifecycle operates on stack-allocated `Request` and `Response` objects. Shared resources (such as route tables and configuration) are strictly read-only after server startup.

---

## 2. End-to-End Request Lifecycle (11-Step Pipeline)

When a client sends an HTTP request, Peregrine processes it through an **11-step deterministic pipeline**:

```text
 Client
   │  (TCP / TLS Handshake)
   ▼
[ 1. Connection Accept ] ──> Worker Thread Enqueued from Pool
   ▼
[ 2. Wire Protocol Parse ] ──> Headers, Query, Form, Cookies, JSON, Multipart
   ▼
[ 3. Session Deserialization ] ──> HMAC-SHA256 Token Verify & Key Rotation
   ▼
[ 4. CSRF Interception ] ──> Validates Token on POST/PUT/DELETE
   ▼
[ 5. Global Before-Hooks ] ──> Executes app.before_request() Middlewares
   ▼
[ 6. Router Match & Path Extraction ] ──> Static / Regex Dynamic (<int>, <string>)
   ▼
[ 7. Blueprint Before-Hooks ] ──> Executes Blueprint-Scoped Middlewares
   ▼
[ 8. Handler Execution & Exception Shield ] ──> Invokes Lambda / MethodView
   ▼
[ 9. Post-Request Middlewares ] ──> Blueprint & Global app.after_request()
   ▼
[ 10. Session Signing & Cookie ] ──> Re-signs HMAC-SHA256 if Modified
   ▼
[ 11. Wire Serialization & Send ] ──> Transmits Status, Headers & Body to Socket
```

---

## 3. Lifecycle Sequence Diagram

```text
Client            Server/Listener       WorkerThread         Router/Handler
  │                      │                    │                     │
  │─── TCP SYN ─────────>│                    │                     │
  │<── TCP SYN-ACK ──────│                    │                     │
  │─── TCP ACK ─────────>│                    │                     │
  │                      │── Enqueue Task ───>│                     │
  │                      │                    │                     │
  │─── HTTP GET /users ──────────────────────>│                     │
  │                      │                    │── Parse Wire Data   │
  │                      │                    │── Verify Session    │
  │                      │                    │── CSRF Verification │
  │                      │                    │── before_request()  │
  │                      │                    │                     │
  │                      │                    │── Match Route ─────>│
  │                      │                    │                     │── Execute Handler
  │                      │                    │<── Return Response ─│
  │                      │                    │                     │
  │                      │                    │── after_request()   │
  │                      │                    │── Sign Session      │
  │                      │                    │── Serialize Response│
  │<── HTTP/1.1 200 OK ───────────────────────│                     │
  │                      │                    │── Close Connection  │
  │                      │                    ▼ (Returns to Pool)   │
```

---

## 4. Detailed Breakdown of Each Lifecycle Phase

### Phase 1: Connection Acceptance & Worker Dispatch
- The master server loop inside `app.run()` calls `accept()` on the listening socket descriptor.
- If TLS is enabled (`Protocol::HTTPS`), an OpenSSL `SSL*` structure is bound to the socket and completes the TLS handshake via `SSL_accept()`.
- The connection is encapsulated in a `peregrine::Connection` object and handed to the internal `ThreadPool::enqueue()`.
- **Overload Defense**: If all worker threads are saturated and the bounded task queue capacity is reached, Peregrine rejects excess connections immediately, preventing thread starvation and memory exhaustion.

### Phase 2: Wire Protocol Parsing
Executed inside `detail::parse_request()`:
- **Request Line**: Parses the HTTP method (`GET`, `POST`, etc.), raw target, path component, and HTTP version (`HTTP/1.1`).
- **Headers**: Decomposes header lines into a case-insensitive `std::map<string, string>`.
- **Query Parameters**: Parses URL parameters (`?key=value&filter=active`) with RFC 3986 percent-decoding into `req.query`.
- **Cookies**: Parses client `Cookie:` headers into `req.cookies`.
- **Form Data**: If `Content-Type: application/x-www-form-urlencoded`, decomposes the body into `req.form`.
- **Multipart Data**: If `Content-Type: multipart/form-data`, extracts boundaries and populates `req.form` (fields) and `req.files` (`UploadedFile` binary payloads).
- **JSON Body**: If `Content-Type: application/json`, parses the payload on-demand via `req.get_json()`.

### Phase 3: Cryptographic Session Verification
If `app.secret_key` is configured and the client supplied a session cookie:
- Decomposes the cookie string formatted as `base64_payload.timestamp.hmac_signature`.
- **Constant-Time Verification**: Recomputes the HMAC-SHA256 signature using `secret_key` and verifies it using bitwise XOR comparison (`constant_time_compare`) to prevent timing side-channel attacks.
- **Expiration Check**: Validates that `current_time - timestamp < session_lifetime_seconds`.
- **Key Rotation Fallback**: If verification fails against the primary key, automatically attempts verification against each key in `app.secret_key_fallbacks`. If a fallback key matches, marks the session as `modified` to re-sign it with the primary key upon response.
- If signature verification fails, the cookie is discarded and a fresh empty session is created.

### Phase 4: Multi-Channel CSRF Interception
If `app.enable_csrf_protection()` is active and the HTTP method is mutating (`POST`, `PUT`, `DELETE`, `PATCH`):
- Checks if the requested path matches any entry in `csrf_exempt_paths`.
- If not exempt, calls `csrf::verify_token(req)`.
- Validates token presence across 4 channels in order of precedence:
  1. Form field `csrf_token`
  2. Request header `X-CSRF-Token`
  3. Request header `X-CSRFToken`
  4. URL query parameter `?csrf_token=...`
- Compares the extracted token with `req.session["_csrf_token"]` using constant-time equality.
- If missing or invalid, halts execution immediately and returns **HTTP 400 Bad Request**.

### Phase 5: Global Pre-Request Middlewares
- Iterates over all global callbacks registered via `app.before_request([](Request& req) { ... })`.
- Middlewares can inspect `req.headers`, authenticate tokens, or store per-request state inside `req.g["user_id"]`.
- If a middleware throws an `HTTPException` (e.g. via `abort(401, "Unauthorized")`), execution halts immediately and routes to the error pipeline.

### Phase 6: URL Routing & Parameter Extraction
The router evaluates the path:
1. **Exact Match**: Tests static route paths in constant time.
2. **Regex Parameter Match**: Evaluates compiled dynamic patterns:
   - `<int:id>`: Matches `[0-9]+` and populates `req.path_params["id"]`.
   - `<string:name>`: Matches `[^/]+`.
   - `<path:filepath>`: Matches `.+` (including slashes).
3. **Method Mismatch Detection (HTTP 405)**: If the path exists but does not accept the requested HTTP verb, collects all supported verbs and returns **HTTP 405 Method Not Allowed** with an automated `Allow: GET, POST` header.
4. **404 Not Found**: If no route pattern matches, triggers the 404 error handler.

### Phase 7: Blueprint Scoped Pre-Hooks
If the matched route was registered through a `Blueprint`:
- Executes any scoped middlewares registered via `blueprint.before_request()`.
- Allows sub-applications (e.g. `/api/v1` or `/admin`) to apply isolated security checks without affecting other routes.

### Phase 8: Route Handler Execution & Exception Shield
The user-defined handler lambda `[](Request& req) -> Response` is invoked:
- The entire handler body is wrapped in a multi-level `try/catch` exception shield:
  - **`catch (const HTTPException& ex)`**: Dispatches the corresponding HTTP status code (`ex.code`) to the registered `app.error_handler(code)`.
  - **`catch (const std::exception& ex)`**: Logs the error to `std::cerr` and returns **HTTP 500 Internal Server Error**.
  - **`catch (...)`**: Catches unhandled non-standard throws and returns **HTTP 500**.

### Phase 9: Post-Request Middlewares & Header Injection
Once a `Response` object is produced:
- Executes blueprint-scoped `blueprint.after_request([](Request&, Response& res) { ... })`.
- Executes global `app.after_request([](Request&, Response& res) { ... })`.
- Middlewares can append security headers (`X-Frame-Options`, `Content-Security-Policy`), modify cookies, or log execution metrics.

### Phase 10: Session Signing & Cookie Serialization
If sessions are enabled:
- If `req.session.accessed` is true, appends `Vary: Cookie` to ensure downstream HTTP caches do not serve authenticated content to other users.
- If the session was cleared (`req.session.empty()`), deletes the cookie from the client browser (`Max-Age=0`).
- If `req.session.modified` is true:
  - Serializes session key-value pairs into compact Base64URL text.
  - Computes `HMAC-SHA256(payload + "." + timestamp, secret_key)`.
  - Appends a `Set-Cookie` header with configured flags: `HttpOnly`, `Secure`, `SameSite=Lax`, and `Max-Age`.

### Phase 11: Wire Serialization & Socket Transmission
- Formats the complete HTTP/1.1 response wire format:
  ```http
  HTTP/1.1 200 OK\r\n
  Content-Type: application/json\r\n
  Content-Length: 42\r\n
  Set-Cookie: session=...; HttpOnly; SameSite=Lax\r\n
  \r\n
  {"status":"active"}
  ```
- Writes the serialized buffer to the socket using `Connection::write()`.
- Closes the connection socket descriptor and releases all per-request stack memory.
- The worker thread returns to the pool to service the next incoming client.

---

## 5. Concurrency Architecture & Worker Thread Pool

Peregrine's concurrency is powered by [`peregrine::ThreadPool`](../include/peregrine/thread_pool.hpp):

```cpp
// Constructed during app.run() with ServerConfig parameters:
ThreadPool pool(config.thread_pool_size, config.max_queue_size);
```

### Key Concurrency Mechanics
1. **Bounded Work Queue**: The task queue has a fixed capacity (`max_queue_size`, default 64). This prevents unbounded memory growth during traffic surges.
2. **Backpressure Rejection**: If the queue is saturated, `pool.enqueue()` returns `false`. The server terminates the connection gracefully rather than accepting connections it cannot process in a timely manner.
3. **Exception Isolation**: If a worker encounters an unhandled runtime error inside a client task, the thread pool catches it, logs it, and continues executing subsequent tasks without crashing the worker thread.
4. **Graceful Shutdown**: Calling `pool.stop()` notifies all workers, drains any queued tasks, joins all worker threads, and releases system resources cleanly.

---

## 6. Memory Model & State Isolation

| Object | Lifecycle | Storage Location | Thread Safety |
| :--- | :--- | :--- | :--- |
| **`App` / `ServerConfig`** | Application Lifetime | Main Thread / Heap | Read-only during server execution |
| **`Router` / Routes** | Application Lifetime | Internal to `App` | Immutable after `app.run()` |
| **`Connection`** | Per-TCP Connection | Worker Stack | Single-thread access only |
| **`Request`** | Per-HTTP Request | Worker Stack | Stack-isolated per thread |
| **`Response`** | Per-HTTP Request | Worker Stack | Stack-isolated per thread |
| **`Session`** | Per-HTTP Request | Embedded in `Request` | Stack-isolated per thread |
| **`req.g`** | Per-HTTP Request | Embedded in `Request` | Request-scoped context storage |

Because `Request` and `Response` live exclusively on the worker thread's execution stack, Peregrine achieves **thread safety by design** with zero lock contention between concurrent HTTP requests.
