cmake_minimum_required(VERSION 3.14)
project(PeregrineExamples VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# 01_hello_world
add_executable(example_hello_world 01_hello_world/main.cpp)
target_link_libraries(example_hello_world PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(example_hello_world PRIVATE ws2_32)
endif()

# 02_rest_api
add_executable(example_rest_api 02_rest_api/main.cpp)
target_link_libraries(example_rest_api PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(example_rest_api PRIVATE ws2_32)
endif()

# 03_auth_sessions
add_executable(example_auth_sessions 03_auth_sessions/main.cpp)
target_link_libraries(example_auth_sessions PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(example_auth_sessions PRIVATE ws2_32)
endif()

# 04_template_rendering
add_executable(example_templates 04_template_rendering/main.cpp)
target_link_libraries(example_templates PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(example_templates PRIVATE ws2_32)
endif()

# 05_file_upload
add_executable(example_file_upload 05_file_upload/main.cpp)
target_link_libraries(example_file_upload PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(example_file_upload PRIVATE ws2_32)
endif()
