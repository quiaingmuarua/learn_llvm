#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "learn_llvm/Kotoamatsukami/AntiDebugPass.h"
#include "learn_llvm/Kotoamatsukami/BogusControlFlow.h"
#include "learn_llvm/Kotoamatsukami/Flatten.h"
#include "learn_llvm/Kotoamatsukami/GVEncrypt.h"
#include "learn_llvm/Kotoamatsukami/IndirectBranch.h"
#include "learn_llvm/Kotoamatsukami/IndirectCall.h"
#include "learn_llvm/Kotoamatsukami/SplitBasicBlock.h"
#include "learn_llvm/Kotoamatsukami/Substitution.h"
#include "learn_llvm/Kotoamatsukami/AddJunkCodePass.h"
#include "learn_llvm/Kotoamatsukami/Branch2Call.h"
#include "learn_llvm/Kotoamatsukami/Branch2Call_32.h"
#include "learn_llvm/Kotoamatsukami/ForObsPass.h"
#include "learn_llvm/Kotoamatsukami/Loopen.hpp"
using namespace llvm;

llvm::PassPluginLibraryInfo getKotoamatsukamiPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION, "Kotoamatsukami", LLVM_VERSION_STRING,
        [](PassBuilder& PB) {
            // first way to use the pass
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "gv-encrypt"){
                        MPM.addPass(GVEncrypt());
                        return true;
                    }
                    else if (Name == "split-basic-block") {
                        MPM.addPass(SplitBasicBlock());
                        return true;
                    } else if (Name == "anti-debug") {
                        MPM.addPass(AntiDebugPass());
                        return true;
                    }  else if (Name == "bogus-control-flow") {
                        MPM.addPass(BogusControlFlow());
                        return true;
                    } else if (Name == "add-junk-code") {
                        MPM.addPass(AddJunkCodePass());
                        return true;
                    } else if (Name == "loopen") {
                        MPM.addPass(Loopen());
                        return true;
                    } else if (Name == "for-obs") {
                        MPM.addPass(ForObsPass());
                        return true;
                    } else if (Name == "branch2call-32") {
                        MPM.addPass(Branch2Call_32());
                        return true;
                    } else if (Name == "branch2call") {
                        MPM.addPass(Branch2Call());
                        return true;
                    } else if (Name == "indirect-call") {
                        MPM.addPass(IndirectCall());
                        return true;
                    } else if (Name == "indirect-branch") {
                        MPM.addPass(IndirectBranch());
                        return true;
                    } else if (Name == "flatten") {
                        MPM.addPass(Flatten());
                        return true;
                    } else if (Name == "substitution") {
                        MPM.addPass(Substitution());
                        return true;
                    }
                    return false;
                });
            // second way to use the pass
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager& MPM, OptimizationLevel Level) {
                    MPM.addPass(AntiDebugPass());
                    MPM.addPass(SplitBasicBlock());
                    MPM.addPass(GVEncrypt());
                    MPM.addPass(BogusControlFlow());
                    MPM.addPass(AddJunkCodePass());
                    MPM.addPass(Loopen());
                    MPM.addPass(ForObsPass());
                    MPM.addPass(Branch2Call_32());
                    MPM.addPass(Branch2Call());
                    MPM.addPass(IndirectCall());
                    MPM.addPass(IndirectBranch());
                    MPM.addPass(Flatten());
                    MPM.addPass(Substitution());
                });
        }
    };
}

__attribute__((visibility("default"))) extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return getKotoamatsukamiPluginInfo();
}
