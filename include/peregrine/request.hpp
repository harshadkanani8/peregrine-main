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
// peregrine/request.hpp
//
// HTTP Request wrapper containing parameters, headers, cookies, form data,
// JSON payloads, uploaded files, and session storage.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <map>
#include <string>
#include <vector>

#include "common.hpp"
#include "json.hpp"
#include "session.hpp"
#include "types.hpp"

namespace peregrine {

struct Request {
    std::string method;
    std::string path;                 // Path component only (URL decoded)
    std::string raw_target;           // Raw URI target (path + ?query)
    std::string http_version = "HTTP/1.1";

    std::map<std::string, std::string> headers;      // Case-insensitive (keys stored in lower-case)
    std::map<std::string, std::string> query;        // Query string parameters (request.args)
    std::map<std::string, std::string> path_params;  // Matched URL parameters (e.g. <int:user_id>)
    std::map<std::string, std::string> cookies;      // Client cookies
    std::string body;                                // Raw request body

    // Form data (from application/x-www-form-urlencoded or multipart/form-data)
    std::map<std::string, std::string> form;

    // Uploaded files (from multipart/form-data)
    std::vector<UploadedFile> files;

    // First-class session container (Peregrine-style SessionMixin)
    Session session;

    // Per-request application context storage (Peregrine-style `g` object)
    std::map<std::string, std::string> g;

    // Connection metadata
    std::string scheme = "http";
    std::string remote_addr = "127.0.0.1";

    // ------------------------------------------------------------------------
    // Helper accessors
    // ------------------------------------------------------------------------

    std::string header(const std::string& name, const std::string& def = "") const {
        auto it = headers.find(to_lower(name));
        return (it != headers.end()) ? it->second : def;
    }

    bool has_header(const std::string& name) const {
        return headers.count(to_lower(name)) > 0;
    }

    std::string arg(const std::string& name, const std::string& def = "") const {
        auto it = query.find(name);
        return (it != query.end()) ? it->second : def;
    }

    bool has_arg(const std::string& name) const {
        return query.count(name) > 0;
    }

    std::string form_get(const std::string& name, const std::string& def = "") const {
        auto it = form.find(name);
        return (it != form.end()) ? it->second : def;
    }

    bool has_form(const std::string& name) const {
        return form.count(name) > 0;
    }

    std::string cookie(const std::string& name, const std::string& def = "") const {
        auto it = cookies.find(name);
        return (it != cookies.end()) ? it->second : def;
    }

    bool has_cookie(const std::string& name) const {
        return cookies.count(name) > 0;
    }

    bool is_secure() const {
        return scheme == "https";
    }

    bool is_json() const {
        std::string ct = header("Content-Type");
        return ct.find("application/json") != std::string::npos;
    }

    // Parses the request body as JSON. Returns a Null Json object if empty or invalid.
    Json get_json() const {
        if (body.empty()) return Json();
        return Json::parse(body);
    }
};

}  // namespace peregrine
