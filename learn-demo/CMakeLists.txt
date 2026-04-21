cmake_minimum_required(VERSION 3.13)

project(learn_demo LANGUAGES CXX)

# 自动扫描 src/*.cpp，每个 cpp 生成一个同名可执行文件
file(GLOB DEMO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(sourcefile ${DEMO_SOURCES})
    get_filename_component(exename ${sourcefile} NAME_WE)

    add_executable(${exename} ${sourcefile})

    # 复用顶层的 llvm_common（包含 include/defs/libs）
    target_link_libraries(${exename} PRIVATE llvm_common)
endforeach()


add_subdirectory(llvm_practice/01-llvm-hellowold)
add_subdirectory(llvm_practice/02-expr-compiler)
add_subdirectory(llvm_practice/03-variable)
add_subdirectory(llvm_practice/04-errhandle_unittest)

# One_backend requires a full LLVM source tree (Target.td is not shipped in
# installed LLVM packages). Disabled by default; enable only if you have the
# LLVM source and have set LLVM_MAIN_SRC_DIR.
option(LEARN_LLVM_ENABLE_ONE_BACKEND
    "Build the custom One LLVM backend (requires LLVM source tree)" OFF)
if(LEARN_LLVM_ENABLE_ONE_BACKEND)
    add_subdirectory(llvm_practice/One_backend)
endif()



