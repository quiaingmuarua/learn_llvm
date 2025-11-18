#include <gtest/gtest.h>

#include "learn_llvm/lang/repl/Lexer.h"
#include "learn_llvm/lang/repl/Parser.h"
#include "learn_llvm/lang/repl/AST.h"

using namespace learn_llvm::lang;

TEST(LangParserTest, ParseSimplePrecedence) {
    // 测试 1 + 2 * 3 的优先级：
    //    +
    //   / \
    //  1   (* 2 3)

    std::string code = "1 + 2 * 3";

    Lexer L(code);
    Parser P(L);
    auto Expr = P.parseExpression();
    ASSERT_NE(Expr, nullptr);

    // 根节点应该是 '+'
    auto *Bin = dynamic_cast<BinaryExpr *>(Expr.get());
    ASSERT_NE(Bin, nullptr);
    EXPECT_EQ(Bin->Op, '+');

    // 左子树：Number(1)
    auto *LHSNum = dynamic_cast<NumberExpr *>(Bin->LHS.get());
    ASSERT_NE(LHSNum, nullptr);
    EXPECT_DOUBLE_EQ(LHSNum->Value, 1.0);

    // 右子树：Binary('*', Number(2), Number(3))
    auto *RHSBin = dynamic_cast<BinaryExpr *>(Bin->RHS.get());
    ASSERT_NE(RHSBin, nullptr);
    EXPECT_EQ(RHSBin->Op, '*');

    auto *RHSLeft = dynamic_cast<NumberExpr *>(RHSBin->LHS.get());
    auto *RHSRight = dynamic_cast<NumberExpr *>(RHSBin->RHS.get());
    ASSERT_NE(RHSLeft, nullptr);
    ASSERT_NE(RHSRight, nullptr);

    EXPECT_DOUBLE_EQ(RHSLeft->Value, 2.0);
    EXPECT_DOUBLE_EQ(RHSRight->Value, 3.0);
}
