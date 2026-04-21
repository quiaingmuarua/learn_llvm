# learn-demo — Agent Guide

> See root `CLAUDE.md` for full project context and build instructions.

## Purpose

A sandbox for experimenting with LLVM APIs. Each `.cpp` file in `src/` is an
independent program. The `llvm_practice/` subdirectory contains numbered
step-by-step exercises building from hello-world to a custom backend.

---

## Directory Layout

```
learn-demo/
├── CMakeLists.txt
├── src/                          ← standalone demos (each .cpp → one binary)
│   ├── demo.cpp
│   ├── demo2.cpp
│   └── ...
└── llvm_practice/
    ├── README.md                 ← overview of all exercises
    ├── 01-llvm-hellowold/        ← IRBuilder basics
    ├── 02-expr-compiler/         ← simple expression compiler
    ├── 03-variable/              ← alloca / load / store
    ├── 04-errhandle_unittest/    ← error handling patterns
    ├── 05-if/                    ← conditional IR
    ├── 06-for/                   ← loop IR
    ├── 07-logical-bit/           ← bitwise ops
    ├── 08-point/                 ← pointer IR
    ├── 09-array/                 ← array IR
    ├── 10-struct/                ← struct IR
    ├── 11-func-globalvar/        ← functions + global vars
    ├── 12-func_with_array_and_struct/
    ├── 13-func_varags/           ← variadic functions
    ├── 14-switch_and_while/
    ├── 15-more_type_and_constant_expr/
    └── One_backend/              ← custom LLVM backend (DISABLED by default)
```

---

## Adding a New Demo

1. Create `learn-demo/src/demo_myname.cpp`
2. Include `llvm_common` headers, write your experiment
3. Rebuild — CMake will auto-discover the file and create `build/bin/demo_myname`
4. No CMakeLists.txt edit needed

### Minimal demo template

```cpp
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

int main() {
    llvm::LLVMContext ctx;
    auto mod = std::make_unique<llvm::Module>("demo", ctx);
    llvm::IRBuilder<> builder(ctx);

    // Create a function: i32 @add(i32 %a, i32 %b)
    auto *i32 = builder.getInt32Ty();
    auto *fnTy = llvm::FunctionType::get(i32, {i32, i32}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "add", mod.get());

    auto *entry = llvm::BasicBlock::Create(ctx, "entry", fn);
    builder.SetInsertPoint(entry);
    auto args = fn->args().begin();
    auto *a = &*args++, *b = &*args;
    builder.CreateRet(builder.CreateAdd(a, b, "sum"));

    mod->print(llvm::outs(), nullptr);
    return 0;
}
```

---

## One_backend (disabled)

The `One_backend` exercise implements a toy LLVM backend using LLVM's TableGen.
It requires the **full LLVM source tree** (not just an installed package) because
`Target.td` is only available in the source. It is guarded behind the CMake
option `LEARN_LLVM_ENABLE_ONE_BACKEND=OFF`. Do not enable it in CI.
