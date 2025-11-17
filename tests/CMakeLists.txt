cmake_minimum_required(VERSION 3.13)

project(learn_llvm_tests LANGUAGES CXX)

# GoogleTest 提供的 CMake 模块
include(GoogleTest)

# 一个针对 pass_hello 的测试
add_executable(test_pass_hello
        test_pass_hello.cpp
)

# 让测试目标能用 GoogleTest
target_link_libraries(test_pass_hello PRIVATE
        GTest::gtest_main
        hello_pass_lib      # 直接复用 HelloPass 库
)


# LLVM 需要的头文件 / 宏已经包含在 llvm_common 里

# 让 CTest 自动发现并注册 gtest 用例
gtest_discover_tests(test_pass_hello)
