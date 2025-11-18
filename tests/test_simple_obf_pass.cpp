#include <gtest/gtest.h>

#include "../learn-pass/include/learn_llvm/hello/HelloPass.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

using namespace llvm;
using namespace learn_llvm;

TEST(SimpleObfPassTest, ReplacesAddWithHelperCall) {
    const char *IRText = R"(
        define i32 @foo(i32 %x, i32 %y) {
        entry:
          %sum = add i32 %x, %y
          ret i32 %sum
        }
    )";

    LLVMContext Ctx;
    SMDiagnostic Err;

    auto Buffer = MemoryBuffer::getMemBuffer(IRText, "simple_obf_test", false);
    std::unique_ptr<Module> M = parseIR(*Buffer, Err, Ctx);
    ASSERT_NE(M, nullptr) << "Failed to parse test IR";

    pass::runSimpleObfOnModule(*M);

    Function *Foo = M->getFunction("foo");
    ASSERT_NE(Foo, nullptr);

    unsigned AddCount = 0;
    unsigned HelperCalls = 0;
    for (Instruction &I : instructions(Foo)) {
        if (I.getOpcode() == Instruction::Add)
            ++AddCount;
        if (auto *CB = dyn_cast<CallBase>(&I)) {
            if (Function *Callee = CB->getCalledFunction()) {
                if (Callee->getName().startswith("__learn_llvm_obf_add_i32"))
                    ++HelperCalls;
            }
        }
    }

    EXPECT_EQ(AddCount, 0u);
    EXPECT_EQ(HelperCalls, 1u);

    Function *Helper = M->getFunction("__learn_llvm_obf_add_i32");
    ASSERT_NE(Helper, nullptr);
    EXPECT_FALSE(Helper->isDeclaration());

    EXPECT_FALSE(verifyModule(*M, &errs()));
}

