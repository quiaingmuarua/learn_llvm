#include "learn_llvm/pass/HelloPass.h"

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

using namespace llvm;
using namespace learn_llvm::pass;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "learn-llvm-pass",   // 插件名字
        "0.1",               // 版本
        [](PassBuilder &PB) {
            // ① 给 opt 用的：文本 pipeline（-passes=hello-pass,junk-pass,simple-obf,flatten-cf）
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "hello-pass") {
                        FPM.addPass(HelloPass{});
                        return true;
                    }
                    if (Name == "junk-pass") {
                        FPM.addPass(JunkPass{});
                        return true;
                    }
                    if (Name == "simple-obf") {
                        FPM.addPass(SimpleObfPass{});
                        return true;
                    }
                    if (Name == "flatten-cf") {
                        FPM.addPass(FlattenCFPass{});
                        return true;
                    }
                    return false;
                });

            // ② 给 clang 用的：挂在优化管线的“末尾”
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    // 如果你想在 O0 也混淆，可以不判断 Level；
                    // 一般混淆都是在 O1+，这里可以过滤一下：
                    if (Level == OptimizationLevel::O0)
                        return;

                    FunctionPassManager FPM;
                    FPM.addPass(HelloPass{});
                    FPM.addPass(JunkPass{});
                    FPM.addPass(SimpleObfPass{});
                    FPM.addPass(FlattenCFPass{});

                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }
    };
}
