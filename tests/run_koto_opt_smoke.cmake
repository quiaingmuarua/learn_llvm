# run_koto_opt_smoke.cmake
# Smoke test: run opt with Kotoamatsukami plugin and verify output content.
#
# Required variables (passed via cmake -D):
#   OPT_EXE          - path to opt
#   PLUGIN_FILE      - path to Kotoamatsukami.{so,dylib}
#   PASS_PIPELINE    - comma-separated pass names, e.g. "flatten"
#   INPUT_FILE       - path to input .ll file
#   CONFIG_FILE      - full path to the pass-specific JSON config
#   CONFIG_FILE_NAME - filename of the config (e.g. koto_flatten_smoke_config.json)
#   OUTPUT_FILE      - where to write the obfuscated .ll
#   WORKING_DIR      - scratch directory; config copied here so plugin can find it
#   EXPECT_MODE      - "contains" | "not_contains"
#   EXPECT_TEXT      - text to look for (or not) in OUTPUT_FILE

# Create scratch dir and drop the per-test config there.
# The plugin searches for Kotoamatsukami.config (by that exact name) in CWD,
# so we rename the smoke config on copy.
file(MAKE_DIRECTORY "${WORKING_DIR}")
file(COPY_FILE "${CONFIG_FILE}" "${WORKING_DIR}/Kotoamatsukami.config")

execute_process(
    COMMAND "${OPT_EXE}"
            -load-pass-plugin ${PLUGIN_FILE}
            -passes=${PASS_PIPELINE}
            -S "${INPUT_FILE}"
            -o "${OUTPUT_FILE}"
    WORKING_DIRECTORY "${WORKING_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE  stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "plugin_koto_opt_smoke (${PASS_PIPELINE}) FAILED (exit ${result})\n"
        "stderr:\n${stderr}")
endif()

file(READ "${OUTPUT_FILE}" obf_ir)

if(EXPECT_MODE STREQUAL "contains")
    if(NOT obf_ir MATCHES "${EXPECT_TEXT}")
        message(FATAL_ERROR
            "plugin_koto_opt_smoke (${PASS_PIPELINE}): "
            "expected '${EXPECT_TEXT}' in output but not found.\n"
            "IR:\n${obf_ir}")
    endif()
    message(STATUS "plugin_koto_opt_smoke (${PASS_PIPELINE}) PASSED — found '${EXPECT_TEXT}'")

elseif(EXPECT_MODE STREQUAL "not_contains")
    if(obf_ir MATCHES "${EXPECT_TEXT}")
        message(FATAL_ERROR
            "plugin_koto_opt_smoke (${PASS_PIPELINE}): "
            "expected '${EXPECT_TEXT}' to be absent but was found.\n"
            "IR:\n${obf_ir}")
    endif()
    message(STATUS "plugin_koto_opt_smoke (${PASS_PIPELINE}) PASSED — '${EXPECT_TEXT}' correctly absent")

else()
    message(FATAL_ERROR "Unknown EXPECT_MODE: ${EXPECT_MODE}")
endif()
