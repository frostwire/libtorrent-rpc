cmake_minimum_required(VERSION 3.12)
project(libtorrent-rpc)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-padded)
endif()

add_subdirectory(libtorrent)

set(include_files
    session_rpc
    aux/json
    aux/json_types)

set(src_files
    session_rpc
    aux/json_types)

add_library(libtorrent-rpc
    src/$<JOIN:${src_files}, src/>
    include/ltrpc/$<JOIN:${include_files}, include/ltrpc/>)

target_compile_features(libtorrent-rpc PUBLIC cxx_std_11)

target_include_directories(libtorrent-rpc PUBLIC include)

target_link_libraries(libtorrent-rpc PRIVATE torrent-rasterbar)

add_subdirectory(client_test)
