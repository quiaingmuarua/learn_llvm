cmake_minimum_required(VERSION 3.13)
project(learn_llvm LANGUAGES C CXX)

# 所有 target 默认用 -fPIC（生成 .so 时不会再报错）
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 开启 CTest
include(CTest)
enable_testing()

# 不写死 LLVM_DIR，外面传或用环境变量
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVMConfig at ${LLVM_DIR}")

add_library(llvm_common INTERFACE)
target_include_directories(llvm_common INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(llvm_common INTERFACE ${LLVM_DEFINITIONS})

# 这里组件要比之前多一点，因为要用 IRReader + Passes
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


# 让我们的代码在 RTTI / 异常设置上和 LLVM 保持一致
if (DEFINED LLVM_ENABLE_RTTI AND NOT LLVM_ENABLE_RTTI)
    message(STATUS "LLVM built with RTTI disabled; adding -fno-rtti to our targets")
    target_compile_options(llvm_common INTERFACE -fno-rtti)
endif()

if (DEFINED LLVM_ENABLE_EH AND NOT LLVM_ENABLE_EH)
    message(STATUS "LLVM built with exceptions disabled; adding -fno-exceptions to our targets")
    target_compile_options(llvm_common INTERFACE -fno-exceptions)
endif()


add_subdirectory(learn-demo)
add_subdirectory(learn-lang)
add_subdirectory(learn-pass)
add_subdirectory(tests)


include(FetchContent)

if (BUILD_TESTING)  # enable_testing() 会设置这个变量
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )

    # 对旧的 MSVC 选项做点兼容，Linux 这行其实没啥影响
    set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif()

