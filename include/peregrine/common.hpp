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
// peregrine/common.hpp
//
// String utilities, URL parsing/encoding, cryptography, and random generators.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace peregrine {

// ============================================================================
// String Operations
// ============================================================================

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

inline bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

// ============================================================================
// URL Encoding / Decoding
// ============================================================================

inline std::string url_decode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int value = 0;
            std::istringstream iss(s.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                out += static_cast<char>(value);
                i += 2;
            } else {
                out += s[i];
            }
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

inline std::string url_encode(const std::string& s) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(c);
        }
    }
    return escaped.str();
}

inline std::map<std::string, std::string> parse_urlencoded(const std::string& body) {
    std::map<std::string, std::string> result;
    for (const auto& pair : split(body, '&')) {
        if (pair.empty()) continue;
        auto eq = pair.find('=');
        if (eq == std::string::npos) {
            result[url_decode(pair)] = "";
        } else {
            result[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
        }
    }
    return result;
}

// ============================================================================
// Cryptography & Random Utilities
// ============================================================================

inline std::string random_hex(size_t n_bytes) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    std::ostringstream oss;
    for (size_t i = 0; i < n_bytes; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << dist(gen);
    }
    return oss.str();
}

// URL-safe Base64 encoding without padding
inline std::string base64url_encode(const std::string& in) {
    static const char* alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    int val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(alphabet[(val >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) {
        out.push_back(alphabet[((val << 8) >> (bits + 8)) & 0x3F]);
    }
    return out;
}

inline std::string base64url_decode(const std::string& in) {
    static int8_t table[256];
    static bool init = false;
    if (!init) {
        std::fill(std::begin(table), std::end(table), -1);
        static const char* alphabet =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        for (int i = 0; i < 64; i++) {
            table[static_cast<unsigned char>(alphabet[i])] = static_cast<int8_t>(i);
        }
        init = true;
    }
    std::string out;
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        if (table[c] == -1) continue;
        val = (val << 6) + table[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

// HMAC-SHA256 calculation using OpenSSL EVP
inline std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char* digest = HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
                                  reinterpret_cast<const unsigned char*>(data.data()),
                                  data.size(), nullptr, nullptr);
    return std::string(reinterpret_cast<char*>(digest), 32);  // SHA-256 is 32 bytes
}

// Constant-time string comparison to prevent timing attacks on cryptographic signatures
inline bool constant_time_equal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); i++) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

}  // namespace peregrine
