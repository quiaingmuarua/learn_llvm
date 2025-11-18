#include <gtest/gtest.h>

#include "TestHelpers.h"

#include "learn_llvm/Kotoamatsukami/Loopen.hpp"

using namespace llvm;

namespace {

TEST(KotoamatsukamiLoopenTest, InjectsQuickPowHelper) {
    const char *IRText = R"IR(
        define i32 @foo(i32 %x, i32 %y) {
        entry:
          %cmp = icmp ult i32 %x, %y
          br i1 %cmp, label %then, label %else

        then:
          %add = add i32 %x, %y
          ret i32 %add

        else:
          %sub = sub i32 %x, %y
          ret i32 %sub
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = koto_test::parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    auto cfg = koto_test::makeBaseConfig();
    cfg["loopen"]["model"] = 1;
    koto_test::loadConfig(cfg, "loopen");

    Loopen Pass;
    koto_test::runModulePass(*Mod, Pass);

    Function *QuickPow = Mod->getFunction("Kotoamatsukami_quick_pow");
    ASSERT_NE(QuickPow, nullptr);
    EXPECT_FALSE(QuickPow->isDeclaration());

    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    unsigned QuickPowCalls = 0;
    for (Instruction &I : instructions(Foo)) {
        if (auto *CB = dyn_cast<CallBase>(&I)) {
            if (Function *Callee = CB->getCalledFunction()) {
                if (Callee->getName() == "Kotoamatsukami_quick_pow") {
                    ++QuickPowCalls;
                }
            }
        }
    }
    EXPECT_GT(QuickPowCalls, 0u);
}

} // namespace

