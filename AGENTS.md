# AGENTS.md — Agent Guide for `learn_llvm`

> **Start here.** This file is the authoritative reference for any AI agent
> working in this repository. Read it fully before making changes.

---

## 1. Project Purpose

A hands-on LLVM monorepo for learning compiler internals through writing real
code. Four modules cover the full pipeline: toy language frontend → IR passes →
obfuscation plugin → deobfuscation analysis.

---

## 2. Agent Fast Start

If you are a new agent or a fresh sub-agent, do this in order:

1. Read this file once.
2. Read the module-local guide for the area you will touch:
   - `learn-demo/AGENTS.md`
   - `learn-lang/AGENTS.md`
   - `learn-pass/AGENTS.md`
   - `learn-deobf/AGENTS.md`
3. Run `./build.sh test` before or after changes unless the task is doc-only.
4. If the task touches pass registration, config, or plugin loading, also inspect:
   - `learn-pass/src/Kotoamatsukami/PassPlugin.cpp`
   - `Kotoamatsukami.config`
   - `Kotoamatsukami.config.schema.json`

### Task Router

| If the task is about... | Read this first | Usually edit here |
|---|---|---|
| LLVM API experiments / numbered exercises | `learn-demo/AGENTS.md` | `learn-demo/src/`, `learn-demo/llvm_practice/` |
| Lexer / Parser / toy language REPL | `learn-lang/AGENTS.md` | `learn-lang/include/`, `learn-lang/src/repl/` |
| New pass / pass plugin / obfuscation config | `learn-pass/AGENTS.md` | `learn-pass/include/`, `learn-pass/src/`, root config files |
| Deobfuscation / IR recovery | `learn-deobf/AGENTS.md` | `learn-deobf/include/`, `learn-deobf/src/` |
| Test failures / behavior checks | this file + owning module guide | `tests/`, module sources it exercises |

### Five-Minute Onboarding

- Confirm LLVM version first: `llvm-config --version`
- Prefer repo helper build: `./build.sh` or `./build.sh test`
- The repo is centered on LLVM 17; 14–17 are tested, 22+ is not a target
- New passes are not complete until registration, config wiring, and tests are updated together
- Most cross-module regressions show up in root `tests/`, not inside the module directory

For fast onboarding and handoff, see `docs/AGENT_QUICKSTART.md`.

---

## 3. Repository Map

```
learn_llvm/
├── AGENTS.md                          ← you are here (agent entry point)
├── CMakeLists.txt                     ← top-level CMake (ties all modules)
├── Kotoamatsukami.config              ← JSON: enable/disable obfuscation passes
├── Kotoamatsukami.config.schema.json  ← JSON Schema for the above
├── build.sh                           ← local dev helper (auto-detects LLVM)
├── example/test.c                     ← RC4 implementation, obfuscation target
│
├── learn-demo/      AGENTS.md inside  ← LLVM API experiments
│   ├── src/                           ← one .cpp → one binary (auto-discovered)
│   └── llvm_practice/                 ← numbered exercises 01–15 + One_backend
│
├── learn-lang/      AGENTS.md inside  ← toy language compiler
│   ├── include/                       ← AST/Lexer/Parser/CodeGen headers
│   └── src/repl/                      ← impl + lang_repl_main.cpp
│
├── learn-pass/      AGENTS.md inside  ← LLVM passes + Kotoamatsukami plugin
│   ├── include/learn_llvm/
│   │   ├── hello/                     ← simple pass headers
│   │   └── Kotoamatsukami/            ← obfuscation pass headers + config.h
│   └── src/
│       ├── hello/                     ← HelloPass, JunkPass, SimpleObf, Flatten
│       └── Kotoamatsukami/
│           ├── PassPlugin.cpp         ← plugin entry (ALL pass names live here)
│           ├── pass/                  ← 13 obfuscation pass .cpp files
│           └── utils/                 ← config, jitter, TaintAnalysis, utils
│
├── learn-deobf/     AGENTS.md inside  ← deobfuscation passes
│   ├── include/
│   └── src/de_hello/
│
└── tests/                             ← GoogleTest suites (link pass libs)
    ├── test_pass_hello.cpp
    ├── test_junk_pass.cpp
    ├── test_simple_obf_pass.cpp
    ├── test_flatten_cf_pass.cpp
    └── koto/
        ├── test_loopen.cpp
        └── test_passes.cpp
```

---

## 4. Build

### Requirements

| Requirement | Version | Notes |
|---|---|---|
| CMake | ≥ 3.17 | |
| Ninja | any | recommended over Make |
| LLVM (installed) | 17 preferred, 14–17 tested | On macOS use Homebrew `llvm@17`; plain `llvm` may install LLVM 22+ and break the build |
| C++ compiler | C++17 | Linux: prefer `g++`; macOS: prefer `clang/clang++` from `llvm@17` |
| Internet (first build only) | — | FetchContent pulls googletest v1.15.2 + nlohmann/json v3.11.3 |

### Recommended local build

```bash
./build.sh
./build.sh test
```

