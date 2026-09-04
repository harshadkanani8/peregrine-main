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
// peregrine/response.hpp
//
// HTTP Response wrapper supporting headers, cookies, redirects, JSON,
// HTML, live data streams, and MJPEG video streaming.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "connection.hpp"
#include "json.hpp"

namespace peregrine {

struct Response {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::vector<std::string> raw_set_cookies;
    std::string body;
    std::function<void(Connection&)> streamer = nullptr;

    Response() {
        headers["Content-Type"] = "text/html; charset=utf-8";
    }

    bool is_stream() const { return streamer != nullptr; }

    // ------------------------------------------------------------------------
    // Static response factories
    // ------------------------------------------------------------------------

    static Response text(const std::string& body_text, int status_code = 200) {
        Response r;
        r.status = status_code;
        r.body = body_text;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        return r;
    }

    static Response html(const std::string& html_content, int status_code = 200) {
        Response r;
        r.status = status_code;
        r.body = html_content;
        r.headers["Content-Type"] = "text/html; charset=utf-8";
        return r;
    }

    static Response json(const Json& j, int status_code = 200) {
        Response r;
        r.status = status_code;
        r.body = j.dump();
        r.headers["Content-Type"] = "application/json";
        return r;
    }

    static Response redirect(const std::string& location, int status_code = 302) {
        Response r;
        r.status = status_code;
        r.headers["Location"] = location;
        r.body = "";
        return r;
    }

    static Response stream(const std::string& content_type,
                           std::function<void(Connection&)> streamer_fn,
                           int status_code = 200) {
        Response r;
        r.status = status_code;
        r.headers["Content-Type"] = content_type;
        r.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        r.headers["Pragma"] = "no-cache";
        r.headers["Expires"] = "0";
        r.headers["Connection"] = "close";
        r.streamer = std::move(streamer_fn);
        return r;
    }

    static Response mjpeg(std::function<void(Connection&)> streamer_fn) {
        return stream("multipart/x-mixed-replace; boundary=frame", std::move(streamer_fn));
    }

    // ------------------------------------------------------------------------
    // Response mutators & Cookie management
    // ------------------------------------------------------------------------

    Response& set_header(const std::string& key, const std::string& value) {
        headers[key] = value;
        return *this;
    }

    Response& set_status(int status_code) {
        status = status_code;
        return *this;
    }

    void set_cookie(const std::string& name, const std::string& value,
                    int max_age_seconds = -1, const std::string& path = "/",
                    const std::string& domain = "",
                    bool http_only = false, bool secure = false,
                    const std::string& same_site = "",
                    bool partitioned = false) {
        std::ostringstream c;
        c << name << "=" << value << "; Path=" << (path.empty() ? "/" : path);
        if (!domain.empty()) c << "; Domain=" << domain;
        if (max_age_seconds >= 0) c << "; Max-Age=" << max_age_seconds;
        if (http_only) c << "; HttpOnly";
        if (secure) c << "; Secure";
        if (!same_site.empty()) c << "; SameSite=" << same_site;
        if (partitioned) c << "; Partitioned";
        raw_set_cookies.push_back(c.str());
    }

    void delete_cookie(const std::string& name, const std::string& path = "/",
                       const std::string& domain = "") {
        std::ostringstream c;
        c << name << "=; Path=" << (path.empty() ? "/" : path);
        if (!domain.empty()) c << "; Domain=" << domain;
        c << "; Max-Age=0";
        raw_set_cookies.push_back(c.str());
    }
};

}  // namespace peregrine
