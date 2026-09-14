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
// peregrine/peregrine.hpp
//
// Master header for the Peregrine C++ Web Framework.
// Include this single header to access all framework components.
// ============================================================================
#pragma once

#include "app.hpp"
#include "blueprint.hpp"
#include "common.hpp"
#include "config.hpp"
#include "connection.hpp"
#include "csrf.hpp"
#include "helpers.hpp"
#include "json.hpp"
#include "request.hpp"
#include "response.hpp"
#include "routing.hpp"
#include "server.hpp"
#include "session.hpp"
#include "template.hpp"
#include "thread_pool.hpp"
#include "types.hpp"
#include "view.hpp"

// Backward-compatibility aliases for smooth transition
namespace fw = peregrine;

