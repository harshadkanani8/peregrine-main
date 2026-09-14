// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "peregrine/peregrine.hpp"
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <direct.h>
#define mkdir_compat(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#define mkdir_compat(dir) mkdir(dir, 0755)
#endif

using namespace peregrine;

// Sanitizes uploaded filenames to prevent Path Traversal attacks (CWE-22)
std::string sanitize_filename(const std::string& input) {
    std::string clean;
    for (char c : input) {
        if (c == '/' || c == '\\' || c == ':' || c == '\0') {
            clean += '_';
        } else {
            clean += c;
        }
    }
    // Remove leading dots or slashes
    while (!clean.empty() && (clean.front() == '.' || clean.front() == ' ')) {
        clean.erase(clean.begin());
    }
    return clean.empty() ? "uploaded_file.bin" : clean;
}

int main() {
    App app("file_upload_app");

    // Ensure upload storage directory exists
    mkdir_compat("uploads");

    // ------------------------------------------------------------------------
    // GET / - Upload Form Page
    // ------------------------------------------------------------------------
    app.get("/", [](Request&) {
        std::string html =
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Peregrine File Upload</title></head>"
            "<body style='font-family: sans-serif; max-width: 600px; margin: 50px auto; line-height: 1.6;'>"
            "  <h1>🦅 Peregrine File Upload Demo</h1>"
            "  <p>Select a file to upload using standard <code>multipart/form-data</code>.</p>"
            "  <form method='POST' action='/upload' enctype='multipart/form-data' style='border: 2px dashed #94a3b8; padding: 24px; border-radius: 8px; text-align: center;'>"
            "    <input type='file' name='file' style='margin-bottom: 16px;' required /><br/>"
            "    <button type='submit' style='background: #0284c7; color: white; border: none; padding: 10px 20px; border-radius: 6px; font-weight: bold; cursor: pointer;'>Upload File</button>"
            "  </form>"
            "</body>"
            "</html>";
        return Response::html(html);
    });

    // ------------------------------------------------------------------------
    // POST /upload - Process Multipart Upload
    // ------------------------------------------------------------------------
    app.post("/upload", [](Request& req) {
        if (req.files.empty()) {
            Json err = Json::object();
            err["status"] = "error";
            err["message"] = "No files found in request. Ensure form uses enctype='multipart/form-data'.";
            return Response::json(err, 400);
        }

        Json uploaded_files = Json::array();

        for (const auto& file : req.files) {
            std::string safe_name = sanitize_filename(file.filename);
            std::string destination = "uploads/" + safe_name;

            // Persist binary payload to disk
            bool saved = file.save(destination);

            Json file_info = Json::object();
            file_info["original_name"] = file.filename;
            file_info["safe_name"] = safe_name;
            file_info["content_type"] = file.content_type;
            file_info["size_bytes"] = static_cast<double>(file.size());
            file_info["saved_to"] = destination;
            file_info["success"] = saved;

            uploaded_files.push_back(file_info);
        }

        Json response = Json::object();
        response["status"] = "success";
        response["count"] = static_cast<double>(req.files.size());
        response["files"] = uploaded_files;

        return Response::json(response, 200);
    });

    // Start server
    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "==================================================" << std::endl;
    std::cout << "  Peregrine File Upload Server Running" << std::endl;
    std::cout << "  Listening on: http://" << host << ":" << port << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Open browser: http://" << host << ":" << port << "/" << std::endl;

    app.run(host, port);
    return 0;
}
