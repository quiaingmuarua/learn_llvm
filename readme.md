# 📘 Learn LLVM – 一个系统学习 LLVM 的实践项目

本项目旨在通过 **实际动手写代码** 的方式，系统性学习 LLVM，包括：

* **通过编写一门简单语言来学习 LLVM 前端与 IR**
* **编写 LLVM Pass（含 OLLVM 混淆）来掌握 LLVM 中端优化技术**
* **通过各种小 demo 熟悉 LLVM API 与 IR 构造方式**

这是一个分模块的学习型仓库，适合用于长期积累与完整掌握 LLVM 技术栈。

---

# 📁 项目结构

```
learn_llvm/
├── CMakeLists.txt          # 顶层 CMake，统一管理三大模块
├── build/                  # 构建输出目录（生成的 bin/lib 都在这里）
│
├── learn-demo/             # 零散的小实验、API 演示、IR 构造练习
│   ├── CMakeLists.txt
│   └── src/
│       └── demo_xxx.cpp
│
├── learn-lang/             # 通过“写一门语言”来学习 LLVM
│   ├── CMakeLists.txt
│   └── src/
│       └── xxx.cpp
│
└── learn-pass/             # 编写 LLVM Pass（含 OLLVM 混淆 Pass）
    ├── CMakeLists.txt
    └── src/
        └── pass_xxx.cpp
```

---

# 🎯 三大模块介绍

## 1️⃣ learn-demo：LLVM 小实验

这里用于快速测试 LLVM API，包含：

* 构造 IR（IRBuilder）
* 输出 `.ll` 文件
* 使用模块、函数、基本块、指令
* IR 变量、常量、函数调用等示例
* 各种零碎功能验证（如 C++17 + LLVM API 的组合）

适合作为 LLVM 世界的**实验田、沙盒区**。

---

## 2️⃣ learn-lang：从零写一门语言（Kaleidoscope 路线）

从最小语言开始，逐步实现：

* 词法分析器（Lexer）
* 语法分析器（Parser）
* AST 结构
* 生成 LLVM IR（IRBuilder）
* 控制流（if / for）
* 函数与外部调用
* JIT 执行（ORC JIT）
* 简单优化 Pass（常量折叠等）

最终目标：
**实现一个可运行的小型语言解释器，支持表达式、函数、控制流。**

这是学习 LLVM 前端与 IR 的最佳实践。

---

## 3️⃣ learn-pass：LLVM Pass + OLLVM 混淆

本模块实现 LLVM Pass（基于 new Pass Manager）：

* Hello Pass
* 遍历函数/基本块/指令
* 修改 IR
* 常见 OLLVM 混淆 pass（集中在 `learn-pass/src/Kotoamatsukami`）：

    * 控制流平坦化（CFF）
    * 虚假控制流（Bogus Control Flow）
    * 指令替换（Instruction Substitution）
    * 字符串/常量混淆
    * 其它辅助 Pass：`ForObs`、`SplitBasicBlock`、`AddJunkCode`、`AntiDebug`、`GVEncrypt`、`Loopen` 等

未来可加入：

* 反混淆（Deobfuscation）
* 静态分析（数据流 / CFG / 循环分析）
* SSA 相关操作

---

# 🛠️ 构建方式

在项目根目录执行以下命令：

```bash
cd learn_llvm

# 推荐使用独立构建目录
mkdir -p build
cd build

cmake .. -DLLVM_DIR=/root/open_source/llvm-build/lib/cmake/llvm
cmake --build .
```

构建完成后，所有可执行文件会统一放在：

```
learn_llvm/build/bin/
```

例如：

```
bin/demo_hello_ir
bin/lang_repl
bin/pass_hello
bin/test_pass_hello
bin/test_koto_loopen
bin/test_koto_passes
```

---

# 📌 开发方式说明

### 增加新的 demo：

```
learn-demo/src/demo_new.cpp
```

CMake 会自动检测并生成新的可执行程序。

---

### 增加新的语言模块文件：

```
learn-lang/src/*.cpp
```

同样无需修改 CMake，会自动构建。

---

### 增加新的 Pass：

```
learn-pass/src/pass_xxx.cpp
```

以后加入插件式 Pass 时，我也可以帮你扩展支持 `-fpass-plugin=xxx.so` 的结构。

---

# 🧪 测试说明

项目中已经集成了一批 GoogleTest 用例如下：

| 可执行文件 | 说明 |
|------------|------|
| `bin/test_pass_hello` | 覆盖 `hello_pass_lib` 中 Hello/Junk/SimpleObf/Flatten Pass 的基础行为 |
| `bin/test_koto_loopen` | 验证 `Kotoamatsukami::Loopen` Pass 是否会注入 `Kotoamatsukami_quick_pow` 助手函数 |
| `bin/test_koto_passes` | 批量测试 `ForObs`、`BogusControlFlow`、`SplitBasicBlock`、`AddJunkCode` 等混淆 Pass |

运行方式：

```bash
# 构建后直接运行某个测试
./build/bin/test_koto_passes

# 只运行 Loopen 测试
./build/bin/test_koto_loopen --gtest_filter='*Loopen*'
```

这些测试会在运行时为 `Kotoamatsukami` 生成临时配置文件（位于 `/tmp/koto_test_config_*.json`），互不干扰。实际使用插件时，可手动编辑仓库根目录的 `Kotoamatsukami.config` 来启用/禁用指定的混淆 Pass。

---

# 🚀 Roadmap / 未来计划

### ✔ learn-demo

* 基本 IR 构造（完成）
* 循环/分支 IR 例子
* 读取/打印 `.ll/.bc`
* 使用 PassManager 对单个模块优化

### ✔ learn-lang

* 表达式解析
* 变量与作用域
* 函数定义
* 控制流
* ORC JIT
* 简单优化
* REPL 调试工具

### ✔ learn-pass

* Hello Pass（完成）
* 打印 CFG、Dominator Tree
* 控制流平坦化 Pass
* 字符串加密
* 指令替换
* 虚假控制流
* 反混淆

---

# ✅ 持续集成

GitHub Actions（`.github/workflows/ci.yml`）会在 `push` 与 `pull_request` 触发，流程如下：

1. 安装 Ninja、LLVM/Clang 17
2. 运行 `cmake -S . -B build -DLLVM_DIR=$(llvm-config-17 --cmakedir) -G Ninja`
3. `cmake --build build`
4. `ctest --output-on-failure`

本地若想与 CI 对齐，可以直接执行以上命令。混淆相关的 gtest（`test_koto_*`）目前需要单独运行，上文“测试说明”部分给出了示例。

---