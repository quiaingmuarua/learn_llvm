# AGENT_QUICKSTART.md — Fast Onboarding

Use this file when a new agent needs to become productive quickly.

## 1. Start

1. Read root `AGENTS.md`
2. Read one module guide only:
   - `learn-demo/AGENTS.md`
   - `learn-lang/AGENTS.md`
   - `learn-pass/AGENTS.md`
   - `learn-deobf/AGENTS.md`
3. Run the smallest valid verification for your change

## 2. Route

| Task | Module | Read first | Fast verify |
|---|---|---|---|
| LLVM API demo / numbered practice | `learn-demo` | local exercise dir + `learn-demo/AGENTS.md` | specific demo binary or `./build.sh test` |
| Lexer / parser / AST / REPL | `learn-lang` | `Lexer.cpp`, `Parser.cpp`, `learn-lang/AGENTS.md` | `ctest -R 'LangParser|Lexer' --output-on-failure` |
| New pass / plugin registration / config | `learn-pass` | `PassPlugin.cpp`, config files, `learn-pass/AGENTS.md` | `./build.sh test` |
| Deobfuscation / IR recovery | `learn-deobf` | `DeHelloPass.cpp`, fixture IR, `learn-deobf/AGENTS.md` | `ctest -R deobf --output-on-failure` |

## 3. Baselines

| Change type | Minimum check |
|---|---|
| Docs only | no build; if `build.sh` docs changed, run `bash -n build.sh` |
| Build/config/script changes | `./build.sh test` |
| Pass logic | `./build.sh test` |

## 4. Pitfalls

- Target LLVM 17 first. Do not treat LLVM 22+ compatibility as the default path.
- A Koto pass is not complete until code, registration, config, schema, and tests move together.
- Plugin outputs are `.so` on Linux/WSL and `.dylib` on macOS.
- `One_backend` stays off unless the full LLVM source tree is available.

## 5. Handoff

Include:

- target module
- touched files
- exact verification command
- LLVM-version assumption
- immediate next step

Template:

```md
# Handoff

## Task
- User asked:
- Done:
- Pending:

## Scope
- Module:
- Files touched:
- Files intentionally not touched:

## State
- Build/test status:
- Known failing command:
- LLVM assumption:

## Next
- Immediate next action:
- Verification:
```