`build.sh` auto-detects installed LLVM. On macOS it prefers Homebrew `llvm@17`
and falls back to other installed versions only if needed. On Linux/WSL it also
checks common distro layouts such as `/usr/lib/llvm-17/bin/llvm-config` and
keeps GNU compilers preferred by default. Explicit `LLVM_CONFIG`, `LLVM_DIR`,
`CC`, and `CXX` overrides are respected.

### Manual build

```bash
cmake -S . -B build \
  -DLLVM_DIR=$(llvm-config --cmakedir) \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -G Ninja
cmake --build build
```

> If `clang++` fails with `ld: cannot find -lstdc++`, install
> `libstdc++-11-dev` or switch to `g++`.

### macOS (recommended)

```bash
xcode-select --install
brew install cmake ninja llvm@17

LLVM17_PREFIX=$(brew --prefix llvm@17)
cmake -S . -B build \
  -DLLVM_DIR="$(${LLVM17_PREFIX}/bin/llvm-config --cmakedir)" \
  -DCMAKE_C_COMPILER="${LLVM17_PREFIX}/bin/clang" \
  -DCMAKE_CXX_COMPILER="${LLVM17_PREFIX}/bin/clang++" \
  -G Ninja
cmake --build build
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `LLVM_DIR` | *(required)* | From `llvm-config --cmakedir` |
| `BUILD_TESTING` | `ON` | Build GoogleTest binaries |
| `LEARN_LLVM_ENABLE_ONE_BACKEND` | `OFF` | Requires full LLVM source tree; never enable in CI |

### Build outputs

| Path | What |
|---|---|
| `build/bin/` | All executables |
| `build/lib/Kotoamatsukami.{so,dylib}` | Obfuscation plugin for `-fpass-plugin=` |
| `build/lib/learn_llvm_pass.{so,dylib}` | Simple pass plugin |
| `build/lib/learn_llvm_deobf.{so,dylib}` | Deobfuscation plugin |
| `build/lib/libhello_pass_lib.{so,dylib}` | Shared helper lib linked by tests and CLI tools |

### Run tests

```bash
cd build && ctest --output-on-failure
# Specific suites:
./bin/test_pass_hello
./bin/test_koto_passes
./bin/test_koto_loopen --gtest_filter='*Loopen*'
```

---

## 5. Kotoamatsukami Pass Reference

All pass implementations are in `learn-pass/src/Kotoamatsukami/pass/`.

### Name mapping

| Config key | `-passes=` string | Source file | Effect |
|---|---|---|---|
| `flatten` | `flatten` | `Flatten.cpp` | Rewrites CFG into switch-dispatcher loop |
| `bogusControlFlow` | `bogus-control-flow` | `BogusControlFlow.cpp` | Inserts opaque predicate fake branches |
| `substitution` | `substitution` | `Substitution.cpp` | Replaces arithmetic with equivalent forms |
| `junkCode` | `add-junk-code` | `AddJunkCodePass.cpp` | Injects dead computation |
| `splitBasicBlocks` | `split-basic-block` | `SplitBasicBlock.cpp` | Splits BBs to complicate CFG |
| `antiDebug` | `anti-debug` | `AntiDebugPass.cpp` | Inserts ptrace anti-debug checks |
| `gvEncrypt` | `gv-encrypt` | `GVEncrypt.cpp` | Encrypts global strings/variables at runtime |
| `loopen` | `loopen` | `Loopen.cpp` | Loop obfuscation; injects `Kotoamatsukami_quick_pow` |
| `forObs` | `for-obs` | `ForObsPass.cpp` | Obscures for-loop structure |
| `indirectBranch` | `indirect-branch` | `IndirectBranch.cpp` | Replaces direct branches with indirect |
| `indirectCall` | `indirect-call` | `IndirectCall.cpp` | Replaces direct calls with indirect dispatch |
| `branch2call` | `branch2call` | `Branch2Call.cpp` | Converts branches to calls (64-bit) |
| `branch2call_32` | `branch2call-32` | `Branch2Call_32.cpp` | Same, 32-bit targets |

### Using the plugin

```bash
PLUGIN_EXT=so   # macOS: dylib

# Via opt — explicit pass list:
opt -load-pass-plugin ./build/lib/Kotoamatsukami.${PLUGIN_EXT} \
    -passes="flatten,bogus-control-flow" \
    -S /tmp/test.ll -o /tmp/test_obf.ll

# Via clang — all enabled passes run at pipeline start:
clang -O1 -fpass-plugin=./build/lib/Kotoamatsukami.${PLUGIN_EXT} \
    ./example/test.c -o /tmp/test_obf

# AArch64 cross-compile:
clang --target=aarch64-linux-gnu -O1 \
    -fpass-plugin=./build/lib/Kotoamatsukami.${PLUGIN_EXT} \
    ./example/test.c -o /tmp/test_obf_arm64
