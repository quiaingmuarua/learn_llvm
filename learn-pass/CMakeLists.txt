cmake_minimum_required(VERSION 3.13)

project(learn_pass LANGUAGES CXX)

# 1. HelloPass 库
add_library(hello_pass_lib
        src/HelloPass.cpp
)

target_link_libraries(hello_pass_lib
        PUBLIC llvm_common           # 复用顶层的 LLVM 封装
)

target_include_directories(hello_pass_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 2. CLI 工具：pass_hello
add_executable(pass_hello
        src/pass_hello_main.cpp
)

target_link_libraries(pass_hello
        PRIVATE hello_pass_lib
)


# ----------------- Tests -----------------
# 测试输入 IR 文件
set(PASS_TEST_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/tests/hello_input.ll")

if(EXISTS ${PASS_TEST_INPUT})
    add_test(
            NAME pass_hello_adds_attr
            # 用 TARGET_FILE 确保找到正确的可执行文件路径
            COMMAND $<TARGET_FILE:pass_hello> ${PASS_TEST_INPUT}
    )
    set_tests_properties(pass_hello_adds_attr
            PROPERTIES PASS_REGULAR_EXPRESSION "hello-pass")
else()
    message(WARNING "Test input file not found: ${PASS_TEST_INPUT}")
endif()

