cmake_minimum_required(VERSION 3.16)

# Use C++20 (or C++17 if you prefer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(simple_http
    LANGUAGES CXX
)

include(CTest)
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

# For MSVC: avoid overriding parent project's runtime library settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Common warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --------------------------------------------------------------------
# Library: net  (platform glue + socket wrapper + connect helper)
# --------------------------------------------------------------------
add_library(net STATIC
    net_platform.cpp
    socket.cpp
    net_connect.cpp
)

# Public include dir: the project root (where all headers live)
target_include_directories(net
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# On Windows, link against Winsock2
if (WIN32)
    target_link_libraries(net
        PUBLIC
            ws2_32
    )
endif()

# --------------------------------------------------------------------
# Library: http  (HTTP messages + server/client code)
# --------------------------------------------------------------------
add_library(http STATIC
    http_message.cpp
    http_client.cpp
    http_server.cpp
)

target_include_directories(http
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(http
    PUBLIC
        net
)

# --------------------------------------------------------------------
# Executable: http_server_main  (main() for the server)
# --------------------------------------------------------------------
add_executable(http_server_main
    server_main.cpp
)

target_link_libraries(http_server_main
    PRIVATE
        http
)

# --------------------------------------------------------------------
# Executable: http_client_main  (main() for the client)
# --------------------------------------------------------------------
add_executable(http_client_main
    client_main.cpp
)

target_link_libraries(http_client_main
    PRIVATE
        http
)

# --------------------------------------------------------------------
# Executable: http tests
# --------------------------------------------------------------------
add_executable(http_tests
    tests/http_message_tests.cpp
)

target_link_libraries(http_tests
    PRIVATE
        http           # your http library
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(http_tests)
