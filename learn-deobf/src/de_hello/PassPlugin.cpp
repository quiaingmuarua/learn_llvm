#include "learn_llvm/de_hello/DeHelloPass.h"

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

using namespace llvm;
using namespace learn_llvm::deobf;

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "learn-llvm-deobf",
        "0.1",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "de-hello") {
                        FPM.addPass(DeHelloPass{});
                        return true;
                    }
                    return false;
                });

            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                    FunctionPassManager FPM;
                    FPM.addPass(DeHelloPass{});
                    MPM.addPass(
                        createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }};
}

