# 📦 Module 08: Template Engine & View Rendering

Peregrine includes a lightweight, built-in **Mustache-style template engine** designed for server-side HTML rendering without external dependencies. It supports scalar variable substitutions, collection iterations, and both disk-based and inline string rendering.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [The `TemplateContext` Builder](#2-the-templatecontext-builder)
   - [Scalar Variables (`.set()`)](#scalar-variables-set)
   - [Collection Loops (`.append()`)](#collection-loops-append)
3. [Template Syntax Guide](#3-template-syntax-guide)
   - [Variable Substitution (`{{var}}`)](#variable-substitution-var)
   - [Loop Iterations (`{{#each list}} ... {{/each}}`)](#loop-iterations-each-list---each)
   - [Nested Field Resolution (`{{this.field}}`)](#nested-field-resolution-thisfield)
4. [Rendering Functions](#4-rendering-functions)
   - [Inline Rendering (`render_string`)](#inline-rendering-render_string)
   - [File-Based Rendering (`render_template`)](#file-based-rendering-render_template)
5. [Complete Code Examples](#5-complete-code-examples)

---

## 1. Architectural Overview

Defined in `include/peregrine/template.hpp`.

Server-side HTML rendering in Peregrine operates on a zero-compilation, stream-based substitution algorithm. Templates are processed at request time using a `TemplateContext` dictionary:

```text
 Template File / String
           │
           ├─► [ Parser identifies {{ tags ]
           │
     ┌─────┴────────────────────────┐
     ▼                              ▼
 Scalar Tag: {{user}}         Loop Tag: {{#each items}}
 Lookup in ctx.vars           Iterate ctx.lists[items]
 Substitute string value      Expand {{this.field}} per item
     │                              │
     └─────────────┬────────────────┘
                   ▼
           Rendered HTML Stream
```

---

## 2. The `TemplateContext` Builder

The `peregrine::TemplateContext` struct encapsulates all data exposed to the template.

### Scalar Variables (`.set()`)

Use `.set()` to register string key-value pairs:

```cpp
peregrine::TemplateContext ctx;

ctx.set("page_title", "User Profile")
   .set("username", "Alice")
   .set("role", "Administrator");
```

---

### Collection Loops (`.append()`)

Use `.append()` to push dictionaries into a named collection:

```cpp
ctx.append("products", {
    {"id", "101"},
    {"name", "Mechanical Keyboard"},
    {"price", "$129.99"}
});

ctx.append("products", {
    {"id", "102"},
    {"name", "Ergonomic Mouse"},
    {"price", "$79.99"}
});
```

---

## 3. Template Syntax Guide

### Variable Substitution (`{{var}}`)

Any tag enclosed in double braces `{{ ... }}` is replaced with the corresponding scalar variable from `ctx.vars`:

```html
<h1>Welcome, {{username}}!</h1>
<p>Your current role is: {{role}}</p>
```

*Note: If a variable is not found in the context, it evaluates cleanly to an empty string without throwing errors.*

---

### Loop Iterations (`{{#each list}} ... {{/each}}`)

To iterate over a collection of items, wrap the HTML markup inside `{{#each <list_name>}}` and `{{/each}}`:

```html
<ul>
  {{#each products}}
    <li><strong>{{this.name}}</strong> — {{this.price}} (SKU: {{this.id}})</li>
  {{/each}}
</ul>
```

---

### Nested Field Resolution (`{{this.field}}`)

Within a loop body, the special prefix `this.` accesses fields belonging to the current item being iterated.

If the collection is empty, the entire block between `{{#each}}` and `{{/each}}` is omitted from the rendered output.

---

## 4. Rendering Functions

### Inline Rendering (`render_string`)

Render templates directly from C++ strings or raw string literals:

```cpp
std::string tmpl = "Hello, {{name}}! You have {{unread_count}} notifications.";

peregrine::TemplateContext ctx;
ctx.set("name", "Alice")
   .set("unread_count", "5");

std::string output = peregrine::render_string(tmpl, ctx);
// Result: "Hello, Alice! You have 5 notifications."
```

---

### File-Based Rendering (`render_template`)

Load and render an external template file stored in the filesystem:

```cpp
app.get("/dashboard", [](peregrine::Request& req) {
    peregrine::TemplateContext ctx;
    ctx.set("title", "Operations Dashboard")
       .set("server_time", "2026-09-14 12:00:00");

    // Renders templates/dashboard.html
    std::string html = peregrine::render_template("templates/dashboard.html", ctx);
    return peregrine::Response::html(html);
});
```

If the file cannot be opened, `render_template` returns a descriptive HTML comment (`<!-- Template Error: File not found '...' -->`) without terminating the server.

---

## 5. Complete Code Examples

### Dynamic Product Catalog Server

#### `templates/catalog.html`:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>{{title}}</title>
    <style>
        body { font-family: sans-serif; margin: 40px; }
        .product-card { border: 1px solid #ccc; padding: 16px; margin-bottom: 12px; border-radius: 8px; }
    </style>
</head>
<body>
    <h1>{{title}}</h1>
    <p>Logged in as: <strong>{{current_user}}</strong></p>
    <hr/>

    <h2>Available Inventory</h2>
    {{#each products}}
    <div class="product-card">
        <h3>{{this.name}}</h3>
        <p>Category: {{this.category}}</p>
        <p>Price: <strong>{{this.price}}</strong></p>
    </div>
    {{/each}}
</body>
</html>
```

#### C++ Server (`main.cpp`):
```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    app.get("/catalog", [](peregrine::Request& req) {
        peregrine::TemplateContext ctx;
        ctx.set("title", "Hardware Catalog")
           .set("current_user", "Alice");

        ctx.append("products", {
            {"name", "NVMe Solid State Drive 2TB"},
            {"category", "Storage"},
            {"price", "$189.00"}
        });

        ctx.append("products", {
            {"name", "DDR5 32GB Memory Kit"},
            {"category", "RAM"},
            {"price", "$119.50"}
        });

        std::string html = peregrine::render_template("templates/catalog.html", ctx);
        return peregrine::Response::html(html);
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```