```

### Kotoamatsukami.config fields

| Field | Values | Meaning |
|---|---|---|
| `model` | `0` = disabled, `1` = enabled | Master switch per pass |
| `enable function` | `[""]` = all funcs, `["foo"]` = named list | Allowlist |
| `disable function` | `[]` = none, `["foo"]` = skip | Denylist |

See `Kotoamatsukami.config.schema.json` for full schema.

---

## 6. Architecture

```
C/C++ source
     │
     ▼
clang frontend                    learn-lang (Kaleidoscope-style)
     │                                 Lexer → Parser → AST
     ▼                                 → IRBuilder → ORC JIT
LLVM IR ◄────────────────────────────────────────────┘
     │
     ▼ New Pass Manager
     ├── learn-pass/src/hello/          HelloPass, JunkPass, SimpleObf, FlattenCF
     └── learn-pass/src/Kotoamatsukami/ full obfuscation suite (13 passes)
                  │
                  ▼
          Obfuscated IR / binary
                  │
                  ▼
          learn-deobf/                  deobfuscation passes (restore semantics)
```

---

## 7. Adding New Code — Exact Steps

### New demo (learn-demo)
1. Create `learn-demo/src/demo_myname.cpp` — CMake auto-discovers it.
2. Binary appears at `build/bin/demo_myname`.

### New Kotoamatsukami obfuscation pass
1. `learn-pass/include/learn_llvm/Kotoamatsukami/MyPass.h` — declare pass class
2. `learn-pass/src/Kotoamatsukami/pass/MyPass.cpp` — implement `run(Module&, ...)`
3. `learn-pass/src/Kotoamatsukami/PassPlugin.cpp`:
   - Add `#include "learn_llvm/Kotoamatsukami/MyPass.h"` at top
   - In `registerPipelineParsingCallback`: add `else if (Name == "my-pass") { MPM.addPass(MyPass()); return true; }`
   - In `registerPipelineStartEPCallback`: add `MPM.addPass(MyPass());`
4. `learn-pass/CMakeLists.txt`: add `src/Kotoamatsukami/pass/MyPass.cpp` to the Kotoamatsukami target
5. `Kotoamatsukami.config`: add `"myPass": { "model": 0, "enable function": [""], "disable function": [] }`
6. `Kotoamatsukami.config.schema.json`: add the new key to `properties`

### New GoogleTest
1. `tests/test_myfeature.cpp` — write tests
2. `tests/CMakeLists.txt`: add `add_executable` + `target_link_libraries` + `gtest_discover_tests`

---

## 8. Common Recipes

### List which passes are currently enabled

```bash
python3 -c "
import json
c = json.load(open('Kotoamatsukami.config'))
for k, v in c.items():
    if isinstance(v, dict) and v.get('model', 0) == 1:
        print('ENABLED:', k)
"
```

### Generate unobfuscated IR from example/test.c

```bash
clang -O1 -S -emit-llvm ./example/test.c -o /tmp/test_O1.ll
clang -O0 -S -emit-llvm ./example/test.c -o /tmp/test_O0.ll
```

### Apply single pass and inspect

```bash
PLUGIN_EXT=so   # macOS: dylib
opt -load-pass-plugin ./build/lib/Kotoamatsukami.${PLUGIN_EXT} \
    -passes=flatten -S /tmp/test_O1.ll -o /tmp/test_flat.ll
# View CFG:
opt -dot-cfg /tmp/test_flat.ll -o /dev/null && xdot .rc4_init.dot
```

---

## 9. Known Issues

| Issue | Cause | Fix |
|---|---|---|
| `One_backend` build failure | Needs LLVM source tree for `Target.td` | `LEARN_LLVM_ENABLE_ONE_BACKEND=OFF` (default) |
| `clang++` linker error `-lstdc++` | Missing `libstdc++-dev` on some Ubuntu | Use `g++` or install `libstdc++-11-dev` |
| macOS `brew install llvm` selects LLVM 22+ | Homebrew `llvm` tracks latest, but this repo is tested against 14–17 | Install `llvm@17`; `build.sh` prefers it automatically |
| `brew install llvm@17` fails on macOS | Command Line Tools are missing | Run `xcode-select --install` first |
| `StringRef::starts_with` compile error | API renamed in LLVM 16 | `#if LLVM_VERSION_MAJOR >= 16` guard (already applied in `DeHelloPass.cpp`) |

---

## 10. Code Conventions

| Convention | Rule |
|---|---|
| Language standard | C++17 |
| Indent | 4 spaces (see `.editorconfig`) |
| Pass class names | `PascalCase` (e.g., `BogusControlFlow`) |
| Pass pipeline strings | `kebab-case` (e.g., `bogus-control-flow`) |
| Config JSON keys | `camelCase` (e.g., `bogusControlFlow`) |
| New pass files | `{PascalName}.cpp` or `{PascalName}Pass.cpp` |

---

## 11. CI

`.github/workflows/ci.yml` — triggers on push/PR to `main`:
- OS: `ubuntu-22.04`, LLVM version: env var `LLVM_VERSION` (currently `17`)
- `LEARN_LLVM_ENABLE_ONE_BACKEND=OFF`
- Parallel build + test; uploads `Testing/` artifacts on failure
