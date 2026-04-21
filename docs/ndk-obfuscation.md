# NDK 混淆集成指南

本文档说明如何将 Kotoamatsukami LLVM 混淆插件集成到 Android NDK 构建流程中。

---

## 工作原理

### 为什么不能直接用 `-fpass-plugin`

理想情况下，你希望这样使用：

```bash
# 这在 NDK 上会崩溃！
clang++ -fpass-plugin=Kotoamatsukami.so ...
```

这种方式在 NDK 上无法工作，原因是 **ABI 不兼容**：
- NDK 26 内置的 clang-17 是用 **PGO + BOLT** 优化编译的 Google 特供版本
- 任何独立构建的 LLVM 17（Homebrew / apt）内部结构的内存布局与之不同
- 加载插件时，`llvm::PassBuilder` 的 `registerPipelineParsingCallback` 会因 `SmallVector` 布局差异而崩溃

### 实际可行方案：两步 IR 流水线

```
                 ┌─────────────────────────────────────────────────────┐
                 │  Host machine                                        │
  native-lib.cpp │                                                      │
       │         │  step 1            step 2               step 3      │
       └─────────►  NDK clang  ──►  host opt      ──►  NDK clang  ──►  .so (AArch64)
                 │  (aarch64 target)  + Kotoamatsukami    (aarch64 target)
                 │  → .bc (bitcode)   → obf.bc            → .o
                 └─────────────────────────────────────────────────────┘
```

**关键洞察**：LLVM bitcode 格式在同一主版本（17.x）内是稳定的。NDK clang 和 host opt 都能读写同一份 bitcode，ABI 差异不影响 IR 层的 pass 处理。

---

## 快速开始

### 前置条件

| 工具 | 版本 | 说明 |
|------|------|------|
| Android NDK | 26.3.11579264 | 在 `ndk_app/app/build.gradle` 中指定 |
| LLVM (host) | 17.x | macOS: `brew install llvm@17`；Linux: apt `llvm-17-dev` |
| CMake | ≥ 3.22 | |

### 构建步骤

```bash
# 1. 先构建 Kotoamatsukami 插件（在项目根目录执行一次）
./build.sh

# 2. NDK CMake 构建（命令行验证用）
NDK_ROOT=~/Library/Android/sdk/ndk/26.3.11579264   # macOS 路径
# Linux: $ANDROID_SDK_ROOT/ndk/26.3.11579264

cmake -S ndk_app/app/src/main/cpp -B build-ndk \
  -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -G Ninja

cmake --build build-ndk

# 3. 验证 JNI 符号和目标架构
file build-ndk/libndk_example.so
llvm-nm -D build-ndk/libndk_example.so | grep "T Java_"
```

### Android Studio / Gradle

标准 `./gradlew assembleDebug` 即可，Gradle 会调用上述 CMake 流程。

---

## 混淆效果数据

以 `example/test.c`（RC4 实现，380 条指令）为基准进行测量：

| Pass 组合 | IR 行数 | 指令数 | 膨胀倍数 | 适用场景 |
|-----------|---------|--------|---------|---------|
| baseline（原始） | 519 | 380 | 1.0x | — |
| `flatten` | 979 | 735 | 1.9x | 轻度，扰乱 CFG |
| `bogus-control-flow` | 2,063 | 1,665 | 4.4x | 中度，插入虚假分支 |
| `substitution` | 3,658 | 3,519 | 9.3x | 中度，替换算术运算 |
| `sbb + bcf` | 2,063 | 1,665 | 4.4x | 中度 |
| `sbb + bcf + sub` | 8,164 | 7,766 | 20.4x | 较强 |
| `sbb + bcf + sub + flatten` | 18,431 | 15,201 | 40.0x | 强度 |
| `sbb + bcf + sub + flatten + junk` | 19,069 | 15,729 | 41.4x | 强度 |
| **`sbb + bcf + sub + flatten + junk + ic + ib`** | **24,111** | **20,732** | **54.6x** | **最强（默认）** |

> 缩写：sbb = split-basic-block，bcf = bogus-control-flow，sub = substitution，
> junk = add-junk-code，ic = indirect-call，ib = indirect-branch

**当前 ndk_app 默认使用最强组合**（54.6x）。

### 各 Pass 作用说明

