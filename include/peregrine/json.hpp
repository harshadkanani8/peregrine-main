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
// examples/json_pimpl.hpp
//
// Refactored Lightweight JSON Engine using PIMPL (Pointer to Implementation).
//
// WHY THIS REFACTOR:
// Storing recursive compound containers (std::vector<Json> and std::map<string, Json>)
// directly by value makes sizeof(Json) ~72 bytes. On 32-bit ARM (arm-linux-gnueabihf),
// this triggers GCC's AAPCS ABI parameter-passing diagnostic (-Wpsabi) during
// std::vector reallocations.
//
// By encapsulating the storage inside `std::shared_ptr<Value>`, sizeof(Json)
// becomes 4 bytes (pointer size on 32-bit ARM), completely eliminating the ARM ABI note
// without requiring any pragmas or compiler flags.
// ============================================================================
#pragma once

#include <cctype>
#include <initializer_list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace peregrine {

class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    // Internal value structure held via shared pointer
    struct Value {
        Type type = Type::Null;
        bool bool_ = false;
        double num_ = 0.0;
        std::string str_;
        std::vector<Json> arr_;
        std::map<std::string, Json> obj_;
    };

    // Constructors
    Json() : val_(std::make_shared<Value>()) {}
    Json(std::nullptr_t) : val_(std::make_shared<Value>()) {}

    Json(bool b) : val_(std::make_shared<Value>()) {
        val_->type = Type::Bool;
        val_->bool_ = b;
    }
    Json(int n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = n;
    }
    Json(long n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = static_cast<double>(n);
    }
    Json(long long n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = static_cast<double>(n);
    }
    Json(unsigned int n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = n;
    }
    Json(unsigned long n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = static_cast<double>(n);
    }
    Json(unsigned long long n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = static_cast<double>(n);
    }
    Json(double n) : val_(std::make_shared<Value>()) {
        val_->type = Type::Number;
        val_->num_ = n;
    }
    Json(const char* s) : val_(std::make_shared<Value>()) {
        val_->type = Type::String;
        val_->str_ = s ? s : "";
    }
    Json(const std::string& s) : val_(std::make_shared<Value>()) {
        val_->type = Type::String;
        val_->str_ = s;
    }

    // Static Initializers
    static Json object() {
        Json j;
        j.val_->type = Type::Object;
        return j;
    }

    static Json array() {
        Json j;
        j.val_->type = Type::Array;
        return j;
    }

    // Type Inspection
    Type type() const { return val_->type; }
    bool is_null() const { return val_->type == Type::Null; }
    bool is_bool() const { return val_->type == Type::Bool; }
    bool is_number() const { return val_->type == Type::Number; }
    bool is_string() const { return val_->type == Type::String; }
    bool is_array() const { return val_->type == Type::Array; }
    bool is_object() const { return val_->type == Type::Object; }

    // Object Operations
    Json& operator[](const std::string& key) {
        if (val_->type != Type::Object) {
            val_->type = Type::Object;
            val_->obj_.clear();
        }
        return val_->obj_[key];
    }

    const Json& operator[](const std::string& key) const {
        static const Json null_json;
        if (val_->type != Type::Object) return null_json;
        auto it = val_->obj_.find(key);
        return (it != val_->obj_.end()) ? it->second : null_json;
    }

    bool has(const std::string& key) const {
        return val_->type == Type::Object && val_->obj_.count(key) > 0;
    }

    const Json& at(const std::string& key) const {
        static const Json null_json;
        if (val_->type != Type::Object) return null_json;
        auto it = val_->obj_.find(key);
        return (it != val_->obj_.end()) ? it->second : null_json;
    }

    // Array Operations
    void push_back(const Json& v) {
        if (val_->type != Type::Array) {
            val_->type = Type::Array;
            val_->arr_.clear();
        }
        val_->arr_.push_back(v);
    }

    const Json& operator[](size_t index) const {
        static const Json null_json;
        if (val_->type != Type::Array || index >= val_->arr_.size()) return null_json;
        return val_->arr_[index];
    }

    Json& operator[](size_t index) {
        if (val_->type != Type::Array) {
            val_->type = Type::Array;
            val_->arr_.clear();
        }
        if (index >= val_->arr_.size()) {
            val_->arr_.resize(index + 1);
        }
        return val_->arr_[index];
    }

    size_t size() const {
        if (val_->type == Type::Array) return val_->arr_.size();
        if (val_->type == Type::Object) return val_->obj_.size();
        if (val_->type == Type::String) return val_->str_.size();
        return 0;
    }

    const std::vector<Json>& items() const { return val_->arr_; }
    const std::map<std::string, Json>& object_items() const { return val_->obj_; }

    // Value Getters with Default Fallbacks
    std::string as_string(const std::string& def = "") const {
        return val_->type == Type::String ? val_->str_ : def;
    }

    double as_number(double def = 0.0) const {
        return val_->type == Type::Number ? val_->num_ : def;
    }

    int as_int(int def = 0) const {
        return val_->type == Type::Number ? static_cast<int>(val_->num_) : def;
    }

    bool as_bool(bool def = false) const {
        return val_->type == Type::Bool ? val_->bool_ : def;
    }

    // Serialization
    std::string dump() const {
        std::ostringstream out;
        write(out);
        return out.str();
    }

    // Parser Entry Point
    static Json parse(const std::string& text) {
        size_t i = 0;
        skip_ws(text, i);
        return parse_value(text, i);
    }

