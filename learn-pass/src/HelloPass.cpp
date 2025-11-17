#include "learn_llvm/pass/HelloPass.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

namespace learn_llvm::pass {

    PreservedAnalyses HelloPass::run(Function &F, FunctionAnalysisManager &) {
        if (!F.isDeclaration()) {
            F.addFnAttr("hello-pass");
        }
        return PreservedAnalyses::none();
    }

    void runHelloPassOnModule(Module &M) {
        PassBuilder PB;

        LoopAnalysisManager LAM;
        FunctionAnalysisManager FAM;
        CGSCCAnalysisManager CGAM;
        ModuleAnalysisManager MAM;

        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        FunctionPassManager FPM;
        FPM.addPass(HelloPass{});

        ModulePassManager MPM;
        MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

        MPM.run(M, MAM);
    }

} // namespace learn_llvm::pass