| Pass | 作用 | 对逆向的干扰 |
|------|------|------------|
| `split-basic-block` | 将基本块切碎 | 增加节点数，降低 CFG 可读性 |
| `bogus-control-flow` | 插入永不执行的分支（opaque predicate） | IDA/Ghidra 无法静态确定真实路径 |
| `substitution` | 用等价算术替换简单运算（`a+b` → `-(~a + ~b) - 1`） | 破坏常量折叠，干扰符号执行 |
| `flatten` | 将 CFG 重写为 switch-dispatcher 循环 | 彻底破坏 if/for/while 结构，反编译结果不可读 |
| `add-junk-code` | 注入死代码计算 | 增加噪声，增大逆向工作量 |
| `indirect-call` | 将直接调用改为函数指针间接调用 | 破坏调用图（call graph），阻止交叉引用 |
| `indirect-branch` | 将直接跳转改为间接跳转 | 阻止 CFG 静态重建 |

---

## 自定义 Pass 组合

通过 CMake 变量覆盖，无需修改代码：

```bash
# 快速迭代：只用 bcf+sub（约 20x 膨胀，编译快）
cmake ... -DKOTO_PASSES="bogus-control-flow,substitution"

# 针对特定场景：只混淆，不影响大小
cmake ... -DKOTO_PASSES="flatten,indirect-call,indirect-branch"

# 使用环境变量（Gradle 友好）
KOTO_PASSES="split-basic-block,bogus-control-flow,flatten" cmake ...
```

通过环境变量指定不同的插件路径：

```bash
KOTO_PLUGIN=/path/to/Kotoamatsukami.so cmake ...
```

### 在 build.gradle 中传递参数

```groovy
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                // 轻量模式（CI 快速构建）
                arguments "-DKOTO_PASSES=bogus-control-flow,substitution"
            }
        }
    }
    buildTypes {
        release {
            externalNativeBuild {
                cmake {
                    // Release 全力混淆
                    arguments "-DKOTO_PASSES=split-basic-block,bogus-control-flow,substitution,flatten,add-junk-code,indirect-call,indirect-branch"
                }
            }
        }
    }
}
```

---

## 注意事项与已知问题

### config 文件查找

Kotoamatsukami 插件在运行时从 **工作目录** 查找 `Kotoamatsukami.config`。
CMakeLists.txt 已通过 `WORKING_DIRECTORY` 将 opt 的工作目录设置为项目根目录，
确保每次构建都能找到正确的配置文件。

如果单独使用 `opt`，需要在项目根目录执行：

```bash
cd /path/to/learn_llvm
opt -load-pass-plugin build/lib/Kotoamatsukami.dylib \
    -passes="flatten" input.bc -o output.bc
```

### pass 的 `model` 标志

`Kotoamatsukami.config` 中每个 pass 有一个 `model` 字段（`0` = 禁用，`1` = 启用）。
即使 `-passes=flatten` 显式指定，若 `flatten.model = 0`，pass 也不会执行。

仓库根目录的 `Kotoamatsukami.config` 默认只启用 `bogusControlFlow` 和 `forObs`。
如需全部开启，可临时修改，或参考 `docs/` 下的全启用示例配置。

### `antiDebug` pass 不建议用于 NDK

`AntiDebugPass` 注入 `ptrace(PTRACE_TRACEME)` 系统调用，在 Android 上会因
SELinux 策略被拦截，且可能导致调试困难。不推荐在 NDK 场景中启用。

### `branch2call` / `branch2call_32`

这两个 pass 设计用于特定平台（64 位 / 32 位 x86），在 ARM 目标上可能产生非法指令，
**不要** 在 NDK AArch64/ARMv7 目标中使用。

### 编译时间

全量 pass（54.6x）会显著增加编译时间。建议：
- **Debug 构建**：只用 `bogus-control-flow`（4.4x，最快）
- **Release 构建**：全量或按需裁剪

---

## 架构支持

NDK CMake 构建支持任意 ABI，只需改 `-DANDROID_ABI=`：

| ABI | 状态 | 说明 |
|-----|------|------|
| `arm64-v8a` | 已验证 | 推荐，现代 Android 主流 |
| `armeabi-v7a` | 理论可用 | 32 位，勿用 `branch2call` |
| `x86_64` | 理论可用 | 模拟器用 |
| `x86` | 理论可用 | 模拟器，勿用 `branch2call` |
