cmake_minimum_required(VERSION 3.13)

project(learn_llvm_tests LANGUAGES CXX)

# GoogleTest 提供的 CMake 模块
include(GoogleTest)

find_program(LLVM_OPT_EXE
        NAMES opt opt-${LLVM_VERSION_MAJOR}
        HINTS "${LLVM_TOOLS_BINARY_DIR}" "${LLVM_TOOL_BIN_DIR}"
)

find_program(LLVM_CLANG_EXE
        NAMES clang clang-${LLVM_VERSION_MAJOR}
        HINTS "${LLVM_TOOLS_BINARY_DIR}" "${LLVM_TOOL_BIN_DIR}"
)

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

# ------- Plugin smoke tests -------
if(LLVM_OPT_EXE)
    add_test(
            NAME plugin_hello_opt_smoke
            COMMAND ${LLVM_OPT_EXE}
            -load-pass-plugin $<TARGET_FILE:learn_llvm_pass_plugin>
            -passes=hello-pass
            -S ${CMAKE_SOURCE_DIR}/learn-pass/tests/hello_input.ll
            -o -
    )
    set_tests_properties(plugin_hello_opt_smoke
            PROPERTIES PASS_REGULAR_EXPRESSION "hello-pass"
    )

    add_test(
            NAME plugin_deobf_opt_smoke
            COMMAND ${LLVM_OPT_EXE}
            -load-pass-plugin $<TARGET_FILE:learn_llvm_deobf_plugin>
            -passes=de-hello
            -S ${CMAKE_SOURCE_DIR}/learn-deobf/tests/deobf_input.ll
            -o -
    )
    set_tests_properties(plugin_deobf_opt_smoke
            PROPERTIES PASS_REGULAR_EXPRESSION "add i32 %a, %b"
    )
else()
    message(WARNING "LLVM opt not found; plugin opt smoke tests are disabled")
endif()

if(LLVM_CLANG_EXE)
    add_test(
            NAME plugin_koto_clang_smoke
            COMMAND ${CMAKE_COMMAND}
            -DCLANG_EXE=${LLVM_CLANG_EXE}
            -DPLUGIN_FILE=$<TARGET_FILE:Kotoamatsukami>
            -DSOURCE_FILE=${CMAKE_SOURCE_DIR}/example/test.c
            -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/bin/koto_plugin_smoke
            -DWORKING_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_koto_plugin_smoke.cmake
    )
else()
    message(WARNING "LLVM clang not found; Koto clang smoke test is disabled")
endif()

if(LLVM_OPT_EXE)
    get_filename_component(KOTO_FLATTEN_CONFIG_NAME
            "${CMAKE_CURRENT_SOURCE_DIR}/koto_flatten_smoke_config.json" NAME)
    add_test(
            NAME plugin_koto_opt_flatten_smoke
            COMMAND ${CMAKE_COMMAND}
            -DOPT_EXE=${LLVM_OPT_EXE}
            -DPLUGIN_FILE=$<TARGET_FILE:Kotoamatsukami>
            -DPASS_PIPELINE=flatten
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/koto_flatten_input.ll
            -DCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/koto_flatten_smoke_config.json
            -DCONFIG_FILE_NAME=${KOTO_FLATTEN_CONFIG_NAME}
            -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/tests/koto_flatten_smoke.ll
            -DWORKING_DIR=${CMAKE_BINARY_DIR}/tests/koto_flatten_smoke
            -DEXPECT_MODE=contains
            -DEXPECT_TEXT=dispatchBB
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_koto_opt_smoke.cmake
    )

    get_filename_component(KOTO_SUBSTITUTION_CONFIG_NAME
            "${CMAKE_CURRENT_SOURCE_DIR}/koto_substitution_smoke_config.json" NAME)
    add_test(
            NAME plugin_koto_opt_substitution_smoke
            COMMAND ${CMAKE_COMMAND}
            -DOPT_EXE=${LLVM_OPT_EXE}
            -DPLUGIN_FILE=$<TARGET_FILE:Kotoamatsukami>
            -DPASS_PIPELINE=substitution
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/koto_substitution_input.ll
            -DCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/koto_substitution_smoke_config.json
            -DCONFIG_FILE_NAME=${KOTO_SUBSTITUTION_CONFIG_NAME}
            -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/tests/koto_substitution_smoke.ll
            -DWORKING_DIR=${CMAKE_BINARY_DIR}/tests/koto_substitution_smoke
            -DEXPECT_MODE=not_contains
            "-DEXPECT_TEXT=%sum = add i32 %a, %b"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_koto_opt_smoke.cmake
    )
endif()
