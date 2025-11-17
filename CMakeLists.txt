cmake_minimum_required(VERSION 3.13)
project(learn_llvm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# 输出目录统一到 build/bin & build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 只需要这一行！
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVMConfig at ${LLVM_DIR}")

add_library(llvm_common INTERFACE)
target_include_directories(llvm_common INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(llvm_common INTERFACE ${LLVM_DEFINITIONS})

# 配置需要的 llvm 组件
set(LLVM_COMPONENTS core support)
llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(llvm_common INTERFACE ${LLVM_LIBS})

add_subdirectory(learn-demo)
add_subdirectory(learn-lang)
add_subdirectory(learn-pass)
