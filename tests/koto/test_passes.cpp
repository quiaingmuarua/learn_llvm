#include <gtest/gtest.h>

#include "TestHelpers.h"

#include "learn_llvm/Kotoamatsukami/AddJunkCodePass.h"
#include "learn_llvm/Kotoamatsukami/ForObsPass.h"
#include "learn_llvm/Kotoamatsukami/SplitBasicBlock.h"
#include "learn_llvm/Kotoamatsukami/BogusControlFlow.h"

using namespace llvm;

namespace {

TEST(KotoamatsukamiForObsTest, InsertsNestedLoops) {
    const char *IRText = R"IR(
        define void @foo(i32 %x, i32 %y) {
        entry:
          %cmp = icmp ult i32 %x, %y
          br i1 %cmp, label %then, label %else

        then:
          %add = add i32 %x, %y
          br label %merge

        else:
          %sub = sub i32 %x, %y
          br label %merge

        merge:
          ret void
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = koto_test::parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    auto cfg = koto_test::makeBaseConfig();
    cfg["forObs"]["model"] = 1;
    cfg["forObs"]["innerLoopBoundary"] = 3;
    cfg["forObs"]["outerLoopBoundary"] = 2;
    koto_test::loadConfig(cfg, "forobs");

    ForObsPass Pass;
    koto_test::runModulePass(*Mod, Pass);

    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    bool hasLoopBlock = false;
    for (BasicBlock &BB : *Foo) {
        if (BB.getName().contains("outerLoop") || BB.getName().contains("innerLoop")) {
            hasLoopBlock = true;
            break;
        }
    }
    EXPECT_TRUE(hasLoopBlock);
}

TEST(KotoamatsukamiBogusCFTest, AddsBogusBlocks) {
    const char *IRText = R"IR(
        define void @foo(i32 %x, i32 %y) {
        entry:
          %cmp = icmp ult i32 %x, %y
          br i1 %cmp, label %then, label %else

        then:
          %add = add i32 %x, %y
          br label %merge

        else:
          %sub = sub i32 %x, %y
          br label %merge

        merge:
          ret void
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = koto_test::parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    auto cfg = koto_test::makeBaseConfig();
    cfg["bogusControlFlow"]["model"] = 1;
    koto_test::loadConfig(cfg, "bogus");

    BogusControlFlow Pass;
    koto_test::runModulePass(*Mod, Pass);

    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    bool hasBogusBlock = false;
    for (BasicBlock &BB : *Foo) {
        if (BB.getName().contains("bodyBB") || BB.getName().contains("jump2BodyBB")) {
            hasBogusBlock = true;
            break;
        }
    }
    EXPECT_TRUE(hasBogusBlock);
}

TEST(KotoamatsukamiSplitBBTest, SplitsBasicBlocks) {
    const char *IRText = R"IR(
        define void @foo(i32 %x, i32 %y) {
        entry:
          %add = add i32 %x, %y
          %mul = mul i32 %add, %y
          %sub = sub i32 %mul, %x
          br label %done

        done:
          ret void
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = koto_test::parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    auto cfg = koto_test::makeBaseConfig();
    cfg["splitBasicBlocks"]["model"] = 1;
    cfg["splitBasicBlocks"]["split number"] = 1;
    koto_test::loadConfig(cfg, "split");

    SplitBasicBlock Pass;
    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    size_t origBlocks = Foo->size();

    koto_test::runModulePass(*Mod, Pass);

    EXPECT_GT(Foo->size(), origBlocks);
}

TEST(KotoamatsukamiAddJunkTest, InsertsInlineAsm) {
    const char *IRText = R"IR(
        define i32 @foo(i32 %x, i32 %y) {
        entry:
          %tmp1 = add i32 %x, %y
          br label %cont

        cont:
          %tmp2 = mul i32 %tmp1, 2
          ret i32 %tmp2
        }
    )IR";

    LLVMContext Ctx;
    auto Mod = koto_test::parseModuleFromIR(IRText, Ctx);
    ASSERT_NE(Mod, nullptr);

    auto cfg = koto_test::makeBaseConfig();
    cfg["junkCode"]["model"] = 1;
    cfg["junkCode"]["probability"] = 100;
    koto_test::loadConfig(cfg, "junk");

    AddJunkCodePass Pass;
    koto_test::runModulePass(*Mod, Pass);

    Function *Foo = Mod->getFunction("foo");
    ASSERT_NE(Foo, nullptr);
    bool hasInlineAsm = false;
    for (Instruction &I : instructions(Foo)) {
        if (auto *CB = dyn_cast<CallBase>(&I)) {
            if (CB->isInlineAsm()) {
                hasInlineAsm = true;
                break;
            }
        }
    }
    EXPECT_TRUE(hasInlineAsm);
}

} // namespace

