#include "learn_llvm/de_hello/DeHelloPass.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

namespace {

bool isObfAddHelperName(StringRef Name) {
    return Name.starts_with("__learn_llvm_obf_add_i");
}

bool matchesObfAddSignature(const Function &Fn) {
    auto *RetTy = dyn_cast<IntegerType>(Fn.getReturnType());
    if (!RetTy)
        return false;
    if (Fn.arg_size() != 2)
        return false;
    for (const Argument &Arg : Fn.args()) {
        if (Arg.getType() != RetTy)
            return false;
    }
    return true;
}

bool isObfAddHelper(const Function &Fn) {
    return Fn.hasName() && isObfAddHelperName(Fn.getName()) &&
           matchesObfAddSignature(Fn);
}

} // namespace

namespace learn_llvm::deobf {

PreservedAnalyses DeHelloPass::run(Function &F, FunctionAnalysisManager &) {
    if (F.isDeclaration())
        return PreservedAnalyses::all();

    bool Changed = false;
    SmallPtrSet<Function *, 4> HelpersToCleanup;

    for (auto &BB : F) {
        for (auto It = BB.begin(), End = BB.end(); It != End;) {
            Instruction *Inst = &*It++;
            auto *Call = dyn_cast<CallBase>(Inst);
            if (!Call)
                continue;

            Function *Callee = Call->getCalledFunction();
            if (!Callee || !isObfAddHelper(*Callee))
                continue;

            if (Call->arg_size() != 2)
                continue;

            Value *LHS = Call->getArgOperand(0);
            Value *RHS = Call->getArgOperand(1);
            Type *ResultTy = Callee->getReturnType();
            if (!ResultTy->isIntegerTy())
                continue;
            if (LHS->getType() != ResultTy || RHS->getType() != ResultTy)
                continue;

            IRBuilder<> Builder(Call);
            std::string Name =
                Call->getName().empty() ? "deobf_sum" : (Call->getName() + ".deobf").str();
            Value *PlainAdd = Builder.CreateAdd(LHS, RHS, Name);
            Call->replaceAllUsesWith(PlainAdd);
            HelpersToCleanup.insert(Callee);
            Call->eraseFromParent();
            Changed = true;
        }
    }

    if (Changed) {
        SmallVector<Function *, 4> Worklist(HelpersToCleanup.begin(),
                                            HelpersToCleanup.end());
        for (Function *Helper : Worklist) {
            if (Helper && Helper->hasInternalLinkage() && Helper->use_empty()) {
                Helper->eraseFromParent();
            }
        }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

void runDeHelloPassOnModule(Module &M) {
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
    FPM.addPass(DeHelloPass{});

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    MPM.run(M, MAM);
}

} // namespace learn_llvm::deobf

