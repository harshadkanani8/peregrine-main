/* ==========================================================================
   Peregrine Documentation Search Index
   Instant in-browser search dataset covering all framework classes & APIs
   ========================================================================== */

const PEREGRINE_SEARCH_INDEX = [
  {
    id: "overview",
    title: "Overview & Framework Philosophy",
    category: "Getting Started",
    snippet: "Lightweight, header-only C++11 web framework engineered for zero-dependency performance and embedded systems.",
    keywords: ["c++11", "header-only", "performance", "embedded", "overview", "intro", "philosophy"]
  },
  {
    id: "quickstart",
    title: "Quickstart & Hello World",
    category: "Getting Started",
    snippet: "Create and run your first native C++ web service with Peregrine in under 5 minutes.",
    keywords: ["quickstart", "hello world", "app.get", "app.run", "starter", "compile", "curl"]
  },
  {
    id: "download",
    title: "Download & Installation Options",
    category: "Getting Started",
    snippet: "Download Peregrine via Git clone, CMake FetchContent, source ZIP archive, or offline PDF documentation.",
    keywords: ["download", "github", "clone", "install", "cmake", "fetchcontent", "zip", "tar", "pdf", "release", "git"]
  },
  {
    id: "architecture",
    title: "Request-Response Lifecycle Architecture",
    category: "Getting Started",
    snippet: "End-to-end 11-step execution pipeline from TCP socket acceptance to worker thread wire serialization.",
    keywords: ["lifecycle", "pipeline", "thread pool", "concurrency", "socket", "dispatch", "architecture", "flow"]
  },
  {
    id: "module-app",
    title: "Module 01: Application & Server Engine",
    category: "Module Reference",
    snippet: "peregrine::App coordinator, ServerConfig parameters, HTTPS/TLS 1.3 setup, and static directory mapping.",
    keywords: ["app", "server", "serverconfig", "https", "tls", "ssl", "port", "host", "config", "socket_timeout_seconds"]
  },
  {
    id: "module-config",
    title: "Module 01b: Configuration Engine (peregrine::Config)",
    category: "Module Reference",
    snippet: "Dictionary-like configuration object with from_env environment variable loading and type-safe accessors.",
    keywords: ["config", "from_env", "get_int", "get_bool", "settings", "environment", "secret_key"]
  },
  {
    id: "module-routing",
    title: "Module 02: URL Routing & Class-Based Views",
    category: "Module Reference",
    snippet: "Dynamic regex parameter extraction (<int:id>, <string:slug>, <path:filepath>), MethodView, and automatic HTTP 405.",
    keywords: ["routing", "router", "parameters", "regex", "path", "methodview", "get", "post", "put", "delete", "405", "allow"]
  },
  {
    id: "module-blueprints",
    title: "Module 03: Blueprints & Middleware",
    category: "Module Reference",
    snippet: "Modularizing large applications into prefix domains with scoped before_request and after_request hooks.",
    keywords: ["blueprint", "middleware", "prefix", "before_request", "after_request", "req.g", "hooks", "error_handler"]
  },
  {
    id: "module-reqres",
    title: "Module 04: Request & Response Objects",
    category: "Module Reference",
    snippet: "Inspecting query strings, headers, form data, cookies, JSON payloads, and response factories with streaming & MJPEG.",
    keywords: ["request", "response", "headers", "query", "cookies", "stream", "mjpeg", "abort", "status", "set_cookie", "delete_cookie"]
  },
  {
    id: "module-helpers",
    title: "Module 04b: Helpers, Exceptions & Common Utilities",
    category: "Module Reference",
    snippet: "abort(), HTTPException, jsonify, send_file, send_from_directory, URL parsing, Base64, and HMAC utilities.",
    keywords: ["helpers", "abort", "httpexception", "send_file", "send_from_directory", "url_encode", "url_decode", "base64", "hmac_sha256", "constant_time_equal"]
  },
  {
    id: "module-json",
    title: "Module 05: PIMPL JSON Engine",
    category: "Module Reference",
    snippet: "Header-only recursive-descent JSON parser with strict 4-byte/8-byte PIMPL design for 32-bit ARM ABI safety.",
    keywords: ["json", "pimpl", "parse", "dump", "arm", "abi", "serialization", "object", "array", "as_string", "as_int", "as_number"]
  },
  {
    id: "module-sessions",
    title: "Module 06: Stateless Sessions & Cookies",
    category: "Module Reference",
    snippet: "Client-side HMAC-SHA256 signed session cookies, zero-downtime key rotation, and automatic Vary: Cookie injection.",
    keywords: ["session", "cookies", "hmac", "sha256", "key rotation", "secret_key", "secret_key_fallbacks", "httponly", "samesite", "vary"]
  },
  {
    id: "module-security",
    title: "Module 07: Security & CSRF Defense",
    category: "Module Reference",
    snippet: "Multi-vector CSRF protection, constant-time XOR comparison, CWE-22 Path Traversal defense, and Slowloris mitigation.",
    keywords: ["csrf", "security", "path traversal", "cwe-22", "slowloris", "protection", "token", "verify_token", "get_token"]
  },
  {
    id: "module-template",
    title: "Module 08: Mustache Template Engine",
    category: "Module Reference",
    snippet: "Server-side HTML rendering with variable substitution {{var}} and collection iterations {{#each list}}.",
    keywords: ["template", "mustache", "render", "html", "views", "loops", "context", "templatecontext", "render_template", "render_string"]
  },
  {
    id: "module-uploads",
    title: "Module 09: File Uploads & MIME Engine",
    category: "Module Reference",
    snippet: "RFC 7578 multipart/form-data processor, UploadedFile disk persistence, and 30+ MIME type resolver.",
    keywords: ["upload", "multipart", "form-data", "file", "mime", "content-type", "save", "guess_mime_type", "uploadedfile"]
  },
  {
    id: "module-concurrency",
    title: "Module 10: Concurrency & Thread Pool",
    category: "Module Reference",
    snippet: "Standard C++11 ThreadPool with bounded task queues, load rejection backpressure (HTTP 503), and exception shielding.",
    keywords: ["threadpool", "threads", "queue", "503", "backpressure", "concurrency", "mutex", "active_workers", "thread_count"]
  },
  {
    id: "guide-rest",
    title: "Feature Guide: Production RESTful JSON APIs",
    category: "Guides",
    snippet: "Designing scalable microservices with CRUD endpoints, query pagination, error models, and mutex synchronization.",
    keywords: ["rest", "api", "crud", "pagination", "json", "microservice", "guide", "mutex", "lock_guard"]
  },
  {
    id: "guide-auth",
    title: "Feature Guide: User Authentication & RBAC",
    category: "Guides",
    snippet: "Implementing secure user login, signed session cookies, protected route guards, and role-based access control.",
    keywords: ["auth", "authentication", "login", "rbac", "permissions", "roles", "logout", "before_request"]
  },
  {
    id: "guide-spa",
    title: "Feature Guide: Single-Page Application (SPA) Hosting",
    category: "Guides",
    snippet: "Hosting React, Vue, Vite, and Angular applications with zero-config HTML5 client-side history fallback.",
    keywords: ["spa", "react", "vue", "vite", "frontend", "static", "index.html", "routing", "serve_spa"]
  },
  {
    id: "examples-hub",
    title: "Runnable Project Examples & Build Hub",
    category: "Project Examples",
    snippet: "Five complete, self-contained native C++ web applications: Hello World, REST API, Auth & Sessions, Templates, and File Uploads.",
    keywords: ["examples", "samples", "demos", "tutorial", "cmake", "projects", "build", "run", "gcc", "clang", "min_gw"]
  },
  {
    id: "example-hello",
    title: "Example 01: Hello World & Dynamic Routing",
    category: "Project Examples",
    snippet: "Minimal HTTP server, text & HTML endpoints, dynamic path parameters (<string:name>), and curl verification.",
    keywords: ["hello world", "example 01", "get", "path_params", "starter", "minimal", "curl"]
  },
  {
    id: "example-rest",
    title: "Example 02: RESTful Task API (CRUD & Blueprints)",
    category: "Project Examples",
    snippet: "Production JSON microservice with Blueprint routing (/api/v1), thread-safe mutex storage, and full CRUD verbs.",
    keywords: ["rest", "api", "example 02", "crud", "blueprint", "tasks", "mutex", "lock_guard", "post", "put", "delete", "json"]
  },
  {
    id: "example-auth",
    title: "Example 03: Authentication & Cookie Sessions",
    category: "Project Examples",
    snippet: "Stateless HMAC-SHA256 client-side signed cookies, protected route guards, login/logout, and session debug JSON.",
    keywords: ["auth", "authentication", "example 03", "sessions", "hmac", "sha256", "cookies", "login", "logout", "tamper", "dashboard"]
  },
  {
    id: "example-templates",
    title: "Example 04: Mustache Template Rendering",
    category: "Project Examples",
    snippet: "Dynamic HTML catalog using single-pass Mustache template engine, TemplateContext, scalar tags, and {{#each}} loops.",
    keywords: ["templates", "mustache", "example 04", "render_template", "render_string", "html", "each", "loops", "catalog"]
  },
  {
    id: "example-uploads",
    title: "Example 05: Multipart File Uploads & CWE-22 Defense",
    category: "Project Examples",
    snippet: "RFC 7578 multipart/form-data upload processor, CWE-22 Path Traversal defense, binary disk persistence, and JSON reporting.",
    keywords: ["uploads", "multipart", "example 05", "form-data", "file.save", "cwe-22", "path traversal", "sanitize_filename", "binary"]
  },
  {
    id: "roadmap",
    title: "Architectural Roadmap & Framework Benchmarks",
    category: "Reference",
    snippet: "Technical gap analysis vs. Crow, Drogon, Oat++ and prioritized Tier 1/2/3 milestones (Keep-Alive, WebSockets, HTTP/2).",
    keywords: ["roadmap", "benchmark", "crow", "drogon", "oat++", "comparison", "performance", "future"]
  },
  {
    id: "api-reference",
    title: "Master API Reference & Method Index",
    category: "Reference",
    snippet: "Complete technical reference of all classes, structs, methods, signatures, parameters, and return types.",
    keywords: ["api", "reference", "index", "classes", "methods", "signatures", "functions", "structs"]
  }
];
