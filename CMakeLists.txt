cmake_minimum_required(VERSION 3.16)

project(simple_http
    LANGUAGES CXX
)

# Use C++20 (or C++17 if you prefer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
