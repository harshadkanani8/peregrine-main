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
 * =========================================================================
 */

// ============================================================================
// peregrine/helpers.hpp
//
// Helper functions: jsonify, make_response, redirect, abort, send_file,
// and send_from_directory with strict path traversal protection.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "common.hpp"
#include "json.hpp"
#include "response.hpp"
#include "types.hpp"

namespace peregrine {

// ----------------------------------------------------------------------------
// Response Builders
// ----------------------------------------------------------------------------

inline Response make_response(const std::string& body = "", int status = 200) {
    return Response::text(body, status);
}

inline Response make_response(const Json& j, int status = 200) {
    return Response::json(j, status);
}

inline Response jsonify(const Json& j, int status = 200) {
    return Response::json(j, status);
}

inline Response redirect(const std::string& location, int status = 302) {
    return Response::redirect(location, status);
}

// ----------------------------------------------------------------------------
// HTTP Exception / Abort
// ----------------------------------------------------------------------------

class HTTPException : public std::runtime_error {
public:
    int code;
    std::string description;

    HTTPException(int status_code, std::string msg)
        : std::runtime_error("HTTP " + std::to_string(status_code) + ": " + msg),
          code(status_code),
          description(std::move(msg)) {}
};

[[noreturn]] inline void abort(int status_code, const std::string& message = "") {
    std::string desc = message;
    if (desc.empty()) {
        if (status_code == 400) desc = "Bad Request";
        else if (status_code == 401) desc = "Unauthorized";
        else if (status_code == 403) desc = "Forbidden";
        else if (status_code == 404) desc = "Not Found";
        else if (status_code == 405) desc = "Method Not Allowed";
        else if (status_code == 500) desc = "Internal Server Error";
        else desc = "HTTP Error";
    }
    throw HTTPException(status_code, desc);
}

// ----------------------------------------------------------------------------
// Static File Helpers
// ----------------------------------------------------------------------------

// Reads requested_path from base_dir on disk.
// Populates content and mime_type and returns true on success, or false if not found / path traversal.
inline bool read_static_file(const std::string& base_dir, const std::string& requested_path,
                             std::string& content, std::string& mime_type) {
    if (requested_path.empty()) return false;

    // Defense against Directory Traversal (CWE-22) and Null-Byte Injection
    if (requested_path.find("..") != std::string::npos) return false;
    if (requested_path.find('\0') != std::string::npos) return false;

    std::string full_path = base_dir;
    if (!full_path.empty() && full_path.back() != '/') full_path += '/';
    full_path += requested_path;

    std::ifstream in(full_path, std::ios::binary);
    if (!in) return false;

    std::ostringstream ss;
    ss << in.rdbuf();
    content = ss.str();
    mime_type = guess_mime_type(full_path);
    return true;
}

inline Response send_file(const std::string& file_path, const std::string& mimetype = "") {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        return Response::text("File not found", 404);
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    Response r;
    r.status = 200;
    r.body = ss.str();
    r.headers["Content-Type"] = mimetype.empty() ? guess_mime_type(file_path) : mimetype;
    return r;
}

inline Response send_from_directory(const std::string& directory, const std::string& filename) {
    std::string body, mime;
    if (!read_static_file(directory, filename, body, mime)) {
        return Response::text("File not found", 404);
    }
    Response r;
    r.status = 200;
    r.body = std::move(body);
    r.headers["Content-Type"] = std::move(mime);
    return r;
}

}  // namespace peregrine
