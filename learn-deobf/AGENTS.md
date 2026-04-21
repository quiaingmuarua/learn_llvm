# learn-deobf — Agent Guide

> See root `CLAUDE.md` for full project context and build instructions.

## Purpose

Implements LLVM passes that **reverse** the obfuscations applied by the
Kotoamatsukami plugin. Currently early-stage; only `DeHelloPass` (recovering
`SimpleObfPass` transformations) is implemented.

## When To Enter This Module

Use `learn-deobf` when the task is about:

- restoring simpler IR from obfuscated IR
- matching patterns emitted by `learn-pass`
- the standalone deobf runner or plugin entry

If you are changing the obfuscation side and the deobfuscation tests break,
read this module guide after you finish the pass-side change.

## Read First

- `learn-deobf/CMakeLists.txt`
- `learn-deobf/src/de_hello/DeHelloPass.cpp`
- `learn-deobf/src/de_hello/PassPlugin.cpp`
- `learn-deobf/tests/deobf_input.ll`

## Common Edit Paths

| Goal | Files |
|---|---|
| Add a new deobf pass | `include/learn_llvm/de_*`, `src/de_*/` |
| Register pass name | `src/de_*/PassPlugin.cpp` |
| Update fixture IR | `learn-deobf/tests/` |
| Adjust CLI runner | `src/de_hello/pass_deobf_main.cpp` |

## Verify

- Preferred: `./build.sh test`
- Narrow check: `ctest -R deobf --output-on-failure`

---

## Directory Layout

```
learn-deobf/
├── CMakeLists.txt
├── include/learn_llvm/
│   └── de_hello/
│       └── DeHelloPass.h
├── src/de_hello/
│   ├── DeHelloPass.cpp       ← deobfuscation logic
│   ├── PassPlugin.cpp        ← plugin entry for learn_llvm_deobf shared module
│   └── pass_deobf_main.cpp   ← standalone CLI runner
└── tests/
    └── deobf_input.ll        ← test IR fixture (obfuscated input)
```

---

## Build Outputs

| Target | Output | Use |
|---|---|---|
| `learn_llvm_deobf_plugin` | `build/lib/learn_llvm_deobf.{so,dylib}` | Load via `opt -load-pass-plugin` |
| `deobf_pass_lib` | `build/lib/libdeobf_pass_lib.{so,dylib}` | Shared lib for programmatic use |
| `pass_deobf` | `build/bin/pass_deobf` | CLI runner |

---

## Using the Deobfuscation Plugin

```bash
# Deobfuscate an IR file:
PLUGIN_EXT=so   # macOS: dylib
opt -load-pass-plugin ./build/lib/learn_llvm_deobf.${PLUGIN_EXT} \
    -passes=de-hello \
    -S ./learn-deobf/tests/deobf_input.ll \
    -o /tmp/restored.ll
```

---

## Adding a New Deobfuscation Pass

To add `DeMyObf` that reverses the `my-pass` obfuscation:

1. `include/learn_llvm/de_myobf/DeMyObfPass.h` — declare pass class
2. `src/de_myobf/DeMyObfPass.cpp` — implement recovery logic
3. `src/de_myobf/PassPlugin.cpp` — register with pass name `de-myobf`
4. `CMakeLists.txt` — add sources to `deobf_pass_objs` and `learn_llvm_deobf_plugin`

### Pass skeleton

```cpp
// DeMyObfPass.h
#pragma once
#include "llvm/IR/PassManager.h"
class DeMyObfPass : public llvm::PassInfoMixin<DeMyObfPass> {
public:
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};
```

---

## LLVM Version Compatibility

`StringRef` API changed in LLVM 16: `startswith` → `starts_with`.
Use the guard already established in `DeHelloPass.cpp`:

```cpp
#include <llvm/Config/llvm-config.h>
#if LLVM_VERSION_MAJOR >= 16
    Name.starts_with("prefix")
#else
    Name.startswith("prefix")
#endif
```

Apply this pattern in any new pass that uses `StringRef` predicates.

---

## Tests

The CTest test `deobf_recovers_simple_add` runs `pass_deobf` against
`tests/deobf_input.ll` and checks the output contains `add i32 %a, %b`
(the recovered original instruction).

Run: `cd build && ctest -R deobf`
