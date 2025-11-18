cmake_minimum_required(VERSION 3.13)

project(learn_deobf LANGUAGES CXX)

set(LEARN_DEOBF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(deobf_pass_objs OBJECT
        src/de_hello/DeHelloPass.cpp
)

target_include_directories(deobf_pass_objs
        PUBLIC ${LEARN_DEOBF_INCLUDE_DIR}
        PRIVATE ${LLVM_INCLUDE_DIRS}
)
target_compile_definitions(deobf_pass_objs
        PRIVATE ${LLVM_DEFINITIONS}
)

add_library(deobf_pass_lib SHARED
        $<TARGET_OBJECTS:deobf_pass_objs>
)

target_link_libraries(deobf_pass_lib
        PUBLIC llvm_common
)

target_include_directories(deobf_pass_lib
        PUBLIC ${LEARN_DEOBF_INCLUDE_DIR}
)

if(NOT CMAKE_CROSSCOMPILING)
    include(AddLLVM)
    include(HandleLLVMOptions)

    add_llvm_pass_plugin(learn_llvm_deobf_plugin
            src/de_hello/DeHelloPass.cpp
            src/de_hello/PassPlugin.cpp
    )

    target_include_directories(learn_llvm_deobf_plugin PRIVATE
            ${LEARN_DEOBF_INCLUDE_DIR}
            ${LLVM_INCLUDE_DIRS}
    )
    target_compile_definitions(learn_llvm_deobf_plugin PRIVATE ${LLVM_DEFINITIONS})

    set_target_properties(learn_llvm_deobf_plugin PROPERTIES
            OUTPUT_NAME "learn_llvm_deobf"
    )
else()
    add_library(learn_llvm_deobf_plugin SHARED
            src/de_hello/DeHelloPass.cpp
            src/de_hello/PassPlugin.cpp
    )

    target_include_directories(learn_llvm_deobf_plugin PRIVATE
            ${LEARN_DEOBF_INCLUDE_DIR}
            ${LLVM_INCLUDE_DIRS}
    )
    target_compile_definitions(learn_llvm_deobf_plugin PRIVATE ${LLVM_DEFINITIONS})

    set_target_properties(learn_llvm_deobf_plugin PROPERTIES
            OUTPUT_NAME "learn_llvm_deobf"
            POSITION_INDEPENDENT_CODE ON
            LINK_FLAGS "-Wl,--unresolved-symbols=ignore-all -Wl,--allow-shlib-undefined"
    )
endif()

add_executable(pass_deobf
        src/de_hello/pass_deobf_main.cpp
)

target_link_libraries(pass_deobf
        PRIVATE deobf_pass_lib
)

set(DEOBF_TEST_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/tests/deobf_input.ll")

if(EXISTS ${DEOBF_TEST_INPUT})
    add_test(
            NAME deobf_recovers_simple_add
            COMMAND $<TARGET_FILE:pass_deobf> ${DEOBF_TEST_INPUT}
    )
    set_tests_properties(deobf_recovers_simple_add
            PROPERTIES PASS_REGULAR_EXPRESSION "add i32 %a, %b")
else()
    message(WARNING "Test input file not found: ${DEOBF_TEST_INPUT}")
endif()