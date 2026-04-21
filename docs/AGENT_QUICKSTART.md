# AGENT_QUICKSTART.md — Fast Onboarding

Use this file when a new agent needs to become productive in this repository
without reading every module in full.

## 1. First 3 Steps

1. Read root `AGENTS.md`
2. Read exactly one module guide:
   - `learn-demo/AGENTS.md`
   - `learn-lang/AGENTS.md`
   - `learn-pass/AGENTS.md`
   - `learn-deobf/AGENTS.md`
3. Verify the workspace with `./build.sh test`

## 2. Task Routing

| Task shape | Main module | High-risk files |
|---|---|---|
| LLVM API demo or numbered practice exercise | `learn-demo` | per-exercise `CMakeLists.txt`, unit tests under `llvm_practice` |
| Toy language syntax / parser / AST / REPL | `learn-lang` | `Lexer.*`, `Parser.*`, AST headers, parser tests |
| New pass, plugin registration, pass config | `learn-pass` | `PassPlugin.cpp`, `Kotoamatsukami.config`, schema, root tests |
| Recovering obfuscated IR | `learn-deobf` | `DeHelloPass.cpp`, plugin entry, deobf fixture IR |

## 3. Minimum Verification

| Change type | Minimum check |
|---|---|
| Docs only | `bash -n build.sh` if script docs changed; otherwise no build needed |
| Build/config/script changes | `./build.sh test` |
| `learn-lang` parser or lexer | `./build.sh test` or `ctest -R 'LangParser|Lexer' --output-on-failure` |
| `learn-pass` / `learn-deobf` pass logic | `./build.sh test` |

## 4. Common Pitfalls

- The repository targets LLVM 17 first. Do not silently upgrade behavior for LLVM 22+ and call it a fix.
- A new Kotoamatsukami pass is incomplete unless all of these move together:
  header, implementation, `PassPlugin.cpp`, `CMakeLists.txt`, config JSON, config schema, tests.
- Plugin outputs are platform-specific shared libraries: `.so` on Linux/WSL, `.dylib` on macOS.
- `One_backend` is intentionally off by default. It requires the full LLVM source tree.

## 5. Handoff Format

When handing off to another agent, include:

- the target module
- the files already touched
- the exact verification command you ran
- any LLVM-version assumption you relied on
