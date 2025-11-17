#include "learn_llvm/pass/HelloPass.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/Passes/PassBuilder.h>

#include <algorithm>

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

namespace {

ConstantInt *createNoiseMask(IntegerType *IntTy) {
    const unsigned BitWidth = IntTy->getBitWidth();
    APInt Mask(BitWidth, 0, false);
    const uint64_t Pattern = 0xA5A5A5A5A5A5A5A5ULL;
    for (unsigned Offset = 0; Offset < BitWidth; Offset += 64) {
        const unsigned ChunkWidth = std::min<unsigned>(64, BitWidth - Offset);
        uint64_t ChunkValue = Pattern;
        if (ChunkWidth < 64) {
            ChunkValue &= ((1ULL << ChunkWidth) - 1ULL);
        }
        APInt Chunk(BitWidth, ChunkValue);
        Mask |= (Chunk << Offset);
    }
    return ConstantInt::get(IntTy->getContext(), Mask);
}

Function *getOrCreateObfAddHelper(Module &M, IntegerType *IntTy) {
    std::string Name = "__learn_llvm_obf_add_i" + std::to_string(IntTy->getBitWidth());
    if (Function *Existing = M.getFunction(Name)) {
        return Existing;
    }

    LLVMContext &Ctx = M.getContext();
    auto *FnTy = FunctionType::get(IntTy, {IntTy, IntTy}, false);
    auto *Fn = Function::Create(FnTy, Function::InternalLinkage, Name, M);
    Fn->setDoesNotThrow();
    Fn->addFnAttr(Attribute::NoInline);
    Fn->addFnAttr(Attribute::OptimizeNone);

    BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
    BasicBlock *Loop = BasicBlock::Create(Ctx, "loop", Fn);
    BasicBlock *Bogus = BasicBlock::Create(Ctx, "bogus", Fn);
    BasicBlock *Exit = BasicBlock::Create(Ctx, "exit", Fn);

    IRBuilder<> Builder(Entry);
    Builder.CreateBr(Loop);

    Builder.SetInsertPoint(Loop);
    auto *PhiSum = Builder.CreatePHI(IntTy, 2, "sum_state");
    auto *PhiCarry = Builder.CreatePHI(IntTy, 2, "carry_state");
    PhiSum->addIncoming(Fn->getArg(0), Entry);
    PhiCarry->addIncoming(Fn->getArg(1), Entry);

    Value *NextSum = Builder.CreateXor(PhiSum, PhiCarry, "obf_sum");
    Value *CarryAnd = Builder.CreateAnd(PhiSum, PhiCarry, "obf_carry");
    Value *NextCarry = Builder.CreateShl(CarryAnd, ConstantInt::get(IntTy, 1), "obf_carry_shl");

    PhiSum->addIncoming(NextSum, Loop);
    PhiCarry->addIncoming(NextCarry, Loop);

    Value *Cond = Builder.CreateICmpEQ(NextCarry, ConstantInt::get(IntTy, 0), "carry_is_zero");
    Builder.CreateCondBr(Cond, Bogus, Loop);

    Builder.SetInsertPoint(Bogus);
    ConstantInt *Mask = createNoiseMask(IntTy);
    Value *Noise = Builder.CreateXor(NextSum, Mask, "noise_mix");
    Value *NoiseFix = Builder.CreateXor(Noise, Mask, "noise_fix");
    Builder.CreateBr(Exit);

    Builder.SetInsertPoint(Exit);
    auto *ResultPhi = Builder.CreatePHI(IntTy, 2, "result_phi");
    ResultPhi->addIncoming(NextSum, Loop);
    ResultPhi->addIncoming(NoiseFix, Bogus);
    Builder.CreateRet(ResultPhi);

    return Fn;
}

} // namespace

// -------- SimpleObfPass 实现 --------
//
// 将整数 add 指令替换成一次调用复杂 helper 的形式。helper 会使用
// 基于位运算和循环的加法实现，还带有假分支，编译后在汇编中能看到更复杂的控制流。
PreservedAnalyses SimpleObfPass::run(Function &F, FunctionAnalysisManager &) {
    if (F.isDeclaration())
        return PreservedAnalyses::all();

    bool Changed = false;
    Module *M = F.getParent();

    for (auto &BB : F) {
        for (auto It = BB.begin(), End = BB.end(); It != End;) {
            Instruction *Inst = &*It++;
            auto *BO = dyn_cast<BinaryOperator>(Inst);
            if (!BO || BO->getOpcode() != Instruction::Add)
                continue;
            auto *IntTy = dyn_cast<IntegerType>(BO->getType());
            if (!IntTy)
                continue;

            IRBuilder<> Builder(BO);
            Function *Helper = getOrCreateObfAddHelper(*M, IntTy);
            Value *ObfCall = Builder.CreateCall(Helper, {BO->getOperand(0), BO->getOperand(1)}, "obf_sum_call");
            BO->replaceAllUsesWith(ObfCall);
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
