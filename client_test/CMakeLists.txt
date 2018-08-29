cmake_minimum_required(VERSION 3.12)
project(client_test)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic)
endif()

add_executable(client_test
    client_test)

target_link_libraries(client_test PRIVATE libtorrent-rpc)
