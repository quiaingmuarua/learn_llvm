cmake_minimum_required(VERSION 3.22.1)
project("ndk_example")

# ---------------------------------------------------------------------------
# Kotoamatsukami obfuscation — two-step IR pipeline
#
# Why two-step instead of -fpass-plugin directly:
#   NDK's prebuilt clang has ABI differences from any independently-built LLVM
#   17 (PGO/BOLT build flags change internal struct layouts). Loading a plugin
#   built against Homebrew/apt LLVM 17 headers crashes inside PassBuilder
#   callback registration.
#
#   Safe path: NDK clang → .bc → host opt+plugin → obf.bc → NDK clang → .o
#   LLVM bitcode format is stable across minor 17.x versions.
# ---------------------------------------------------------------------------

# LLVM17_PREFIX can be set as a CMake variable (-DLLVM17_PREFIX=...) or env var
if(NOT LLVM17_PREFIX AND DEFINED ENV{LLVM17_PREFIX})
    set(LLVM17_PREFIX "$ENV{LLVM17_PREFIX}")
endif()

find_program(HOST_OPT opt
    HINTS
        "${LLVM17_PREFIX}/bin"
        "/opt/homebrew/opt/llvm@17/bin"
        "/usr/lib/llvm-17/bin"
    DOC "Host LLVM opt for obfuscation passes"
)

# Use host (not Android target) extension for the plugin
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_HOST_PLUGIN_EXT ".dylib")
else()
    set(_HOST_PLUGIN_EXT ".so")
endif()
# Project root (5 levels up from ndk_app/app/src/main/cpp)
get_filename_component(KOTO_PROJECT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
set(KOTO_PLUGIN_DEFAULT "${KOTO_PROJECT_ROOT}/build/lib/Kotoamatsukami${_HOST_PLUGIN_EXT}")
set(KOTO_PLUGIN "" CACHE FILEPATH "Kotoamatsukami plugin path")
if(NOT KOTO_PLUGIN AND DEFINED ENV{KOTO_PLUGIN})
    set(KOTO_PLUGIN "$ENV{KOTO_PLUGIN}")
endif()
if(NOT KOTO_PLUGIN)
    set(KOTO_PLUGIN "${KOTO_PLUGIN_DEFAULT}")
endif()

# Default: sbb→bcf→sub→flatten→junk→ic→ib  (measured 54x IR bloat on RC4)
# Adjust via -DKOTO_PASSES="..." or env KOTO_PASSES.
# Lighter option for fast iteration: "bogus-control-flow,substitution"  (~20x)
set(KOTO_PASSES "" CACHE STRING "Comma-separated pass list")
if(NOT KOTO_PASSES AND DEFINED ENV{KOTO_PASSES})
    set(KOTO_PASSES "$ENV{KOTO_PASSES}")
endif()
if(NOT KOTO_PASSES)
    set(KOTO_PASSES "split-basic-block,bogus-control-flow,substitution,flatten,add-junk-code,indirect-call,indirect-branch")
endif()

# ---------------------------------------------------------------------------
# Build obfuscated objects for native-lib.cpp
# ---------------------------------------------------------------------------
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/native-lib.cpp")
set(BC  "${CMAKE_CURRENT_BINARY_DIR}/native-lib.bc")
set(OBC "${CMAKE_CURRENT_BINARY_DIR}/native-lib_obf.bc")
set(OBJ "${CMAKE_CURRENT_BINARY_DIR}/native-lib_obf.o")

# Step 1: .cpp → bitcode  (NDK cross-compiler, target flags intact)
add_custom_command(
    OUTPUT "${BC}"
    COMMAND "${CMAKE_CXX_COMPILER}"
            "--target=${CMAKE_CXX_COMPILER_TARGET}"
            -std=c++17 -O1 -emit-llvm -c
            "${SRC}" -o "${BC}"
    DEPENDS "${SRC}"
    COMMENT "[Koto] native-lib.cpp → IR"
    VERBATIM
)

# Step 2: bitcode → obfuscated bitcode  (host opt + plugin)
if(HOST_OPT AND EXISTS "${KOTO_PLUGIN}")
    message(STATUS "[Koto] opt found: ${HOST_OPT}")
    message(STATUS "[Koto] plugin:    ${KOTO_PLUGIN}")
    message(STATUS "[Koto] passes:    ${KOTO_PASSES}")
    add_custom_command(
        OUTPUT "${OBC}"
        COMMAND "${HOST_OPT}"
                -load-pass-plugin "${KOTO_PLUGIN}"
                "-passes=${KOTO_PASSES}"
                "${BC}" -o "${OBC}"
        DEPENDS "${BC}" "${KOTO_PLUGIN}"
        WORKING_DIRECTORY "${KOTO_PROJECT_ROOT}"
        COMMENT "[Koto] obfuscate (${KOTO_PASSES})"
        VERBATIM
    )
    set(COMPILE_INPUT "${OBC}")
else()
    message(WARNING "[Koto] opt or plugin not found — obfuscation DISABLED")
    if(NOT HOST_OPT)
        message(WARNING "[Koto]   opt not found (set LLVM17_PREFIX env var)")
    endif()
    if(NOT EXISTS "${KOTO_PLUGIN}")
        message(WARNING "[Koto]   plugin not found: ${KOTO_PLUGIN}")
    endif()
    set(COMPILE_INPUT "${BC}")
endif()

# Step 3: obfuscated bitcode → .o  (NDK cross-compiler again)
add_custom_command(
    OUTPUT "${OBJ}"
    COMMAND "${CMAKE_CXX_COMPILER}"
            "--target=${CMAKE_CXX_COMPILER_TARGET}"
            -c "${COMPILE_INPUT}" -o "${OBJ}"
    DEPENDS "${COMPILE_INPUT}"
    COMMENT "[Koto] obf IR → object"
    VERBATIM
)

# Mark as generated so CMake doesn't look for them at configure time
set_source_files_properties("${OBJ}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

add_library(${CMAKE_PROJECT_NAME} SHARED "${OBJ}")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log)
