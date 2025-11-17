cmake_minimum_required(VERSION 3.13)

project(learn_pass LANGUAGES CXX)

# 0. 头文件路径（方便复用）
set(LEARN_PASS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1. OBJECT 库：只包含 Pass 实现的 .o，不链接任何 LLVM 库
add_library(hello_pass_objs OBJECT
        src/HelloPass.cpp
)

target_include_directories(hello_pass_objs
        PUBLIC ${LEARN_PASS_INCLUDE_DIR}
)
target_link_libraries(hello_pass_objs
        PUBLIC llvm_common
)

# 2. 静态库：给 CLI 工具 / 测试用，正常链接 llvm_common
add_library(hello_pass_lib STATIC
        $<TARGET_OBJECTS:hello_pass_objs>
)

target_link_libraries(hello_pass_lib
        PUBLIC llvm_common
)

target_include_directories(hello_pass_lib
        PUBLIC ${LEARN_PASS_INCLUDE_DIR}
)

# 3. 插件 .so：只用我们的 OBJECT 文件，不再链接 llvm_common（也就是不静态地带一整套 LLVM）
add_library(learn_llvm_pass_plugin SHARED
        $<TARGET_OBJECTS:hello_pass_objs>
        src/PassPlugin.cpp
)

target_include_directories(learn_llvm_pass_plugin
        PRIVATE ${LEARN_PASS_INCLUDE_DIR}
)

# 不要 target_link_libraries(learn_llvm_pass_plugin PRIVATE llvm_common)
# 让所有 llvm:: 符号去 opt/clanɡ 本体里解析，避免重复加载 LLVM

set_target_properties(learn_llvm_pass_plugin PROPERTIES
        OUTPUT_NAME "learn_llvm_pass"
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

