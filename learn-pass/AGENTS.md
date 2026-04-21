# learn-pass — Agent Guide

> See root `CLAUDE.md` for full project context and build instructions.

## Purpose

This module contains two LLVM pass libraries:

1. **`hello_pass_lib`** — simple passes for learning (`HelloPass`, `JunkPass`,
   `SimpleObfPass`, `FlattenCFPass`)
2. **`Kotoamatsukami`** — production-grade obfuscation plugin with 13 passes,
   loadable as a shared library via `-fpass-plugin=`

## When To Enter This Module

Use `learn-pass` when the task is about:

- adding or fixing an LLVM pass
- pass plugin registration
- Kotoamatsukami obfuscation behavior
- root config wiring for enabled / disabled passes
- tests that assert transformed IR shape

This is the highest-coupling module in the repo. Many tasks here require edits
in both code and config.

## Read First

- `learn-pass/CMakeLists.txt`
- `learn-pass/src/Kotoamatsukami/PassPlugin.cpp`
- `Kotoamatsukami.config`
- `Kotoamatsukami.config.schema.json`
- the failing test under root `tests/`

## Common Edit Paths

| Goal | Files |
|---|---|
| Add simple learning pass | `include/learn_llvm/hello/`, `src/hello/` |
| Add Koto pass | `include/learn_llvm/Kotoamatsukami/`, `src/Kotoamatsukami/pass/` |
| Register pass name | `src/Kotoamatsukami/PassPlugin.cpp` or `src/hello/PassPlugin.cpp` |
| Change config semantics | `Kotoamatsukami.config`, schema, `utils/config.cpp` |
| Fix IR helper behavior | `src/Kotoamatsukami/utils/` |

## Verify

- Preferred: `./build.sh test`
- Fast checks:
  `ctest -R 'pass_hello|JunkPass|SimpleObf|FlattenCF|Koto' --output-on-failure`
- If plugin-loading behavior changes, also run `./build.sh opt`

---

## Directory Layout

```
learn-pass/
├── CMakeLists.txt
├── include/learn_llvm/
│   ├── hello/                  ← headers for hello_pass_lib passes
│   │   ├── HelloPass.h
│   │   ├── JunkPass.h
│   │   ├── SimpleObfPass.h
│   │   └── FlattenCFPass.h
│   └── Kotoamatsukami/         ← headers for Kotoamatsukami passes
│       ├── config.h            ← KotoConfig struct (parsed from .config JSON)
│       ├── Log.hpp             ← logging macros
│       ├── utils.hpp           ← common IR helpers
│       ├── TaintAnalysis.h
│       ├── jitter.hpp          ← code injection utilities
│       ├── Flatten.h
│       ├── BogusControlFlow.h
│       ├── Substitution.h
│       ├── AddJunkCodePass.h
│       ├── SplitBasicBlock.h
│       ├── AntiDebugPass.h
│       ├── GVEncrypt.h
│       ├── Loopen.hpp
│       ├── ForObsPass.h
│       ├── IndirectBranch.h
│       ├── IndirectCall.h
│       ├── Branch2Call.h
│       └── Branch2Call_32.h
└── src/
    ├── hello/                  ← hello_pass_lib implementations
    │   ├── HelloPass.cpp
    │   ├── JunkPass.cpp
    │   ├── SimpleObfPass.cpp
    │   ├── FlattenCFPass.cpp
    │   └── PassPlugin.cpp      ← plugin entry for learn_llvm_pass shared module
    └── Kotoamatsukami/
        ├── PassPlugin.cpp      ← plugin entry; lists ALL pass names ← READ THIS
        ├── pass/               ← one .cpp per obfuscation pass
        │   ├── Flatten.cpp
        │   ├── BogusControlFlow.cpp
        │   ├── Substitution.cpp
        │   ├── AddJunkCodePass.cpp
        │   ├── SplitBasicBlock.cpp
        │   ├── AntiDebugPass.cpp
        │   ├── GVEncrypt.cpp
        │   ├── Loopen.cpp
        │   ├── ForObsPass.cpp
        │   ├── IndirectBranch.cpp
        │   ├── IndirectCall.cpp
        │   ├── Branch2Call.cpp
        │   └── Branch2Call_32.cpp
        └── utils/
            ├── config.cpp      ← JSON config parsing (reads Kotoamatsukami.config)
            ├── jitter.cpp      ← IR injection helpers
            ├── jitter.hpp
            ├── TaintAnalysis.cpp
            └── utils.cpp
```

---

## Build Outputs

| Target | Output | Use |
|---|---|---|
| `Kotoamatsukami` | `build/lib/Kotoamatsukami.{so,dylib}` | Load via `-fpass-plugin=` or `opt -load-pass-plugin` |
| `hello_pass_lib` | `build/lib/libhello_pass_lib.{so,dylib}` | Shared helper lib linked by tests and CLI tools |
| `learn_llvm_pass` | `build/lib/learn_llvm_pass.{so,dylib}` | Simple pass plugin |
| `pass_hello` | `build/bin/pass_hello` | CLI runner for hello passes |

---

## Pass Pattern (how every pass works)

All Kotoamatsukami passes follow this pattern:

```cpp
// MyPass.h
#pragma once
#include "llvm/IR/PassManager.h"

class MyPass : public llvm::PassInfoMixin<MyPass> {
public:
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

// MyPass.cpp
#include "learn_llvm/Kotoamatsukami/MyPass.h"
#include "learn_llvm/Kotoamatsukami/config.h"   // KotoConfig::get()

using namespace llvm;

PreservedAnalyses MyPass::run(Module &M, ModuleAnalysisManager &) {
    auto &cfg = KotoConfig::get();
    if (!cfg.myPass.enabled) return PreservedAnalyses::all();
    // ... transform M ...
    return PreservedAnalyses::none();
}
```

---

## Registering a New Pass (checklist)

- [ ] Header in `include/learn_llvm/Kotoamatsukami/MyPass.h`
- [ ] Implementation in `src/Kotoamatsukami/pass/MyPass.cpp`
- [ ] `#include` + `else if` branch in `src/Kotoamatsukami/PassPlugin.cpp` (pipeline parsing callback)
- [ ] `MPM.addPass(MyPass())` in `PassPlugin.cpp` (pipeline start callback)
- [ ] Add source file to `CMakeLists.txt` Kotoamatsukami target
- [ ] Add config key to `Kotoamatsukami.config` and schema

---

## Config Integration

Passes read their on/off state via `KotoConfig::get()` defined in
`include/learn_llvm/Kotoamatsukami/config.h`. Config is loaded from the JSON
file whose path is resolved at plugin load time (searches CWD and executable dir
for `Kotoamatsukami.config`).

---

## Tests

Tests live in `tests/` (root-level, not inside learn-pass):

| Binary | What it tests |
|---|---|
| `test_pass_hello` | HelloPass, JunkPass, SimpleObfPass attribute injection |
| `test_junk_pass` | JunkPass instruction count |
| `test_simple_obf_pass` | SimpleObfPass add→helper replacement |
| `test_flatten_cf_pass` | FlattenCFPass CFG structure |
| `test_koto_passes` | ForObs, BogusControlFlow, SplitBasicBlock, AddJunkCode |
| `test_koto_loopen` | Loopen injects `Kotoamatsukami_quick_pow` function |
