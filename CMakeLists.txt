cmake_minimum_required(VERSION 3.14)
project(Peregrine VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# C++ Standard Configuration
# =============================================================================
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# =============================================================================
# Dependencies
# =============================================================================
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# =============================================================================
# Peregrine Framework Target (Header-Only Library)
# =============================================================================
add_library(peregrine_framework INTERFACE)
add_library(peregrine::framework ALIAS peregrine_framework)
add_library(peregrine::peregrine ALIAS peregrine_framework)

# Backward-compatibility aliases
add_library(flask_framework ALIAS peregrine_framework)
add_library(flask::framework ALIAS peregrine_framework)

target_include_directories(peregrine_framework INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(peregrine_framework INTERFACE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# =============================================================================
# Convenience Custom Targets
# =============================================================================
add_custom_target(cert
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_cert.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating self-signed SSL/TLS certificates"
)

# =============================================================================
# Installation Rules (Exporting to other CMake projects)
# =============================================================================
include(GNUInstallDirs)

install(TARGETS peregrine_framework
    EXPORT PeregrineTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/peregrine
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
