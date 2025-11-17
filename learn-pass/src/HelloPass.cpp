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
        // 在每个有定义的函数上打一个标记，方便在 IR 里肉眼确认 pass 跑过
        F.addFnAttr("hello-pass");
        // 额外打印一行 log，配合 clang -fpass-plugin 时可以在终端看到
        errs() << "[HelloPass] run on function: " << F.getName() << "\n";
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

    errs() << "[JunkPass] run on function: " << F.getName() << "\n";

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
    // 显式给“垃圾指令”命名，便于在 IR 里搜索 junk1/junk2
    Value *J1 = Builder.CreateMul(Base, C3, "junk1");
    Value *J2 = Builder.CreateAdd(J1, C7, "junk2");

    // 再次设置一次名字，防止某些 pipeline 中间步骤覆盖掉 IRBuilder 传入的名称
    J1->setName("junk1");
    J2->setName("junk2");

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

// -------- SimpleObfPass 实现 --------
//
// 规则：
//   对整数加法：   %sum = add Ty %a, %b
//   重写为：
//     %c1   = sub Ty %a, C
//     %c2   = add Ty %b, C
//     %obf  = add Ty %c1, %c2
//   并用 %obf 替换原来的 %sum。
// 在模 2^n 算术下，(a - C) + (b + C) == a + b，因此语义不变，但 IR 更复杂。
PreservedAnalyses SimpleObfPass::run(Function &F, FunctionAnalysisManager &) {
    if (F.isDeclaration())
        return PreservedAnalyses::all();

    bool Changed = false;
    LLVMContext &Ctx = F.getContext();

    // 遍历函数中的所有基本块和指令
    for (auto &BB : F) {
        // 为了安全起见，先收集要处理的 add，再统一改写，避免边遍历边修改导致迭代器失效
        SmallVector<Instruction *, 8> AddInsts;
        for (auto &I : BB) {
            if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
                if (BO->getOpcode() == Instruction::Add &&
                    BO->getType()->isIntegerTy()) {
                    AddInsts.push_back(BO);
                }
            }
        }

        for (Instruction *I : AddInsts) {
            auto *BO = cast<BinaryOperator>(I);
            IRBuilder<> Builder(BO);

            auto *IntTy = cast<IntegerType>(BO->getType());
            // 选一个看得见的常量，方便在 IR 中识别混淆逻辑
            auto *C = ConstantInt::get(IntTy, 13);

            Value *A = BO->getOperand(0);
            Value *B = BO->getOperand(1);

            // (a - C) + (b + C)
            Value *SubA = Builder.CreateSub(A, C, "obf_sub_a");
            Value *AddB = Builder.CreateAdd(B, C, "obf_add_b");
            Value *NewAdd = Builder.CreateAdd(SubA, AddB, "obf_add");

            BO->replaceAllUsesWith(NewAdd);
            BO->eraseFromParent();

            Changed = true;
        }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

void runSimpleObfOnModule(Module &M) {
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
    FPM.addPass(SimpleObfPass{});

    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    MPM.run(M, MAM);
}

} // namespace learn_llvm::pass
