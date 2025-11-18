cmake_minimum_required(VERSION 3.13)

project(learn_pass LANGUAGES CXX)

# 0. 头文件路径（方便复用）
set(LEARN_PASS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1. OBJECT 库：只包含 Pass 实现的 .o，不链接任何 LLVM 库
add_library(hello_pass_objs OBJECT
        src/HelloPass.cpp
)

target_include_directories(hello_pass_objs
        PUBLIC ${LEARN_PASS_INCLUDE_DIR}
)
target_link_libraries(hello_pass_objs
        PUBLIC llvm_common
)

# 2. 静态库：给 CLI 工具 / 测试用，正常链接 llvm_common
add_library(hello_pass_lib STATIC
        $<TARGET_OBJECTS:hello_pass_objs>
)

target_link_libraries(hello_pass_lib
        PUBLIC llvm_common
)

target_include_directories(hello_pass_lib
        PUBLIC ${LEARN_PASS_INCLUDE_DIR}
)

# 3. 插件 .so：只用我们的 OBJECT 文件，不再链接 llvm_common（也就是不静态地带一整套 LLVM）
add_library(learn_llvm_pass_plugin SHARED
        $<TARGET_OBJECTS:hello_pass_objs>
        src/PassPlugin.cpp
)

target_include_directories(learn_llvm_pass_plugin
        PRIVATE
        ${LEARN_PASS_INCLUDE_DIR}
        ${LLVM_INCLUDE_DIRS}
)
# 需要 LLVM 的编译宏，但仍避免链接静态库
target_compile_definitions(learn_llvm_pass_plugin PRIVATE ${LLVM_DEFINITIONS})

# 不要 target_link_libraries(learn_llvm_pass_plugin PRIVATE llvm_common)
# 让所有 llvm:: 符号去 opt/clanɡ 本体里解析，避免重复加载 LLVM

set_target_properties(learn_llvm_pass_plugin PROPERTIES
        OUTPUT_NAME "learn_llvm_pass"
)



# 2. CLI 工具：pass_hello
add_executable(pass_hello
        src/pass_hello_main.cpp
)

target_link_libraries(pass_hello
        PRIVATE hello_pass_lib
)


# 源文件列表
set(KOTOAMATSUKAMI_SOURCES
        src/Kotoamatsukami/PassPlugin.cpp
        src/Kotoamatsukami/pass/AddJunkCodePass.cpp
        src/Kotoamatsukami/pass/Branch2Call.cpp
        src/Kotoamatsukami/pass/Branch2Call_32.cpp
        src/Kotoamatsukami/pass/ForObsPass.cpp
        src/Kotoamatsukami/pass/Loopen.cpp
        src/Kotoamatsukami/pass/AntiDebugPass.cpp
        src/Kotoamatsukami/pass/SplitBasicBlock.cpp
        src/Kotoamatsukami/pass/IndirectBranch.cpp
        src/Kotoamatsukami/pass/IndirectCall.cpp
        src/Kotoamatsukami/pass/BogusControlFlow.cpp
        src/Kotoamatsukami/pass/Substitution.cpp
        src/Kotoamatsukami/pass/Flatten.cpp
        src/Kotoamatsukami/pass/GVEncrypt.cpp
        src/Kotoamatsukami/utils/utils.cpp
        src/Kotoamatsukami/utils/config.cpp
        src/Kotoamatsukami/utils/TaintAnalysis.cpp
)

if(CMAKE_CROSSCOMPILING)
    # ARM64交叉编译：创建插件，手动控制链接过程避免依赖冲突
    message(STATUS "ARM64 cross-compilation: Creating plugin with manual library linking")

    # 不使用HandleLLVMOptions避免额外依赖检查
    add_library(Kotoamatsukami SHARED ${KOTOAMATSUKAMI_SOURCES})

    # 设置为LLVM插件的标准属性
    set_target_properties(Kotoamatsukami PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
            # 关键：允许未定义符号，运行时由opt工具解析
            LINK_FLAGS "-Wl,--unresolved-symbols=ignore-all -Wl,--allow-shlib-undefined"
    )

    # 添加LLVM编译定义和包含路径
    target_compile_definitions(Kotoamatsukami PRIVATE ${LLVM_DEFINITIONS})
    target_include_directories(Kotoamatsukami PRIVATE ${LLVM_INCLUDE_DIRS} ${LEARN_PASS_INCLUDE_DIR})

    # 根据json库类型决定是否链接target (ARM64交叉编译)
    if(NOT JSON_LOCAL_AVAILABLE)
        # 检查目标是否存在
        if(TARGET nlohmann_json::nlohmann_json)
            target_link_libraries(Kotoamatsukami PRIVATE nlohmann_json::nlohmann_json)
        elseif(TARGET nlohmann_json)
            target_link_libraries(Kotoamatsukami PRIVATE nlohmann_json)
        endif()
        # 无论目标是否存在，都添加 json 的包含目录（FetchContent 下载的路径）
        if(EXISTS ${CMAKE_BINARY_DIR}/_deps/json-src/include)
            target_include_directories(Kotoamatsukami PRIVATE ${CMAKE_BINARY_DIR}/_deps/json-src/include)
        endif()
    endif()

    # ARM64交叉编译时不链接LLVM库，运行时由opt工具提供符号解析

else()
    # x86_64本机编译：使用标准LLVM插件方法
    include(AddLLVM)
    include(HandleLLVMOptions)

    add_llvm_pass_plugin(Kotoamatsukami ${KOTOAMATSUKAMI_SOURCES})

    # 手动添加包含路径（add_llvm_pass_plugin 可能不会自动设置）
    target_include_directories(Kotoamatsukami PRIVATE ${LEARN_PASS_INCLUDE_DIR})

    # 根据json库类型决定是否链接target (x86_64本地编译)
    if(NOT JSON_LOCAL_AVAILABLE)
        # 检查目标是否存在
        if(TARGET nlohmann_json::nlohmann_json)
            target_link_libraries(Kotoamatsukami PRIVATE nlohmann_json::nlohmann_json)
        elseif(TARGET nlohmann_json)
            target_link_libraries(Kotoamatsukami PRIVATE nlohmann_json)
        endif()
        # 无论目标是否存在，都添加 json 的包含目录（FetchContent 下载的路径）
        if(EXISTS ${CMAKE_BINARY_DIR}/_deps/json-src/include)
            target_include_directories(Kotoamatsukami PRIVATE ${CMAKE_BINARY_DIR}/_deps/json-src/include)
        endif()
    endif()
endif()



# ----------------- Tests -----------------
# 测试输入 IR 文件
set(PASS_TEST_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/tests/hello_input.ll")

if(EXISTS ${PASS_TEST_INPUT})
    add_test(
            NAME pass_hello_adds_attr
            # 用 TARGET_FILE 确保找到正确的可执行文件路径
            COMMAND $<TARGET_FILE:pass_hello> ${PASS_TEST_INPUT}
    )
    set_tests_properties(pass_hello_adds_attr
            PROPERTIES PASS_REGULAR_EXPRESSION "hello-pass")
else()
    message(WARNING "Test input file not found: ${PASS_TEST_INPUT}")
endif()

