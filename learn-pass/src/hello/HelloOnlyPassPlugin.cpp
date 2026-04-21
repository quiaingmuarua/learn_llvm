#include "learn_llvm/hello/HelloPass.h"

#include <llvm/Passes/PassBuilder.h>
#if __has_include(<llvm/Plugins/PassPlugin.h>)
#include <llvm/Plugins/PassPlugin.h>
#else
#include <llvm/Passes/PassPlugin.h>
#endif

using namespace llvm;
using namespace learn_llvm::pass;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "learn-llvm-hello-only",
        "0.1",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "hello-pass") {
                        FPM.addPass(HelloPass{});
                        return true;
                    }
                    return false;
                });

            // Run even under debug-style builds so Android smoke tests do not
            // depend on the frontend choosing O1+.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                    FunctionPassManager FPM;
                    FPM.addPass(HelloPass{});
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }
    };
}
