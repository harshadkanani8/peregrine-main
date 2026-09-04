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
// peregrine/csrf.hpp
//
// CSRF (Cross-Site Request Forgery) protection.
// Stores a random token in the signed session cookie and verifies it upon
// receiving state-modifying HTTP requests (POST, PUT, PATCH, DELETE).
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <string>

#include "common.hpp"
#include "request.hpp"

namespace peregrine {
namespace csrf {

// Generates or retrieves the CSRF token stored in the request's session.
inline std::string get_token(Request& req) {
    std::string token = req.session.get("_csrf_token");
    if (!token.empty()) {
        return token;
    }
    token = random_hex(32);
    req.session["_csrf_token"] = token;
    return token;
}

// Verifies the incoming CSRF token against the one stored in the session.
// Checks (in order):
// 1. Form field "csrf_token"
// 2. HTTP header "X-CSRF-Token" or "X-CSRFToken"
// 3. Query string parameter "csrf_token"
inline bool verify_token(Request& req) {
    std::string expected = req.session.get("_csrf_token");
    if (expected.empty()) {
        return false;
    }

    std::string submitted = req.form_get("csrf_token");
    if (submitted.empty()) submitted = req.header("X-CSRF-Token");
    if (submitted.empty()) submitted = req.header("X-CSRFToken");
    if (submitted.empty()) submitted = req.arg("csrf_token");
    if (submitted.empty()) return false;

    return constant_time_equal(submitted, expected);
}

}  // namespace csrf
}  // namespace peregrine
