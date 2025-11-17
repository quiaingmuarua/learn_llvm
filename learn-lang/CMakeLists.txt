cmake_minimum_required(VERSION 3.13)

project(learn_lang LANGUAGES CXX)

# 1. 核心库：Lexer + Parser + AST
add_library(lang_core_lib
        src/AST.cpp
        src/Lexer.cpp
        src/Parser.cpp
)

target_include_directories(lang_core_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 注意：这里暂时不需要链接 llvm_common，
# 因为目前我们只做前端，不用 LLVM API，
# 这样 RTTI/异常可以保持默认，以便测试里用 dynamic_cast。

# 2. REPL 工具
add_executable(lang_repl
        src/lang_repl_main.cpp
)

target_link_libraries(lang_repl
        PRIVATE lang_core_lib
)
