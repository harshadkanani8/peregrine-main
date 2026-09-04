# Peregrine CMake Guide

This document provides a detailed, section-by-section breakdown of the `CMakeLists.txt` file used in the Peregrine C++ Web Framework. Since Peregrine is a **header-only library**, its CMake configuration is specifically designed to export its interface rather than compile object files.

---

## 1. Initial Setup
```cmake
cmake_minimum_required(VERSION 3.14)
project(Peregrine VERSION 1.0.0 LANGUAGES CXX)
```
- **`cmake_minimum_required`**: Ensures the user has at least CMake version 3.14. This is important because older versions don't support some of the modern target-based commands we use.
- **`project`**: Defines the project name (`Peregrine`), its version (`1.0.0`), and specifies that it is a C++ (`CXX`) project.

## 2. C++ Standard Configuration
```cmake
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```
- We explicitly set the standard to **C++11**, meaning any project consuming this library must compile with at least C++11.
- `REQUIRED ON` forces the build to fail if the compiler doesn't support C++11.
- `EXTENSIONS OFF` ensures we use strict standard C++ (e.g., `-std=c++11` instead of `-std=gnu++11`), maximizing cross-compiler portability.

## 3. Compiler Warnings
```cmake
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()
```
- We set strict warning levels depending on the compiler. 
- For Microsoft Visual C++ (`MSVC`), we use `/W4` (level 4 warnings) and `/permissive-` (strict standard conformance).
- For GCC/Clang, we enable all standard warnings (`-Wall -Wextra -Wpedantic`) and apply level-2 optimization (`-O2`).

## 4. Dependencies
```cmake
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
```
- Peregrine relies on two external dependencies: POSIX Threads (for the multi-threaded web server) and OpenSSL (for cryptography, sessions, and HTTPS). 
- `REQUIRED` means the CMake configuration will halt with an error if it cannot locate these on the host system.

## 5. The Framework Target (Header-Only)
```cmake
add_library(peregrine_framework INTERFACE)
add_library(peregrine::framework ALIAS peregrine_framework)
add_library(peregrine::peregrine ALIAS peregrine_framework)
```
- **`INTERFACE`**: This is the most crucial part. Because Peregrine is header-only, there is no source code to compile into a static (`.a`/`.lib`) or dynamic (`.so`/`.dll`) library. An `INTERFACE` library in CMake is purely a collection of compile flags, include directories, and link dependencies that get passed on to any application that links against it.
- **`ALIAS`**: We create namespace aliases (like `peregrine::peregrine`). This allows developers to consume the library cleanly using `target_link_libraries(myapp PRIVATE peregrine::peregrine)`.

## 6. Include Directories & Generator Expressions
```cmake
target_include_directories(peregrine_framework INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
```
- We attach the `include/` directory to the `peregrine_framework` target. Anyone who links against Peregrine automatically gets this include path.
- **`$<BUILD_INTERFACE:...>`**: These paths are used when another project builds against Peregrine directly from the source tree.
- **`$<INSTALL_INTERFACE:...>`**: This path is used when Peregrine has been installed to the system (e.g., `/usr/local/include`), telling CMake where to find the headers post-installation.

## 7. Linking Dependencies
```cmake
target_link_libraries(peregrine_framework INTERFACE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)
```
- We attach the OpenSSL and Threading libraries to the framework. Because they are linked as `INTERFACE`, any application that links against `peregrine_framework` will automatically link against OpenSSL and Threads without having to configure them manually.

## 8. Custom Targets
```cmake
add_custom_target(cert
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_cert.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating self-signed SSL/TLS certificates"
)
```
- This adds a helper command (`make cert` or `cmake --build . --target cert`) that executes the `generate_cert.sh` bash script to easily generate local SSL certificates for HTTPS testing.

## 9. Installation Rules
```cmake
include(GNUInstallDirs)

install(TARGETS peregrine_framework
    EXPORT PeregrineTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/peregrine
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
```
- **`GNUInstallDirs`**: Provides standard installation paths (like `/usr/local/include` mapping to `${CMAKE_INSTALL_INCLUDEDIR}`).
- **`install(TARGETS ...)`**: Registers the `peregrine_framework` CMake target so it can be exported and found by other CMake projects using `find_package(Peregrine)`.
- **`install(DIRECTORY ...)`**: Physically copies the `include/peregrine` folder into the system's include directory when the user runs `make install` or `cmake --install .`.
