#pragma once

#include "learn_llvm/Kotoamatsukami/config.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

namespace koto_test {

inline nlohmann::json makeBaseConfig() {
    using json = nlohmann::json;
    auto fnList = json::array({""});

    auto makeSection = [&](int model = 0) {
        json section;
        section["model"] = model;
        section["enable function"] = fnList;
        section["disable function"] = fnList;
        return section;
    };

    json cfg;
    cfg["target"] = "x86_64";

    cfg["loopen"] = makeSection();
    cfg["loopen"]["loopen_x_list"] = json::array();
    cfg["loopen"]["module_name"] = "";

    cfg["forObs"] = makeSection();
    cfg["forObs"]["innerLoopBoundary"] = 4;
    cfg["forObs"]["outerLoopBoundary"] = 4;

    cfg["splitBasicBlocks"] = makeSection();
    cfg["splitBasicBlocks"]["split number"] = 2;

    cfg["branch2call"] = makeSection();
    cfg["branch2call"]["split number"] = 2;

    cfg["branch2call_32"] = makeSection();
    cfg["branch2call_32"]["split number"] = 2;

    cfg["junkCode"] = makeSection();
    cfg["junkCode"]["probability"] = 20;

    cfg["antiDebug"] = makeSection();
    cfg["indirectBranch"] = makeSection();
    cfg["indirectCall"] = makeSection();
    cfg["bogusControlFlow"] = makeSection();
    cfg["substitution"] = makeSection();
    cfg["flatten"] = makeSection();
    cfg["gvEncrypt"] = makeSection();

    return cfg;
}

inline std::string writeConfigFile(const nlohmann::json &cfg,
                                   const std::string &suffix) {
    auto tmpPath = std::filesystem::temp_directory_path() /
                   ("koto_test_config_" + suffix + ".json");
    std::ofstream ofs(tmpPath);
    ofs << cfg.dump(2);
    ofs.close();
    return tmpPath.string();
}

inline void loadConfig(const nlohmann::json &cfg,
                       const std::string &suffix) {
    std::string path = writeConfigFile(cfg, suffix);
    isConfigured = 0;
    readConfig(path);
}

inline std::unique_ptr<llvm::Module>
parseModuleFromIR(llvm::StringRef IR, llvm::LLVMContext &Ctx) {
    llvm::SMDiagnostic Err;
    auto Buffer = llvm::MemoryBuffer::getMemBuffer(IR, "koto_test_module", false);
    return llvm::parseIR(*Buffer, Err, Ctx);
}

template <typename PassT>
inline void runModulePass(llvm::Module &M, PassT &Pass) {
    llvm::PassBuilder PB;
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    Pass.run(M, MAM);
}

} // namespace koto_test

