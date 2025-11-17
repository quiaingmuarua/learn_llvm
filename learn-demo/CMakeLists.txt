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
