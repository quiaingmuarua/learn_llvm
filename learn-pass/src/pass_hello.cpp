#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>

#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>       // 关键：PassBuilder
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>

using namespace llvm;

// 一个简单的 Function Pass：给每个有定义的函数加上 "hello-pass" 属性
struct HelloPass : public PassInfoMixin<HelloPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        if (!F.isDeclaration()) {
            F.addFnAttr("hello-pass");
        }
        // 我们修改了 IR，不保留分析
        return PreservedAnalyses::none();
    }
};

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);

    cl::OptionCategory Cat("hello-pass options");
    cl::opt<std::string> InputFilename(
        cl::Positional, cl::desc("<input IR file>"), cl::Required, cl::cat(Cat));

    cl::ParseCommandLineOptions(argc, argv, "Simple Hello LLVM Pass\n");

    LLVMContext Context;
    SMDiagnostic Err;

    // 读取 IR
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    // ===== 标准 new PM 管线构建 =====
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

    // 构建 FunctionPassManager，添加我们的 HelloPass
    FunctionPassManager FPM;
    FPM.addPass(HelloPass());

    // 用 adaptor 把 FPM 嵌到 ModulePassManager 里
    ModulePassManager MPM;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    // 跑整个 Module
    MPM.run(*M, MAM);
    // ================================

    // 校验 IR
    if (verifyModule(*M, &errs())) {
        errs() << "ERROR: module verification failed after HelloPass\n";
        return 1;
    }

    // 输出修改后的 IR
    M->print(outs(), nullptr);

    return 0;
}
