#include "learn_llvm/de_hello/DeHelloPass.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace learn_llvm;

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);

    cl::OptionCategory Cat("deobf-pass options");
    cl::opt<std::string> InputFilename(
        cl::Positional, cl::desc("<input IR file>"), cl::Required, cl::cat(Cat));

    cl::ParseCommandLineOptions(argc, argv, "Simple LLVM deobfuscation pass\n");

    LLVMContext Context;
    SMDiagnostic Err;

    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    deobf::runDeHelloPassOnModule(*M);

    if (verifyModule(*M, &errs())) {
        errs() << "ERROR: module verification failed after DeHelloPass\n";
        return 1;
    }

    M->print(outs(), nullptr);
    return 0;
}

