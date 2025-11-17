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

} // namespace learn_llvm::pass
