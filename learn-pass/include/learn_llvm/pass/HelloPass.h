#pragma once

#include <llvm/IR/PassManager.h>

namespace llvm {
    class Module;
} // namespace llvm

namespace learn_llvm::pass {

    // ---------------- HelloPass ----------------
    struct HelloPass : public llvm::PassInfoMixin<HelloPass> {
        llvm::PreservedAnalyses run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &AM);
    };

    void runHelloPassOnModule(llvm::Module &M);

    // ---------------- JunkPass -----------------
    struct JunkPass : public llvm::PassInfoMixin<JunkPass> {
        llvm::PreservedAnalyses run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &AM);
    };

    void runJunkPassOnModule(llvm::Module &M);

    // ------------- SimpleObfPass ----------------
    // 一个非常简单的“指令级”混淆 pass：
    //   对整数 add 指令：  a + b  ==>  (a - C) + (b + C)
    // 这样在 IR 里能明显看到更多算术指令，但语义保持不变（模 2^n 算术下恒等）。
    struct SimpleObfPass : public llvm::PassInfoMixin<SimpleObfPass> {
        llvm::PreservedAnalyses run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &AM);
    };

    void runSimpleObfOnModule(llvm::Module &M);

} // namespace learn_llvm::pass
