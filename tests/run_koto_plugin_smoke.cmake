# run_koto_plugin_smoke.cmake
# Smoke test: compile example/test.c via clang with -fpass-plugin=Kotoamatsukami
#
# Required variables (passed via cmake -D):
#   CLANG_EXE      - path to clang
#   PLUGIN_FILE    - path to Kotoamatsukami.{so,dylib}
#   SOURCE_FILE    - path to the C source to compile
#   OUTPUT_FILE    - destination binary
#   WORKING_DIR    - project root (plugin reads Kotoamatsukami.config from here)

file(MAKE_DIRECTORY "${WORKING_DIR}")

execute_process(
    COMMAND "${CLANG_EXE}"
            -fpass-plugin=${PLUGIN_FILE}
            -O1
            "${SOURCE_FILE}"
            -o "${OUTPUT_FILE}"
    WORKING_DIRECTORY "${WORKING_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE  stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "plugin_koto_clang_smoke FAILED (exit ${result})\n"
        "stderr:\n${stderr}")
endif()

message(STATUS "plugin_koto_clang_smoke PASSED")
