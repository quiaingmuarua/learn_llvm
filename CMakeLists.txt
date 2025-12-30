cmake_minimum_required(VERSION 3.17)

# =========================================================
# Project
# =========================================================
project(learn_llvm LANGUAGES C CXX)

# =========================================================
# Global build settings
# =========================================================
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# =========================================================
# Testing (CTest provides BUILD_TESTING option)
# =========================================================
include(CTest)      # defines BUILD_TESTING option
enable_testing()

# =========================================================
# LLVM (installed LLVM via LLVMConfig.cmake)
# =========================================================
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVMConfig at ${LLVM_DIR}")
message(STATUS "LLVM CMake dir at ${LLVM_CMAKE_DIR}")

# Make LLVM CMake modules (AddLLVM/TableGen/...) available
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include(TableGen)
include(HandleLLVMOptions)

# Locate llvm-tblgen for TableGen.cmake
get_filename_component(LLVM_INSTALL_PREFIX "${LLVM_DIR}/../.." ABSOLUTE)

find_program(LLVM_TABLEGEN_EXE
        NAMES llvm-tblgen llvm-tblgen-17
        HINTS "${LLVM_INSTALL_PREFIX}/bin" "/usr/local/bin" "/usr/bin"
)

if (NOT LLVM_TABLEGEN_EXE)
    message(FATAL_ERROR
            "llvm-tblgen not found. Install/build llvm-tblgen or set LLVM_TABLEGEN_EXE.")
endif()

message(STATUS "Using llvm-tblgen: ${LLVM_TABLEGEN_EXE}")

# A common interface target for your code to link against LLVM
add_library(llvm_common INTERFACE)
target_include_directories(llvm_common INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(llvm_common INTERFACE ${LLVM_DEFINITIONS})

set(LLVM_COMPONENTS
        core
        support
        irreader
        asmparser
        analysis
        passes
        transformutils
)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(llvm_common INTERFACE ${LLVM_LIBS})

# Match LLVM build flags regarding RTTI / exceptions
if (DEFINED LLVM_ENABLE_RTTI AND NOT LLVM_ENABLE_RTTI)
    message(STATUS "LLVM built with RTTI disabled; adding -fno-rtti to our targets")
    target_compile_options(llvm_common INTERFACE -fno-rtti)
endif()

if (DEFINED LLVM_ENABLE_EH AND NOT LLVM_ENABLE_EH)
    message(STATUS "LLVM built with exceptions disabled; adding -fno-exceptions to our targets")
    target_compile_options(llvm_common INTERFACE -fno-exceptions)
endif()

# =========================================================
# Dependencies (FetchContent)
# =========================================================
include(FetchContent)

if (BUILD_TESTING)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    # harmless on Linux; keeps older MSVC happy
    set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# =========================================================
# Subdirectories
# =========================================================
add_subdirectory(learn-demo)
add_subdirectory(learn-lang)
add_subdirectory(learn-pass)
add_subdirectory(tests)
add_subdirectory(learn-deobf)
