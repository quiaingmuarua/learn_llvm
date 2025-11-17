cmake_minimum_required(VERSION 3.13)

project(learn_pass LANGUAGES CXX)

file(GLOB PASS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(sourcefile ${PASS_SOURCES})
    get_filename_component(exename ${sourcefile} NAME_WE)

    add_executable(${exename} ${sourcefile})

    target_link_libraries(${exename} PRIVATE llvm_common)
endforeach()
