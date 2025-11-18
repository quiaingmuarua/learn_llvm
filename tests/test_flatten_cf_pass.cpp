#include <gtest/gtest.h>

#include "../learn-pass/include/learn_llvm/hello/HelloPass.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;
using namespace learn_llvm;

TEST(FlattenCFPassTest, FlattensSimpleBranchFunction) {
    const char *IRText = R"(
        define i32 @foo(i1 %cond) {
        entry:
          br i1 %cond, label %then, label %else

        then:
          ret i32 1

        else:
          ret i32 2
        }
    )";

    LLVMContext Ctx;
    SMDiagnostic Err;
    auto Buffer = MemoryBuffer::getMemBuffer(IRText, "flatten_test", false);
    std::unique_ptr<Module> M = parseIR(*Buffer, Err, Ctx);
    ASSERT_NE(M, nullptr) << "Failed to parse IR";

    pass::runFlattenCFOnModule(*M);

    Function *Foo = M->getFunction("foo");
    ASSERT_NE(Foo, nullptr);

    bool HasDispatch = false;
    unsigned SwitchCount = 0;
    for (auto &BB : *Foo) {
        if (BB.getName() == "fla.dispatch")
            HasDispatch = true;
        if (isa<SwitchInst>(BB.getTerminator()))
            ++SwitchCount;
    }

    EXPECT_TRUE(HasDispatch);
    EXPECT_EQ(SwitchCount, 1u);
    EXPECT_FALSE(verifyModule(*M, &errs()));
}

