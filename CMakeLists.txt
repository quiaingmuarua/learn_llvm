cmake_minimum_required(VERSION 3.13)
project(learn_llvm LANGUAGES C CXX)

# --------- 统一输出目录：bin / lib 在顶层 build 下 ----------
# 例如: learn_llvm/build/bin, learn_llvm/build/lib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   # 静态库
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   # 动态库
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)   # 可执行文件

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# --------- 找 LLVM ----------
set(LLVM_DIR "/root/open_source/llvm-build/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# --------- 定义一个公共的 llvm_common 目标，给子模块复用 ----------
add_library(llvm_common INTERFACE)

# 头文件 & 宏
target_include_directories(llvm_common INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(llvm_common INTERFACE ${LLVM_DEFINITIONS})

# 需要的组件可以先简单点，后面不够再加
set(LLVM_COMPONENTS
        core
        support
)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(llvm_common INTERFACE ${LLVM_LIBS})

# --------- 子目录：demo / lang / pass ----------
add_subdirectory(learn-demo)
add_subdirectory(learn-lang)
add_subdirectory(learn-pass)
