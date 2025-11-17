#pragma once

#include <llvm/IR/PassManager.h>

namespace llvm {
    class Module;
} // namespace llvm

namespace learn_llvm::pass {

    // 真正的 FunctionPass 本体
    struct HelloPass : public llvm::PassInfoMixin<HelloPass> {
        llvm::PreservedAnalyses run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &AM);
    };

    // 一个辅助函数：在整个 Module 上跑 HelloPass
    // 供 CLI 工具和测试复用
    void runHelloPassOnModule(llvm::Module &M);

} // namespace learn_llvm::pass
