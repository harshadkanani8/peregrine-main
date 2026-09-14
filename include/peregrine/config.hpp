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
// peregrine/config.hpp
//
// Configuration object with dictionary-like access and environment variable loading.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <cstdlib>
#include <map>
#include <string>

namespace peregrine {

class Config {
public:
    Config() {
        // Sensible defaults
        values_["SECRET_KEY"] = "";
        values_["SESSION_COOKIE_NAME"] = "session";
        values_["PERMANENT_SESSION_LIFETIME"] = "86400";  // 24 hours
        values_["SESSION_COOKIE_HTTPONLY"] = "true";
        values_["SESSION_COOKIE_SECURE"] = "false";
        values_["SESSION_COOKIE_SAMESITE"] = "Lax";
        values_["MAX_CONTENT_LENGTH"] = "16777216";       // 16 MB
    }

    std::string& operator[](const std::string& key) {
        return values_[key];
    }

    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = values_.find(key);
        return (it != values_.end()) ? it->second : def;
    }

    int get_int(const std::string& key, int def = 0) const {
        auto it = values_.find(key);
        if (it == values_.end()) return def;
        try {
            return std::stoi(it->second);
        } catch (...) {
            return def;
        }
    }

    bool get_bool(const std::string& key, bool def = false) const {
        auto it = values_.find(key);
        if (it == values_.end()) return def;
        std::string val = it->second;
        return (val == "1" || val == "true" || val == "True" || val == "YES" || val == "yes");
    }

    void set(const std::string& key, const std::string& value) {
        values_[key] = value;
    }

    // Loads a setting from an environment variable if set
    void from_env(const std::string& config_key, const std::string& env_var_name) {
        const char* val = std::getenv(env_var_name.c_str());
        if (val) {
            values_[config_key] = val;
        }
    }

    const std::map<std::string, std::string>& all() const { return values_; }

private:
    std::map<std::string, std::string> values_;
};

}  // namespace peregrine
