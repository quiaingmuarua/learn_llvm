# learn-pass 模块说明

该目录用于集中练习 LLVM Pass，当前包含以下 pass：

| Pass | 作用 | 备注 |
|------|------|------|
| `HelloPass` | 给所有有定义的函数添加 `hello-pass` 属性 | 主要用于验证 Pass pipeline |
| `JunkPass` | 在函数入口插入两条未使用算术（`junk1`、`junk2`） | 方便肉眼观察插件已生效 |
| `SimpleObfPass` | 将整数 `add` 替换为调用内部 helper；helper 使用 bitwise 加法循环 + 假分支 + 噪声 | 产生更明显的控制流噪声但保持语义 |

## 构建

```bash
cd /root/open_source/learn_llvm
cmake --build .
```

生成内容：

- `lib/liblearn_llvm_pass.so`：提供给 `opt` 或 `clang -fpass-plugin` 动态加载。
- `lib/libhello_pass_lib.a`：静态库，供测试或 `bin/pass_hello` 使用。

## 快速体验

### 使用 `opt`
```bash
/root/open_source/llvm-build/bin/opt \
  -load-pass-plugin ./lib/liblearn_llvm_pass.so \
  -passes=hello-pass,junk-pass,simple-obf \
  -S ./learn-pass/tests/hello_input.ll \
  -o ./tmp/out_obf.ll
```

### 使用 `clang -fpass-plugin`
```bash
/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./lib/liblearn_llvm_pass.so \
  -fPIC -shared \
  ./example/test.c \
  -o ./tmp/libexample_arm64.so
```

编译出的 `.so` / 可执行在 IDA 中查看，可以看到 `SimpleObfPass` 注入的位运算、循环和假分支。

## 单元测试

`tests/` 目录提供了 GoogleTest：

- `test_pass_hello.cpp`
- `test_junk_pass.cpp`
- `test_simple_obf_pass.cpp`

执行：

```bash
cd /root/open_source/learn_llvm
ctest -R simple_obf_pass   # 或 ctest 运行所有测试
```

## 开发提示

- 新增 Function Pass 时，参考现有 `HelloPass`/`JunkPass` 的模式，实现 `run` 并在 `PassPlugin.cpp` 注册。
- 目前 `SimpleObfPass` 基于 helper 函数实现；如需更强混淆，可继续扩展 helper（opaque predicate、内存访存等）或编写 Machine Pass。
- 若希望在 `-O0` 下也执行 Pass，可调整 `PassPlugin.cpp` 中 `registerOptimizerLastEPCallback` 的 `OptimizationLevel` 条件。

记得在此文档登记新增 Pass/工具链命令，便于回顾。***
