#include <gtest/gtest.h>

#include "../learn-pass/include/learn_llvm/hello/HelloPass.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace learn_llvm;

TEST(JunkPassTest, InsertsJunkInstructionsInEntry) {
    const char *IRText = R"(
        define i32 @foo(i32 %x) {
        entry:
          ret i32 %x
        }
    )";

    LLVMContext Ctx;
    SMDiagnostic Err;

    auto Buffer = MemoryBuffer::getMemBuffer(IRText, "test_module", false);
    std::unique_ptr<Module> M = parseIR(*Buffer, Err, Ctx);
    ASSERT_NE(M, nullptr) << "Failed to parse test IR";

    // 跑 JunkPass
    pass::runJunkPassOnModule(*M);

    Function *Foo = M->getFunction("foo");
    ASSERT_NE(Foo, nullptr);

    BasicBlock &Entry = Foo->getEntryBlock();

    bool HasJunk1 = false;
    bool HasJunk2 = false;

    for (Instruction &I : Entry) {
        if (I.getName() == "junk1")
            HasJunk1 = true;
        if (I.getName() == "junk2")
            HasJunk2 = true;
    }

    EXPECT_TRUE(HasJunk1);
    EXPECT_TRUE(HasJunk2);

    // 确保 IR 仍然合法
    EXPECT_FALSE(verifyModule(*M, &errs()));
}
