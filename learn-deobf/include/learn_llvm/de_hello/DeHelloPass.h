#pragma once

#include <llvm/IR/PassManager.h>

namespace llvm {
    class Module;
}

namespace learn_llvm::deobf {

    struct DeHelloPass : public llvm::PassInfoMixin<DeHelloPass> {
        llvm::PreservedAnalyses run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &AM);
    };

    void runDeHelloPassOnModule(llvm::Module &M);

} // namespace learn_llvm::deobf

