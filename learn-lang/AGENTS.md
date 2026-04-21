# learn-lang — Agent Guide

> See root `CLAUDE.md` for full project context and build instructions.

## Purpose

Implements a Kaleidoscope-style toy language compiler as a practical way to learn
LLVM's frontend APIs: lexing, parsing, AST construction, IR generation via
`IRBuilder`, and JIT execution via ORC JIT.

---

## Directory Layout

```
learn-lang/
├── CMakeLists.txt
├── include/               ← public headers (used by lang_core_lib and tests)
│   └── (AST, Lexer, Parser, CodeGen headers)
└── src/repl/
    ├── AST.cpp            ← AST node implementations
    ├── Lexer.cpp          ← tokenizer
    ├── Parser.cpp         ← recursive-descent parser
    └── lang_repl_main.cpp ← entry point for interactive REPL
```

---

## Build Outputs

| Target | Output | Use |
|---|---|---|
| `lang_core_lib` | static lib | linked by `lang_repl` and `test_lang_parser` |
| `lang_repl` | `build/bin/lang_repl` | interactive REPL |

---

## Architecture

```
Input string
     │
     ▼  Lexer (Lexer.cpp)
Token stream
     │
     ▼  Parser (Parser.cpp)  — recursive descent
AST nodes (AST.cpp)
     │
     ▼  (planned) CodeGen → IRBuilder → LLVM IR → ORC JIT
```

---

## Adding a New Language Feature

1. Add AST node class(es) in `include/` header and `src/repl/AST.cpp`
2. Add token type(s) in `Lexer.h` / `Lexer.cpp`
3. Add parse rule in `Parser.cpp`
4. Add IR codegen in CodeGen (if it exists), following the IRBuilder pattern
5. Add a test case in `tests/test_lang_parser.cpp`

---

## LLVM API Notes

- `lang_core_lib` intentionally does **not** link `llvm_common` — the parser/lexer/AST
  are pure C++ with no LLVM dependency. This keeps RTTI/exceptions available for
  `dynamic_cast` in tests. Only the codegen layer should link LLVM.
- When adding IR generation, link `llvm_common` to the codegen target only.

---

## Tests

```
tests/test_lang_parser.cpp   ← unit tests for Lexer + Parser
```

Run: `./build/bin/test_lang_parser`