private:
    std::shared_ptr<Value> val_;

    void write(std::ostringstream& out) const {
        switch (val_->type) {
            case Type::Null:
                out << "null";
                break;
            case Type::Bool:
                out << (val_->bool_ ? "true" : "false");
                break;
            case Type::Number: {
                if (val_->num_ == static_cast<long long>(val_->num_)) {
                    out << static_cast<long long>(val_->num_);
                } else {
                    out << val_->num_;
                }
                break;
            }
            case Type::String:
                write_escaped(out, val_->str_);
                break;
            case Type::Array: {
                out << "[";
                for (size_t i = 0; i < val_->arr_.size(); i++) {
                    if (i > 0) out << ",";
                    val_->arr_[i].write(out);
                }
                out << "]";
                break;
            }
            case Type::Object: {
                out << "{";
                bool first = true;
                for (const auto& item : val_->obj_) {
                    const std::string& k = item.first;
                    const Json& v = item.second;
                    if (!first) out << ",";
                    first = false;
                    write_escaped(out, k);
                    out << ":";
                    v.write(out);
                }
                out << "}";
                break;
            }
        }
    }

    static void write_escaped(std::ostringstream& out, const std::string& s) {
        out << '"';
        for (char c : s) {
            switch (c) {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b";  break;
                case '\f': out << "\\f";  break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                default:   out << c;      break;
            }
        }
        out << '"';
    }

    static void skip_ws(const std::string& s, size_t& i) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            i++;
        }
    }

    static Json parse_value(const std::string& s, size_t& i) {
        skip_ws(s, i);
        if (i >= s.size()) return Json();
        char c = s[i];
        if (c == '{') return parse_object(s, i);
        if (c == '[') return parse_array(s, i);
        if (c == '"') return Json(parse_string(s, i));
        if (c == 't' && s.compare(i, 4, "true") == 0) { i += 4; return Json(true); }
        if (c == 'f' && s.compare(i, 5, "false") == 0) { i += 5; return Json(false); }
        if (c == 'n' && s.compare(i, 4, "null") == 0) { i += 4; return Json(nullptr); }
        return parse_number(s, i);
    }

    static std::string parse_string(const std::string& s, size_t& i) {
        std::string out;
        i++;  // Skip opening quote
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char esc = s[i + 1];
                switch (esc) {
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    default:   out += esc;  break;
                }
                i += 2;
            } else {
                out += s[i++];
            }
        }
        if (i < s.size() && s[i] == '"') i++;  // Skip closing quote
        return out;
    }

    static Json parse_number(const std::string& s, size_t& i) {
        size_t start = i;
        if (i < s.size() && (s[i] == '-' || s[i] == '+')) i++;
        while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) ||
                               s[i] == '.' || s[i] == 'e' || s[i] == 'E' ||
                               s[i] == '+' || s[i] == '-')) {
            i++;
        }
        double val = 0.0;
        try {
            val = std::stod(s.substr(start, i - start));
        } catch (...) {
            val = 0.0;
        }
        return Json(val);
    }

    static Json parse_array(const std::string& s, size_t& i) {
        Json arr = Json::array();
        i++;  // Skip '['
        skip_ws(s, i);
        if (i < s.size() && s[i] == ']') { i++; return arr; }
        while (i < s.size()) {
            arr.push_back(parse_value(s, i));
            skip_ws(s, i);
            if (i < s.size() && s[i] == ',') {
                i++;
                continue;
            }
            if (i < s.size() && s[i] == ']') {
                i++;
                break;
            }
            break;
        }
        return arr;
    }

    static Json parse_object(const std::string& s, size_t& i) {
        Json obj = Json::object();
        i++;  // Skip '{'
        skip_ws(s, i);
        if (i < s.size() && s[i] == '}') { i++; return obj; }
        while (i < s.size()) {
            skip_ws(s, i);
            if (i >= s.size() || s[i] != '"') break;
            std::string key = parse_string(s, i);
            skip_ws(s, i);
            if (i < s.size() && s[i] == ':') i++;
            obj[key] = parse_value(s, i);
            skip_ws(s, i);
            if (i < s.size() && s[i] == ',') {
                i++;
                continue;
            }
            if (i < s.size() && s[i] == '}') {
                i++;
                break;
            }
            break;
        }
        return obj;
    }
};

}  // namespace peregrine