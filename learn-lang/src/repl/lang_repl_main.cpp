#include "learn_llvm/lang/repl/Lexer.h"
#include "learn_llvm/lang/repl/Parser.h"
#include "learn_llvm/lang/repl/AST.h"

#include <iostream>
#include <memory>

using namespace learn_llvm::lang;

void printExpr(const Expr *E, int indent = 0) {
    auto indentStr = std::string(indent, ' ');

    switch (E->Kind) {
    case ExprKind::Number: {
            auto *N = static_cast<const NumberExpr *>(E);
            std::cout << indentStr << "Number(" << N->Value << ")\n";
            break;
    }
    case ExprKind::Variable: {
            auto *V = static_cast<const VariableExpr *>(E);
            std::cout << indentStr << "Variable(" << V->Name << ")\n";
            break;
    }
    case ExprKind::Binary: {
            auto *B = static_cast<const BinaryExpr *>(E);
            std::cout << indentStr << "Binary(" << B->Op << ")\n";
            printExpr(B->LHS.get(), indent + 2);
            printExpr(B->RHS.get(), indent + 2);
            break;
    }
    }
}

int main() {
    std::cout << "Simple lang REPL. Type expressions like: 1 + 2 * 3\n";
    std::cout << "Ctrl-D to exit.\n";

    std::string line;
    while (true) {
        std::cout << ">>> " << std::flush;
        if (!std::getline(std::cin, line))
            break;

        try {
            Lexer L(line);
            Parser P(L);
            auto Expr = P.parseExpression();
            printExpr(Expr.get());
        } catch (const std::exception &ex) {
            std::cerr << "Parse error: " << ex.what() << "\n";
        }
    }

    return 0;
}
