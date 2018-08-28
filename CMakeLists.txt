cmake_minimum_required(VERSION 3.12)
project(libtorrent-rpc)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic)
endif()

add_subdirectory(libtorrent)

set(include_files
    session_rcp)

set(src_files
    session_rcp)

add_library(libtorrent-rpc
    src/$<JOIN:${src_files}, src/>
    include/ltrcp/$<JOIN:${include_files}, include/ltrcp/>)

if (cxx_std_14 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    target_compile_features(libtorrent-rpc PUBLIC cxx_std_14)
    message(STATUS "Building in C++14 mode")
else()
    target_compile_features(libtorrent-rpc PUBLIC cxx_std_11)
    message(STATUS "Building in C++11 mode")
endif()

target_include_directories(libtorrent-rpc PUBLIC include)

target_link_libraries(libtorrent-rpc torrent-rasterbar)
