// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"

int main() {
    return ::peregrine_test::TestRegistry::instance().run_all();
}
