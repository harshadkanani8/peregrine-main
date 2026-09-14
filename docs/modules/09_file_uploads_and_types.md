# 📦 Module 09: File Uploads & MIME Type Engine

Peregrine includes native support for **RFC 7578 multipart/form-data** processing, enabling direct binary file uploads without external libraries. It also includes an integrated MIME-type resolution engine mapping 30+ file extensions.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [The `UploadedFile` Structure](#2-the-uploadedfile-structure)
   - [Member Variables & Inspecting Files](#member-variables--inspecting-files)
   - [Saving to Disk (`.save()`)](#saving-to-disk-save)
3. [Multipart Form Data Processing](#3-multipart-form-data-processing)
   - [Wire Parsing Mechanics](#wire-parsing-mechanics)
   - [Mixed Form Fields & File Payloads](#mixed-form-fields--file-payloads)
4. [MIME Type Resolution (`guess_mime_type`)](#4-mime-type-resolution-guess_mime_type)
   - [Supported File Extensions](#supported-file-extensions)
5. [Static File Helpers (`send_file`, `send_from_directory`)](#5-static-file-helpers-send_file-send_from_directory)
6. [Complete Code Examples](#6-complete-code-examples)

---

## 1. Architectural Overview

Defined in `include/peregrine/types.hpp` and `include/peregrine/server.hpp`.

When a client submits an HTTP `POST` or `PUT` request with `Content-Type: multipart/form-data`:
1. Peregrine extracts the unique multipart boundary delimiter.
2. The wire parser splits the body into individual multipart sections.
3. Form fields without a `filename` attribute are inserted into `req.form`.
4. File attachments with a `filename` attribute are parsed into `UploadedFile` structures and placed in `req.files`.

---

## 2. The `UploadedFile` Structure

Defined in `include/peregrine/types.hpp`.

```cpp
struct UploadedFile {
    std::string field_name;    // <input name="..."> attribute
    std::string filename;      // Original client filename (e.g. "photo.png")
    std::string content_type;  // Part MIME type (e.g. "image/png")
    std::string data;          // Raw binary payload

    // Saves the uploaded file content to disk in binary mode
    bool save(const std::string& path) const;

    // Size of the uploaded binary in bytes
    size_t size() const;

    // Returns true if no data or filename was supplied
    bool empty() const;
};
```

---

### Saving to Disk (`.save()`)

Save the file safely in binary mode:

```cpp
app.post("/upload", [](peregrine::Request& req) {
    if (req.files.empty()) {
        return peregrine::Response::text("No file uploaded", 400);
    }

    const auto& file = req.files[0];
    
    // Save to server destination path
    bool success = file.save("uploads/" + file.filename);
    if (!success) {
        return peregrine::Response::text("Failed to write file to disk", 500);
    }

    return peregrine::Response::text("Saved " + file.filename + " (" + 
                                     std::to_string(file.size()) + " bytes)");
});
```

---

## 3. Multipart Form Data Processing

### Mixed Form Fields & File Payloads

A single multipart submission can contain standard text input fields alongside file attachments:

```html
<form method="POST" action="/submit-profile" enctype="multipart/form-data">
    <input type="text" name="full_name" value="Alice">
    <input type="file" name="avatar">
    <button type="submit">Upload</button>
</form>
```

In the handler:

```cpp
app.post("/submit-profile", [](peregrine::Request& req) {
    // 1. Read standard form field
    std::string name = req.form_get("full_name");

    // 2. Read attached file
    for (const auto& file : req.files) {
        if (file.field_name == "avatar") {
            file.save("avatars/" + name + ".png");
        }
    }

    return peregrine::Response::text("Profile updated for " + name);
});
```

---

## 4. MIME Type Resolution (`guess_mime_type`)

Defined in `include/peregrine/types.hpp`.

The `peregrine::guess_mime_type(path)` function extracts the file extension and maps it to the standard IANA Media Type string:

```cpp
std::string mime = peregrine::guess_mime_type("bundle.min.js");
// mime == "application/javascript; charset=utf-8"
```

### Supported File Extensions

| Category | File Extensions | Content-Type Header |
| :--- | :--- | :--- |
| **Web Documents** | `.html`, `.htm` | `text/html; charset=utf-8` |
| **Stylesheets** | `.css` | `text/css; charset=utf-8` |
| **Scripts** | `.js`, `.mjs` | `application/javascript; charset=utf-8` |
| **Data Formats** | `.json` | `application/json` |
| **Plain Text** | `.txt` | `text/plain; charset=utf-8` |
| **Spreadsheets**| `.csv` | `text/csv; charset=utf-8` |
| **Markup** | `.xml` | `application/xml` |
| **Raster Images**| `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp` | `image/png`, `image/jpeg`, etc. |
| **Vector Images**| `.svg` | `image/svg+xml` |
| **Favicons** | `.ico` | `image/x-icon` |
| **Documents** | `.pdf` | `application/pdf` |
| **Web Fonts** | `.woff`, `.woff2`, `.ttf`, `.otf` | `font/woff`, `font/woff2`, etc. |
| **Video Streams**| `.mp4`, `.webm` | `video/mp4`, `video/webm` |
| **Audio Streams**| `.mp3`, `.wav`, `.ogg` | `audio/mpeg`, `audio/wav`, etc. |
| **WebAssembly** | `.wasm` | `application/wasm` |
| **Archives** | `.zip`, `.tar`, `.gz` | `application/zip`, `application/gzip` |
| *Fallback* | Unrecognized extension | `application/octet-stream` |

---

## 5. Static File Helpers (`send_file`, `send_from_directory`)

Defined in `include/peregrine/helpers.hpp`.

Deliver files securely to clients with automatic Content-Type headers and Path Traversal protection:

```cpp
// 1. Send specific file directly
app.get("/download/manual", [](peregrine::Request& req) {
    return peregrine::send_file("docs/manual.pdf", "application/pdf");
});

// 2. Safely serve file from a base folder
app.get("/assets/<path:filepath>", [](peregrine::Request& req) {
    std::string rel_path = req.path_params["filepath"];
    // Automatically checks for ".." traversal attacks
    return peregrine::send_from_directory("public/assets", rel_path);
});
```

---

## 6. Complete Code Examples

### Secure Multi-File Upload Service

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    // Configure maximum payload size (e.g. 32 MB)
    peregrine::ServerConfig cfg;
    cfg.max_body_size = 32 * 1024 * 1024;
    cfg.port = 8080;

    // Render file upload UI
    app.get("/", [](peregrine::Request& req) {
        std::string html =
            "<!DOCTYPE html><html><body>"
            "<h2>Upload Document</h2>"
            "<form action='/upload' method='POST' enctype='multipart/form-data'>"
            "  <input type='text' name='description' placeholder='Document Description'/><br/><br/>"
            "  <input type='file' name='doc' multiple/><br/><br/>"
            "  <button type='submit'>Upload File</button>"
            "</form>"
            "</body></html>";
        return peregrine::Response::html(html);
    });

    // Handle uploaded files
    app.post("/upload", [](peregrine::Request& req) {
        std::string desc = req.form_get("description", "No description provided");

        if (req.files.empty()) {
            return peregrine::Response::text("Error: No files selected.", 400);
        }

        peregrine::Json result = peregrine::Json::object();
        result["description"] = desc;

        peregrine::Json file_list = peregrine::Json::array();
        for (const auto& file : req.files) {
            // Prevent dangerous filenames
            if (file.filename.find("..") != std::string::npos) {
                continue;
            }

            std::string dest = "uploads/" + file.filename;
            if (file.save(dest)) {
                peregrine::Json item = peregrine::Json::object();
                item["name"] = file.filename;
                item["content_type"] = file.content_type;
                item["size_bytes"] = static_cast<int>(file.size());
                file_list.push_back(item);
            }
        }

        result["uploaded_files"] = file_list;
        return peregrine::Response::json(result, 201);
    });

    app.run(cfg);
    return 0;
}
```
