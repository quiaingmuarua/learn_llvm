#include "learn_llvm/pass/HelloPass.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

namespace learn_llvm::pass {

// -------- HelloPass 实现 --------
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

// -------- JunkPass 实现 --------
PreservedAnalyses JunkPass::run(Function &F, FunctionAnalysisManager &) {
    if (F.isDeclaration())
        return PreservedAnalyses::all();

    // 找 entry 基本块
    BasicBlock &Entry = F.getEntryBlock();

    // 找一个作为基准的 i32 值：优先用函数第一个 i32 参数
    Value *Base = nullptr;
    for (auto &Arg : F.args()) {
        if (Arg.getType()->isIntegerTy(32)) {
            Base = &Arg;
            break;
        }
    }

    LLVMContext &Ctx = F.getContext();
    IRBuilder<> Builder(&*Entry.getFirstInsertionPt());

    if (!Base) {
        // 没有 i32 参数，就用常量 42
        Base = ConstantInt::get(Type::getInt32Ty(Ctx), 42);
    }

    auto *IntTy = cast<IntegerType>(Base->getType());
    auto *C3 = ConstantInt::get(IntTy, 3);
    auto *C7 = ConstantInt::get(IntTy, 7);

    // 插入两条“垃圾”算术指令（未使用）
    Value *J1 = Builder.CreateMul(Base, C3, "junk1");
    Builder.CreateAdd(J1, C7, "junk2");

    // 我们修改了函数 IR，不保留分析
    return PreservedAnalyses::none();
}

void runJunkPassOnModule(Module &M) {
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
    FPM.addPass(JunkPass{});

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    MPM.run(M, MAM);
}

} // namespace learn_llvm::pass
