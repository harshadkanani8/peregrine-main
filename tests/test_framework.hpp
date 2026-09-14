/*
 * =========================================================================
 *  Peregrine++ Web Application Framework
 *  Unit Test & Verification Suite
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace peregrine_test {

struct TestCase {
    std::string suite_name;
    std::string test_name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add(const std::string& suite_name, const std::string& test_name, std::function<void()> func) {
        cases_.push_back({suite_name, test_name, std::move(func)});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;
        auto total_start = std::chrono::high_resolution_clock::now();

        std::cout << "\033[1;36m";
        std::cout << "======================================================================\n";
        std::cout << "  ____                              _             __ __ \n";
        std::cout << " /  __\\___  ________  ____ ________(_)_  _____   / // / \n";
        std::cout << "/ /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \\/ -_) / // /_ \n";
        std::cout << "/ .___/\\__//_/  \\__/\\_, /_/  /_/_/_/_//_/\\__/ /__  __/ \n";
        std::cout << "/_/                 /___/                        /_/    \n";
        std::cout << "             Peregrine Unit Tests & Verification Suite\n";
        std::cout << "======================================================================\033[0m\n\n";

        std::cout << "\033[1;34m[==========]\033[0m Running " << cases_.size() << " test cases.\n\n";

        std::string current_suite = "";

        for (const auto& tc : cases_) {
            if (tc.suite_name != current_suite) {
                current_suite = tc.suite_name;
                std::cout << "\033[1;35m[----------] Global test suite: " << current_suite << "\033[0m\n";
            }

            std::cout << "\033[1;33m[ RUN      ]\033[0m " << tc.suite_name << "." << tc.test_name << "\n";
            auto start = std::chrono::high_resolution_clock::now();
            try {
                tc.func();
                auto end = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                double ms_f = static_cast<double>(ms) / 1000.0;
                std::cout << "\033[1;32m[       OK ]\033[0m " << tc.suite_name << "." << tc.test_name
                          << " (" << ms_f << " ms)\n";
                passed++;
            } catch (const std::exception& ex) {
                std::cout << "\033[1;31m[  FAILED  ]\033[0m " << tc.suite_name << "." << tc.test_name
                          << "\n  \033[0;31mError: " << ex.what() << "\033[0m\n";
                failed++;
            } catch (...) {
                std::cout << "\033[1;31m[  FAILED  ]\033[0m " << tc.suite_name << "." << tc.test_name
                          << "\n  \033[0;31mError: Unknown non-std::exception thrown\033[0m\n";
                failed++;
            }
        }

        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();

        std::cout << "\n\033[1;34m[==========]\033[0m Total tests executed: " << cases_.size()
                  << " across " << total_ms << " ms.\n";
        std::cout << "\033[1;32m[  PASSED  ]\033[0m " << passed << " tests passed.\n";

        if (failed > 0) {
            std::cout << "\033[1;31m[  FAILED  ]\033[0m " << failed << " tests failed!\n";
            return 1;
        }

        std::cout << "\033[1;32m[  ALL TESTS PASSED!  ]\033[0m\n\n";
        return 0;
    }

private:
    std::vector<TestCase> cases_;
};

#define TEST_CASE(suite, name) \
    static void _test_##suite##_##name(); \
    static bool _reg_##suite##_##name = []() { \
        ::peregrine_test::TestRegistry::instance().add(#suite, #name, _test_##suite##_##name); \
        return true; \
    }(); \
    static void _test_##suite##_##name()

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream oss_msg; \
            oss_msg << "Condition (" #cond ") evaluated to false at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

#define EXPECT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream oss_msg; \
            oss_msg << "Condition (" #cond ") evaluated to true (expected false) at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

#define EXPECT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::ostringstream oss_msg; \
            oss_msg << "Equality check failed: (" #a " == " #b ") [" << (a) << " != " << (b) << "] at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

#define EXPECT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream oss_msg; \
            oss_msg << "Inequality check failed: (" #a " != " #b ") [" << (a) << " == " << (b) << "] at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

#define EXPECT_CONTAINS(haystack, needle) \
    do { \
        std::string _h = (haystack); \
        std::string _n = (needle); \
        if (_h.find(_n) == std::string::npos) { \
            std::ostringstream oss_msg; \
            oss_msg << "Substring search failed: '" << _n << "' not found in '" << _h << "' at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

#define EXPECT_THROW(expr, ExceptionType) \
    do { \
        bool _threw = false; \
        try { \
            expr; \
        } catch (const ExceptionType&) { \
            _threw = true; \
        } catch (...) { \
            std::ostringstream oss_msg; \
            oss_msg << "Expression (" #expr ") threw unexpected exception type (expected " #ExceptionType ") at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
        if (!_threw) { \
            std::ostringstream oss_msg; \
            oss_msg << "Expression (" #expr ") failed to throw expected " #ExceptionType " at line " << __LINE__; \
            throw std::runtime_error(oss_msg.str()); \
        } \
    } while (0)

} // namespace peregrine_test
