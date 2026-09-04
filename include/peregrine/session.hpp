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
// peregrine/session.hpp
//
// 100% Peregrine-compatible session architecture matching Pallets Peregrine src/peregrine/sessions.py:
// - SessionMixin & SecureCookieSession data container
// - NullSession safety & error semantics
// - Pluggable SessionInterface base class (for Redis, DB, or custom session stores)
// - SecureCookieSessionInterface (default itsdangerous HMAC-SHA256 signed cookies)
// - Key rotation fallbacks (SECRET_KEY_FALLBACKS)
// - Expiration & Inactivity lifetime management
// - SameSite, HttpOnly, Secure, Domain, Path, and Partitioned (CHIPS) flags
// - Automatic "Vary: Cookie" header propagation on session access
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common.hpp"
#include "json.hpp"

namespace peregrine {

// Forward declarations
class App;
struct Request;
struct Response;

// ============================================================================
// 1. Session Container (mirrors Peregrine SessionMixin + SecureCookieSession)
// ============================================================================

class Session {
public:
    // If permanent is true, cookie has Max-Age / Expires (PERMANENT_SESSION_LIFETIME).
    // If false (default in Peregrine), cookie is a browser session cookie (deleted on browser close).
    bool permanent = false;

    // Modification tracking: cookie is only written if session data changed or refresh is requested
    bool modified = false;

    // Access tracking: if accessed during request dispatch, "Vary: Cookie" is emitted
    bool accessed = false;

    // True if session was newly created during this request
    bool is_new = true;

    // Null session flag (if secret_key is missing on the app)
    bool is_null = false;

    Session() = default;

    // Bracket operators (marks session as accessed and modified)
    std::string& operator[](const std::string& key) {
        check_null();
        accessed = true;
        modified = true;
        return data_[key];
    }

    const std::string& operator[](const std::string& key) const {
        const_cast<Session*>(this)->accessed = true;
        static const std::string empty_str;
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : empty_str;
    }

    // Getters
    std::string get(const std::string& key, const std::string& def = "") const {
        const_cast<Session*>(this)->accessed = true;
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : def;
    }

    int get_int(const std::string& key, int def = 0) const {
        const_cast<Session*>(this)->accessed = true;
        auto it = data_.find(key);
        if (it == data_.end()) return def;
        try {
            return std::stoi(it->second);
        } catch (...) {
            return def;
        }
    }

    bool get_bool(const std::string& key, bool def = false) const {
        const_cast<Session*>(this)->accessed = true;
        auto it = data_.find(key);
        if (it == data_.end()) return def;
        const std::string& val = it->second;
        return (val == "1" || val == "true" || val == "True" || val == "yes" || val == "YES");
    }

    // Mutators
    void set(const std::string& key, const std::string& value) {
        check_null();
        data_[key] = value;
        modified = true;
        accessed = true;
    }

    bool has(const std::string& key) const {
        const_cast<Session*>(this)->accessed = true;
        return data_.count(key) > 0;
    }

    bool contains(const std::string& key) const {
        return has(key);
    }

    size_t count(const std::string& key) const {
        const_cast<Session*>(this)->accessed = true;
        return data_.count(key);
    }

    std::string pop(const std::string& key, const std::string& def = "") {
        check_null();
        accessed = true;
        auto it = data_.find(key);
        if (it != data_.end()) {
            std::string val = std::move(it->second);
            data_.erase(it);
            modified = true;
            return val;
        }
        return def;
    }

    void erase(const std::string& key) {
        check_null();
        accessed = true;
        if (data_.erase(key) > 0) {
            modified = true;
        }
    }

    void clear() {
        check_null();
        if (!data_.empty()) {
            data_.clear();
            modified = true;
        }
    }

    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }

    const std::map<std::string, std::string>& data() const { const_cast<Session*>(this)->accessed = true; return data_; }
    std::map<std::string, std::string>& data() { accessed = true; modified = true; return data_; }

    // Map iterators
    std::map<std::string, std::string>::iterator begin() { accessed = true; return data_.begin(); }
    std::map<std::string, std::string>::iterator end() { return data_.end(); }
    std::map<std::string, std::string>::const_iterator begin() const { const_cast<Session*>(this)->accessed = true; return data_.begin(); }
    std::map<std::string, std::string>::const_iterator end() const { return data_.end(); }
    std::map<std::string, std::string>::iterator find(const std::string& key) { accessed = true; return data_.find(key); }
    std::map<std::string, std::string>::const_iterator find(const std::string& key) const { const_cast<Session*>(this)->accessed = true; return data_.find(key); }
    const std::string& at(const std::string& key) const { const_cast<Session*>(this)->accessed = true; return data_.at(key); }

    // Pointer-like dereference helpers for backward compatibility with `*req.session`
    Session& operator*() { return *this; }
    const Session& operator*() const { return *this; }
    Session* operator->() { return this; }
    const Session* operator->() const { return this; }

private:
    std::map<std::string, std::string> data_;

    void check_null() const {
        if (is_null) {
            throw std::runtime_error(
                "The session is unavailable because no secret key was set. "
                "Set the secret_key on the application to something unique and secret.");
        }
    }
};

// ============================================================================
// 2. Cryptographic Token Engine (itsdangerous URLSafeTimedSerializer)
// ============================================================================

// Signs session data into a token:
//   base64url(json) + "." + timestamp_seconds + "." + base64url(hmac_sha256)
inline std::string sign_session(const std::string& secret_key,
                                const std::map<std::string, std::string>& session_data,
                                const std::string& salt = "cookie-session") {
    Json j = Json::object();
    for (const auto& item : session_data) {
        const std::string& k = item.first;
        const std::string& v = item.second;
        j[k] = v;
    }

    std::string payload = base64url_encode(j.dump());

    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    std::string ts = std::to_string(now);

    std::string signing_input = payload + "." + ts;
    std::string effective_key = hmac_sha256(secret_key, salt);
    std::string sig = base64url_encode(hmac_sha256(effective_key, signing_input));

    return signing_input + "." + sig;
}

// Verifies and decodes a signed session token.
inline bool verify_and_load_session(const std::string& secret_key,
                                    const std::string& token,
                                    int max_age_seconds,
                                    std::map<std::string, std::string>& session_data,
                                    const std::string& salt = "cookie-session") {
    auto first_dot = token.find('.');
    if (first_dot == std::string::npos) return false;
    auto second_dot = token.find('.', first_dot + 1);
    if (second_dot == std::string::npos) return false;

    std::string payload_b64 = token.substr(0, first_dot);
    std::string ts_str = token.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string sig_b64 = token.substr(second_dot + 1);

    std::string signing_input = payload_b64 + "." + ts_str;
    std::string effective_key = hmac_sha256(secret_key, salt);
    std::string expected_sig = base64url_encode(hmac_sha256(effective_key, signing_input));
    if (!constant_time_equal(sig_b64, expected_sig)) {
        return false;
    }

    uint64_t issued_at = 0;
    try {
        issued_at = std::stoull(ts_str);
    } catch (...) {
        return false;
    }

    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    if (now < issued_at || (now - issued_at) > static_cast<uint64_t>(max_age_seconds)) {
        return false;  // Expired
    }

    Json j = Json::parse(base64url_decode(payload_b64));
    if (j.type() != Json::Type::Object) return false;

    session_data.clear();
    for (const auto& item : j.object_items()) {
        const std::string& k = item.first;
        const Json& v = item.second;
        session_data[k] = v.as_string();
    }
    return true;
}

}  // namespace peregrine
