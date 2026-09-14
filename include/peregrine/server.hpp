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
// peregrine/server.hpp
//
// HTTP/1.1 parsing engine, multipart/form-data processor, TLS context setup,
// and response serializer.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "common.hpp"
#include "connection.hpp"
#include "request.hpp"
#include "response.hpp"
#include "types.hpp"

namespace peregrine {
namespace detail {

inline SSL_CTX* create_ssl_context(const std::string& cert_file,
                                  const std::string& key_file,
                                  const std::string& tls_version = "TLS_1_2") {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Error creating SSL context\n";
        return nullptr;
    }

    // Configure minimum TLS protocol version
    if (tls_version == "TLS_1_3") {
#ifdef TLS1_3_VERSION
        SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
#else
        SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#endif
    } else {
#ifdef TLS1_2_VERSION
        SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#else
        SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading certificate: " << cert_file << "\n";
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading private key: " << key_file << "\n";
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Private key does not match the certificate public key\n";
        SSL_CTX_free(ctx);
        return nullptr;
    }

    return ctx;
}

inline void parse_multipart(Request& req, const std::string& content_type_header) {
    auto bpos = content_type_header.find("boundary=");
    if (bpos == std::string::npos) return;
    std::string boundary = "--" + content_type_header.substr(bpos + 9);
    if (!boundary.empty() && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }

    size_t pos = 0;
    while ((pos = req.body.find(boundary, pos)) != std::string::npos) {
        pos += boundary.size();
        if (pos + 1 < req.body.size() && req.body[pos] == '-' && req.body[pos + 1] == '-') {
            break;  // End boundary
        }
        if (pos + 1 < req.body.size() && req.body[pos] == '\r' && req.body[pos + 1] == '\n') {
            pos += 2;
        }

        size_t next_boundary = req.body.find(boundary, pos);
        if (next_boundary == std::string::npos) break;

        std::string part = req.body.substr(pos, next_boundary - pos);
        if (part.size() >= 2 && part.substr(part.size() - 2) == "\r\n") {
            part.erase(part.size() - 2);
        }

        size_t header_end = part.find("\r\n\r\n");
        if (header_end == std::string::npos) continue;

        std::string part_headers = part.substr(0, header_end);
        std::string part_body = part.substr(header_end + 4);

        std::string name, filename, ctype;
        for (const auto& hline : split(part_headers, '\n')) {
            std::string h = trim(hline);
            if (starts_with(to_lower(h), "content-disposition:")) {
                auto name_pos = h.find("name=\"");
                if (name_pos != std::string::npos) {
                    auto name_end = h.find('"', name_pos + 6);
                    if (name_end != std::string::npos) {
                        name = h.substr(name_pos + 6, name_end - name_pos - 6);
                    }
                }
                auto fn_pos = h.find("filename=\"");
                if (fn_pos != std::string::npos) {
                    auto fn_end = h.find('"', fn_pos + 10);
                    if (fn_end != std::string::npos) {
                        filename = h.substr(fn_pos + 10, fn_end - fn_pos - 10);
                    }
                }
            } else if (starts_with(to_lower(h), "content-type:")) {
                ctype = trim(h.substr(13));
            }
        }

        if (!filename.empty()) {
            UploadedFile f;
            f.field_name = name;
            f.filename = filename;
            f.content_type = ctype.empty() ? "application/octet-stream" : ctype;
            f.data = part_body;
            req.files.push_back(std::move(f));
        } else if (!name.empty()) {
            req.form[name] = part_body;
        }
    }
}

inline std::string read_full_request(Connection& conn, size_t max_body_size) {
    std::string raw;
    char buf[8192];
    size_t header_end = std::string::npos;
    size_t content_length = 0;
    bool has_cl = false;

    while (true) {
        ssize_t n = conn.read(buf, sizeof(buf));
        if (n <= 0) break;
        raw.append(buf, static_cast<size_t>(n));

        if (header_end == std::string::npos) {
            header_end = raw.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                std::string header_part = to_lower(raw.substr(0, header_end));
                auto cl_pos = header_part.find("content-length:");
                if (cl_pos != std::string::npos) {
                    auto end_line = header_part.find("\r\n", cl_pos);
                    std::string cl_str = trim(header_part.substr(cl_pos + 15, end_line - (cl_pos + 15)));
                    try {
                        content_length = static_cast<size_t>(std::stoul(cl_str));
                        has_cl = true;
                    } catch (...) {
                        content_length = 0;
                    }
                }
            }
        }

        if (header_end != std::string::npos) {
            if (content_length > max_body_size) {
                return "";  // Payload Too Large
            }
            size_t total_expected = header_end + 4 + (has_cl ? content_length : 0);
            if (raw.size() >= total_expected) {
                break;
            }
        }
    }
    return raw;
}

inline Request parse_request(const std::string& raw) {
    Request req;
    if (raw.empty()) return req;

    std::istringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    auto parts = split(line, ' ');
    if (parts.size() < 2) return req;
    req.method = parts[0];
    std::string target = parts[1];
    if (parts.size() >= 3) req.http_version = parts[2];

    auto qpos = target.find('?');
    req.raw_target = target;
    if (qpos != std::string::npos) {
        req.path = target.substr(0, qpos);
        req.query = parse_urlencoded(target.substr(qpos + 1));
    } else {
        req.path = target;
    }
    req.path = url_decode(req.path);

    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty()) break;
        if (line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = to_lower(trim(line.substr(0, colon)));
        std::string val = trim(line.substr(colon + 1));
        req.headers[key] = val;
    }

    std::ostringstream body_stream;
    body_stream << stream.rdbuf();
    req.body = body_stream.str();

    // Parse Cookies
    if (req.headers.count("cookie")) {
        for (const auto& c : split(req.headers["cookie"], ';')) {
            auto eq = c.find('=');
            if (eq == std::string::npos) continue;
            req.cookies[trim(c.substr(0, eq))] = trim(c.substr(eq + 1));
        }
    }

    // Parse Form or Multipart
    std::string ctype = req.header("content-type");
    if (ctype.find("application/x-www-form-urlencoded") != std::string::npos) {
        req.form = parse_urlencoded(req.body);
    } else if (ctype.find("multipart/form-data") != std::string::npos) {
        parse_multipart(req, ctype);
    }

    return req;
}

inline std::string status_reason(int status) {
    static const std::map<int, std::string> reasons = {
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {304, "Not Modified"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {409, "Conflict"},
        {413, "Payload Too Large"},
        {415, "Unsupported Media Type"},
        {429, "Too Many Requests"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"}
    };
    auto it = reasons.find(status);
    return (it != reasons.end()) ? it->second : "OK";
}

inline std::string serialize_response_headers(const Response& res) {
    std::ostringstream out;
    out << "HTTP/1.1 " << res.status << " " << status_reason(res.status) << "\r\n";

    for (const auto& item : res.headers) {
        const std::string& k = item.first;
        const std::string& v = item.second;
        out << k << ": " << v << "\r\n";
    }
    for (const auto& c : res.raw_set_cookies) {
        out << "Set-Cookie: " << c << "\r\n";
    }

    if (!res.is_stream()) {
        out << "Content-Length: " << res.body.size() << "\r\n";
    }
    out << "Connection: close\r\n\r\n";
    return out.str();
}

inline std::string serialize_response(const Response& res, bool head_only = false) {
    std::string out = serialize_response_headers(res);
    if (!head_only && !res.is_stream()) {
        out += res.body;
    }
    return out;
}

}  // namespace detail
}  // namespace peregrine
