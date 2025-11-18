#include <gtest/gtest.h>

#include "learn_llvm/Kotoamatsukami/Loopen.hpp"
#include "learn_llvm/Kotoamatsukami/config.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

#include <filesystem>
#include <fstream>

using namespace llvm;

namespace {

std::string writeTempConfig() {
    static const char *kConfigTemplate = R"JSON(
{
    "target": "x86_64",
    "loopen": {
        "model": 1,
        "enable function": [""],
        "disable function": [],
        "loopen_x_list": [2,3,5,7],
        "module_name": ""
    },
    "forObs": { "model": 0, "enable function": [""], "disable function": [] },
    "splitBasicBlocks": { "model": 0, "enable function": [""], "disable function": [], "split number": 2 },
    "branch2call": { "model": 0, "enable function": [""], "disable function": [], "split number": 2 },
    "branch2call_32": { "model": 0, "enable function": [""], "disable function": [], "split number": 2 },
    "junkCode": { "model": 0, "enable function": [""], "disable function": [] },
    "antiDebug": { "model": 0, "enable function": [""], "disable function": [] },
    "indirectBranch": { "model": 0, "enable function": [""], "disable function": [] },
    "indirectCall": { "model": 0, "enable function": [""], "disable function": [] },
    "bogusControlFlow": { "model": 0, "enable function": [""], "disable function": [] },
    "substitution": { "model": 0, "enable function": [""], "disable function": [] },
    "flatten": { "model": 0, "enable function": [""], "disable function": [] },
    "gvEncrypt": { "model": 0, "enable function": [""], "disable function": [] }
}
)JSON";

    auto tmpPath = std::filesystem::temp_directory_path() / "koto_loopen_test_config.json";
    std::ofstream ofs(tmpPath);
    ofs << kConfigTemplate;
    ofs.close();
    return tmpPath.string();
}

std::unique_ptr<Module> parseModuleFromIR(StringRef IR, LLVMContext &Ctx) {
    SMDiagnostic Err;
    auto Buffer = MemoryBuffer::getMemBuffer(IR, "koto_loopen_test", false);
    return parseIR(*Buffer, Err, Ctx);
}

TEST(KotoamatsukamiLoopenTest, InjectsQuickPowHelper) {
    const char *IRText = R"IR(
        define i32 @foo(i32 %x, i32 %y) {
        entry:
          %cmp = icmp ult i32 %x, %y
          br i1 %cmp, label %then, label %else

        then:
          %add = add i32 %x, %y
          ret i32 %add

        else:
          %sub = sub i32 %x, %y
          ret i32 %sub
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    std::string cfgPath = writeTempConfig();
    isConfigured = 0;
    readConfig(cfgPath);

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

    Loopen Pass;
    Pass.run(*Mod, MAM);

    Function *QuickPow = Mod->getFunction("Kotoamatsukami_quick_pow");
    ASSERT_NE(QuickPow, nullptr);
    EXPECT_FALSE(QuickPow->isDeclaration());

    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    unsigned QuickPowCalls = 0;
    for (Instruction &I : instructions(Foo)) {
        if (auto *CB = dyn_cast<CallBase>(&I)) {
            if (Function *Callee = CB->getCalledFunction()) {
                if (Callee->getName() == "Kotoamatsukami_quick_pow") {
                    ++QuickPowCalls;
                }
            }
        }
    }
    EXPECT_GT(QuickPowCalls, 0u);

    EXPECT_FALSE(verifyModule(*Mod, &errs()));
}

} // namespace

