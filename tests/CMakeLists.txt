cmake_minimum_required(VERSION 3.13)

project(learn_llvm_tests LANGUAGES CXX)

# GoogleTest 提供的 CMake 模块
include(GoogleTest)

# ------- 测试 1：HelloPass -------
add_executable(test_pass_hello
        test_pass_hello.cpp
)
target_link_libraries(test_pass_hello PRIVATE
        GTest::gtest_main
        hello_pass_lib
)
gtest_discover_tests(test_pass_hello)

# ------- 测试 2：Lang Parser -------
add_executable(test_lang_parser
        test_lang_parser.cpp
)
target_link_libraries(test_lang_parser PRIVATE
        GTest::gtest_main
        lang_core_lib
)
gtest_discover_tests(test_lang_parser)

# ------- 测试 3：JunkPass -------
add_executable(test_junk_pass
        test_junk_pass.cpp
)
target_link_libraries(test_junk_pass PRIVATE
        GTest::gtest_main
        hello_pass_lib      # 里面现在也有 JunkPass
)
gtest_discover_tests(test_junk_pass)

# ------- 测试 4：SimpleObfPass -------
add_executable(test_simple_obf_pass
        test_simple_obf_pass.cpp
)
target_link_libraries(test_simple_obf_pass PRIVATE
        GTest::gtest_main
        hello_pass_lib
)
gtest_discover_tests(test_simple_obf_pass)

# ------- 测试 5：FlattenCFPass -------
add_executable(test_flatten_cf_pass
        test_flatten_cf_pass.cpp
)
target_link_libraries(test_flatten_cf_pass PRIVATE
        GTest::gtest_main
        hello_pass_lib
)
gtest_discover_tests(test_flatten_cf_pass)

# ------- Kotoamatsukami Tests -------
add_subdirectory(koto)