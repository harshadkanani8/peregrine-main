# 🌐 Feature Guide: Single-Page Application (SPA) Frontend Hosting

This guide explains how to host modern client-side frontend applications (built with **React, Vue, Vite, Angular, or Svelte**) alongside your Peregrine C++ backend using the built-in, zero-configuration `serve_spa()` engine.

---

## 📋 Table of Contents

1. [The SPA Routing Challenge (HTML5 History Mode)](#1-the-spa-routing-challenge-html5-history-mode)
2. [How Peregrine's `serve_spa` Resolves Routes](#2-how-peregrines-serve_spa-resolves-routes)
3. [Building Your Frontend Application](#3-building-your-frontend-application)
4. [Configuring Peregrine for SPA Hosting](#4-configuring-peregrine-for-spa-hosting)
5. [Co-Locating Backend APIs & Frontend SPAs](#5-co-locating-backend-apis--frontend-spas)
6. [HTTP Cache Control Best Practices](#6-http-cache-control-best-practices)
7. [Complete Runnable Example](#7-complete-runnable-example)

---

## 1. The SPA Routing Challenge (HTML5 History Mode)

Modern Single-Page Applications (SPAs) use the browser's HTML5 History API (`pushState`) to simulate multi-page navigation without reloading the browser window.

For example, navigating inside a React or Vue application changes the browser URL to:
```text
https://example.com/dashboard/analytics
```
* If a user clicks a link **inside** the application, JavaScript renders the page without contacting the server.
* However, if the user presses **F5 (Reload)** or bookmarks `https://example.com/dashboard/analytics`, the browser asks the web server for `/dashboard/analytics`.
* In traditional web servers, this returns a `404 Not Found` error because no physical file named `dashboard/analytics` exists on disk.

To solve this, the server must **fall back to serving `index.html`** for all non-file requests, allowing the client-side router to boot up and render the route.

---

## 2. How Peregrine's `serve_spa` Resolves Routes

Peregrine includes built-in SPA resolution logic in `app.serve_spa()`:

```text
 Client Request: GET /dashboard/settings
                      │
                      ▼
         [ Is it an API route? ] ──(Yes)──► Execute API Route Handler (or 404 JSON)
                      │
                     (No)
                      ▼
        [ Does file exist on disk? ] ──(Yes)──► Serve physical file (CSS, JS, PNG)
        (e.g. dist/assets/app.js)
                      │
                     (No)
                      ▼
        [ Is it a system probe? ] ──(Yes)──► Return 404 (Reject .git, .well-known)
                      │
                     (No)
                      ▼
          [ Serve dist/index.html ] ──► Status 200 (Client-side router renders view)
```

---

## 3. Building Your Frontend Application

In your frontend project (e.g. Vite + React):

```bash
# 1. In your frontend repository
npm run build
```

This compiles your application into a production distribution directory:
```text
dist/
├── index.html
├── favicon.ico
└── assets/
    ├── index-a4f12b.js
    └── index-98c11e.css
```

---

## 4. Configuring Peregrine for SPA Hosting

In your C++ Peregrine application, point `serve_spa()` to the built directory:

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    // Point to the built assets folder
    app.serve_spa("dist", "index.html", "api");

    app.run("127.0.0.1", 8080);
    return 0;
}
```

### Parameter Reference:
1. `dist_directory`: Physical directory where built assets reside (default: `"dist"`).
2. `index_file`: Name of the entrypoint HTML file (default: `"index.html"`).
3. `api_prefix`: URL prefix reserved for backend endpoints (default: `"api"`). Requests starting with `/api` will **never** fallback to `index.html`, returning clean `404 Not Found` JSON errors instead.

---

## 5. Co-Locating Backend APIs & Frontend SPAs

By reserving an API prefix (e.g. `/api`), you can host high-performance native C++ JSON endpoints on the exact same port and domain as your frontend UI:

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    // 1. Backend REST API Endpoints
    app.get("/api/v1/system-info", [](peregrine::Request& req) {
        peregrine::Json j = peregrine::Json::object();
        j["os"] = "Linux x86_64";
        j["framework"] = "Peregrine C++";
        return peregrine::Response::json(j);
    });

    // 2. Host the frontend SPA for everything else
    app.serve_spa("dist", "index.html", "api");

    app.run("0.0.0.0", 8080);
    return 0;
}
```

#### Client Fetch Call (in React / Vue):
Because the backend and frontend share the same origin, your frontend code requires no CORS configuration and can use relative URLs:

```javascript
// React component fetch
const response = await fetch('/api/v1/system-info');
const data = await response.json();
```

---

## 6. HTTP Cache Control Best Practices

Vite and modern bundlers generate content-hashed filenames for scripts and stylesheets (e.g. `index-a4f12b.js`).

Use an `after_request` hook to inject optimized caching directives:

```cpp
app.after_request([](peregrine::Request& req, peregrine::Response& res) {
    // 1. Immutable caching for hashed static assets
    if (peregrine::starts_with(req.path, "/assets/")) {
        res.set_header("Cache-Control", "public, max-age=31536000, immutable");
    }
    // 2. Zero caching for index.html to ensure clients receive immediate updates
    else if (req.path == "/" || res.headers["Content-Type"].find("text/html") != std::string::npos) {
        res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
        res.set_header("Pragma", "no-cache");
    }
});
```

---

## 7. Complete Runnable Example

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app("single_binary_app");

    // ------------------------------------------------------------------------
    // JSON Backend Services
    // ------------------------------------------------------------------------
    peregrine::Blueprint api_bp("api", "/api");

    api_bp.get("/metrics", [](peregrine::Request& req) {
        peregrine::Json j = peregrine::Json::object();
        j["cpu_usage"] = 4.2;
        j["memory_mb"] = 128.5;
        j["status"] = "online";
        return peregrine::Response::json(j);
    });

    app.register_blueprint(api_bp);

    // ------------------------------------------------------------------------
    // Frontend SPA Engine
    // ------------------------------------------------------------------------
    // Serves physical files from "./dist".
    // Any unmatched non-API path (e.g. /profile, /settings) serves dist/index.html
    app.serve_spa("./dist", "index.html", "api");

    std::cout << "Application online at http://127.0.0.1:8080\n";
    std::cout << "  Frontend UI: http://127.0.0.1:8080/\n";
    std::cout << "  Backend API: http://127.0.0.1:8080/api/metrics\n";

    app.run("127.0.0.1", 8080);
    return 0;
}
```
