include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(cache_server_tests
    store_tests.cpp
    parser_tests.cpp
    command_processor_tests.cpp
    request_pipeline_tests.cpp
    server_tests.cpp
    store_lru_tests.cpp
)

target_link_libraries(cache_server_tests
    PRIVATE
        cache_server_core
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(
    cache_server_tests
    PROPERTIES
        RUN_SERIAL TRUE
)