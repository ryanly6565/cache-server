cmake_minimum_required(VERSION 3.20)

project(CacheServer VERSION 0.1.0 LANGUAGES CXX)

add_library(cache_server_core
    src/store.cpp
    src/request_parser.cpp
    src/command_processor.cpp
    src/server.cpp
)

target_include_directories(cache_server_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(cache_server_core
    PUBLIC
        cxx_std_20
)

add_executable(cache_server
    src/main.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(
    cache_server_core
    PUBLIC
        Threads::Threads
)

target_link_libraries(cache_server
    PRIVATE
        cache_server_core
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cache_server_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(cache_server PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

enable_testing()
add_subdirectory(tests)