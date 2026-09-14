 /*
 * =========================================================================
 *     ____                              _             __ __ 
 *    /  __\___  ________  ____ ________(_)_  _____   / // / 
 *   / /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \/ -_) / // /_ 
 *  / .___/\__//_/  \__/\_, /_/  /_/_/_/_//_/\__/ /__  __/ 
 * /_/                 /___/                        /_/    
 *
 *  Peregrine++ Web Application Framework
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

 // ============================================================================
// peregrine/types.hpp
//
// Core server types, protocol options, file upload structures, and MIME type helpers.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "common.hpp"

namespace peregrine {

// ============================================================================
// Server Protocols & Configuration
// ============================================================================

enum class Protocol { HTTP, HTTPS };

struct ServerConfig {
    Protocol protocol = Protocol::HTTP;
    std::string host = "0.0.0.0";
    int port = 5000;
    int backlog = 128;

    // SSL / TLS configuration (required for HTTPS)
    std::string cert_file;
    std::string key_file;
    std::string tls_version = "TLS_1_2"; // "TLS_1_2" or "TLS_1_3"

    // Concurrency / performance options
    size_t max_body_size = 16 * 1024 * 1024;  // 16 MB default
    size_t thread_pool_size = 8;              // 8 worker threads default
    size_t max_queue_size = 64;               // Maximum queued pending requests
    int socket_timeout_seconds = 10;          // Socket read/write timeout in seconds
};

// ============================================================================
// File Upload Structure (multipart/form-data)
// ============================================================================

struct UploadedFile {
    std::string field_name;
    std::string filename;
    std::string content_type;
    std::string data;

    // Saves the uploaded file content to disk
    bool save(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        return out.good();
    }

    size_t size() const { return data.size(); }
    bool empty() const { return data.empty() && filename.empty(); }
};

// ============================================================================
// MIME Types
// ============================================================================

inline std::string guess_mime_type(const std::string& path) {
    static const std::map<std::string, std::string> types = {
        {".html",  "text/html; charset=utf-8"},
        {".htm",   "text/html; charset=utf-8"},
        {".css",   "text/css; charset=utf-8"},
        {".js",    "application/javascript; charset=utf-8"},
        {".mjs",   "application/javascript; charset=utf-8"},
        {".json",  "application/json"},
        {".txt",   "text/plain; charset=utf-8"},
        {".csv",   "text/csv; charset=utf-8"},
        {".xml",   "application/xml"},
        {".png",   "image/png"},
        {".jpg",   "image/jpeg"},
        {".jpeg",  "image/jpeg"},
        {".gif",   "image/gif"},
        {".svg",   "image/svg+xml"},
        {".ico",   "image/x-icon"},
        {".webp",  "image/webp"},
        {".pdf",   "application/pdf"},
        {".woff",  "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf",   "font/ttf"},
        {".otf",   "font/otf"},
        {".mp4",   "video/mp4"},
        {".webm",  "video/webm"},
        {".mp3",   "audio/mpeg"},
        {".wav",   "audio/wav"},
        {".ogg",   "audio/ogg"},
        {".wasm",  "application/wasm"},
        {".zip",   "application/zip"},
        {".tar",   "application/x-tar"},
        {".gz",    "application/gzip"}
    };

    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = to_lower(path.substr(dot));
    auto it = types.find(ext);
    return (it != types.end()) ? it->second : "application/octet-stream";
}

}  // namespace peregrine
