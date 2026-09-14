# 📚 Peregrine Documentation Portal

Welcome to the **Peregrine C++ Web Framework** documentation hub. 

This documentation is designed for software engineers, systems programmers, and web developers building high-performance, native C++ web services and applications.

---

## 🗺️ Documentation Map

The documentation is organized into three distinct perspectives:

```text
docs/
├── README.md                           # Documentation Hub & Navigation Portal (This File)
├── architecture_and_lifecycle.md       # Flow-Wise: Complete Request-Response Lifecycle & Threading
├── user_guide.md                       # High-level Getting Started & Core Concepts
├── api_reference.md                    # Technical Reference for Classes, Methods & Structs
├── roadmap.md                          # Architectural Analysis & Technical Enhancements
│
├── modules/                            # Module-Wise Guides (Deep-Dive into Each Header)
│   ├── 01_app_and_server.md            # App class, ServerConfig, TLS/HTTPS, SPA serving
│   ├── 02_routing_and_views.md         # Regex dynamic routing, path params, MethodView, 405
│   ├── 03_blueprints_and_middleware.md # Modular Blueprints, prefixes, scoped before/after hooks
│   ├── 04_request_and_response.md      # Request parsing, Response builders, streaming, MJPEG
│   ├── 05_json_engine.md               # PIMPL JSON AST, parsing, serialization, escape rules
│   ├── 06_sessions_and_cookies.md      # Stateless HMAC-SHA256 signed cookies & key rotation
│   ├── 07_security_and_csrf.md         # CSRF multi-vector defense, CWE-22 Path Traversal
│   ├── 08_templating_engine.md         # Mustache template engine, scalar {{var}}, {{#each}} loops
│   ├── 09_file_uploads_and_types.md    # UploadedFile, multipart/form-data, MIME resolution
│   └── 10_concurrency_and_threading.md # ThreadPool, bounded task queues, load rejection
│
└── feature_guides/                     # Feature-Wise Guides (Real-World Recipes)
    ├── rest_api_guide.md               # Building production RESTful JSON microservices
    ├── authentication_guide.md         # User login, protected dashboards, role-based access
    └── spa_frontend_guide.md           # Zero-config React / Vue / Vite Single Page App hosting
```

---

## 🚀 Quick Navigation by Topic

### 1. Getting Started & Core Concepts
- [**User Guide**](user_guide.md): Step-by-step introduction to installing, configuring, and writing your first Peregrine application.
- [**API Reference**](api_reference.md): Complete technical reference of classes, methods, signatures, and configuration flags.

### 2. Architecture & Execution Flow (Flow-Wise)
- [**Request-Response Lifecycle Flow**](architecture_and_lifecycle.md): Detailed architectural sequence diagram tracing an incoming TCP packet through worker thread allocation, wire parsing, session validation, middleware execution, routing, handler execution, and wire response serialization.

### 3. Module Deep Dives (Module-Wise)
- [**01. Application & Server Coordinator**](modules/01_app_and_server.md): Lifecycle management, configuration, HTTP/HTTPS listener, and static asset mapping.
- [**02. URL Routing & Class-Based Views**](modules/02_routing_and_views.md): Dynamic regex parameter extraction (`<int:id>`, `<string:slug>`, `<path:file>`), HTTP 405 Method Not Allowed handling, and `MethodView` dispatching.
- [**03. Blueprints & Middlewares**](modules/03_blueprints_and_middleware.md): Modularizing large applications into isolated route prefixes with scoped `before_request` and `after_request` hooks.
- [**04. Request & Response Objects**](modules/04_request_and_response.md): HTTP query string parsing, form fields, headers, cookies, context storage (`req.g`), streaming, and live MJPEG video.
- [**05. JSON Engine**](modules/05_json_engine.md): Header-only recursive-descent JSON parser, AST manipulation, and serialization.
- [**06. Stateless Sessions & Cookies**](modules/06_sessions_and_cookies.md): Client-side HMAC-SHA256 signed session cookies, tamper rejection, and cryptographic key rotation.
- [**07. Security & CSRF Defense**](modules/07_security_and_csrf.md): Multi-vector CSRF protection, constant-time XOR comparison, and Path Traversal defense (CWE-22).
- [**08. Template Engine**](modules/08_templating_engine.md): Mustache variable substitution (`{{var}}`), collection loops (`{{#each}}`), and disk/inline rendering.
- [**09. File Uploads & Type Mapping**](modules/09_file_uploads_and_types.md): RFC 7578 multipart/form-data parsing, binary disk persistence, and MIME type resolution across 30+ extensions.
- [**10. Concurrency & Thread Pool**](modules/10_concurrency_and_threading.md): Worker threads, bounded task queues, and backpressure load rejection.

### 4. Real-World Recipes (Feature-Wise)
- [**REST API Guide**](feature_guides/rest_api_guide.md): Building scalable JSON microservices with proper HTTP verbs, status codes, and error models.
- [**Authentication Guide**](feature_guides/authentication_guide.md): Implementing stateful user authentication, secure cookie sessions, and role-based access control (RBAC).
- [**SPA Frontend Serving**](feature_guides/spa_frontend_guide.md): Hosting modern single-page frontend apps (React, Vue, Vite) with HTML5 client-side routing fallback.

---

## 🧪 Testing & Examples
- [**Testing Guide & In-Memory Client**](../tests/README.md): How to test Peregrine applications in memory without network sockets using `TestClient`.
- [**Application Examples Directory**](../examples/README.md): 5 runnable sample projects with dedicated documentation and CMake build targets.
- [**Architectural Roadmap**](roadmap.md): Technical gap analysis, planned performance enhancements, and future milestones.
