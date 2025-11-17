cmake_minimum_required(VERSION 3.13)

project(learn_pass LANGUAGES CXX)

file(GLOB PASS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(sourcefile ${PASS_SOURCES})
    get_filename_component(exename ${sourcefile} NAME_WE)

    add_executable(${exename} ${sourcefile})

    target_link_libraries(${exename} PRIVATE llvm_common)
endforeach()



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

