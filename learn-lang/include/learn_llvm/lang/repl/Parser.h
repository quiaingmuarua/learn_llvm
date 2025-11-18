#pragma once

#include "learn_llvm/lang/repl/AST.h"
#include "learn_llvm/lang/repl/Lexer.h"

namespace learn_llvm::lang {

    class Parser {
    public:
        explicit Parser(Lexer &L);

        // 解析一个表达式：支持 +, -, *, / 和括号
        ExprPtr parseExpression();

    private:
        Lexer &Lex;
        Token CurTok;

        Token nextToken();

        ExprPtr parsePrimary();
        ExprPtr parseParenExpr();
        ExprPtr parseIdentifierExpr();
        ExprPtr parseNumberExpr();

        ExprPtr parseBinOpRHS(int ExprPrec, ExprPtr LHS);
        int getTokPrecedence() const;
    };

} // namespace learn_llvm::lang
