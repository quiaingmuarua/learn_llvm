#include <gtest/gtest.h>

#include "learn_llvm/hello/HelloPass.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace learn_llvm;

// 这里不再定义 HelloPass，直接用库里的

TEST(HelloPassTest, AddsAttributeToDefinedFunctions) {
    const char *IRText = R"(
        define i32 @add(i32 %a, i32 %b) {
        entry:
          %sum = add i32 %a, %b
          ret i32 %sum
        }

        declare void @external_fn(i32)
    )";

    LLVMContext Context;
    SMDiagnostic Err;

    auto Buffer = MemoryBuffer::getMemBuffer(IRText, "test_module", false);
    std::unique_ptr<Module> M = parseIR(*Buffer, Err, Context);
    ASSERT_NE(M, nullptr) << "Failed to parse test IR";

    // 直接调用封装好的管线
    pass::runHelloPassOnModule(*M);

    Function *AddFn = M->getFunction("add");
    ASSERT_NE(AddFn, nullptr);
    EXPECT_TRUE(AddFn->hasFnAttribute("hello-pass"));

    Function *ExtFn = M->getFunction("external_fn");
    ASSERT_NE(ExtFn, nullptr);
    EXPECT_FALSE(ExtFn->hasFnAttribute("hello-pass"));

    EXPECT_FALSE(verifyModule(*M, &errs()));
}
