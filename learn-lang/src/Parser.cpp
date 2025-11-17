#include "learn_llvm/lang/Parser.h"

#include <map>
#include <stdexcept>

namespace learn_llvm::lang {

Parser::Parser(Lexer &L)
    : Lex(L) {
    CurTok = Lex.next();
}

Token Parser::nextToken() {
    CurTok = Lex.next();
    return CurTok;
}

int Parser::getTokPrecedence() const {
    switch (CurTok.Kind) {
    case TokenKind::Plus:
    case TokenKind::Minus:
        return 20;
    case TokenKind::Star:
    case TokenKind::Slash:
        return 40;
    default:
        return -1;
    }
}

ExprPtr Parser::parseNumberExpr() {
    if (CurTok.Kind != TokenKind::Number)
        throw std::runtime_error("expected number");
    double Val = CurTok.NumberValue;
    nextToken(); // 吃掉 number
    return std::make_unique<NumberExpr>(Val);
}

ExprPtr Parser::parseIdentifierExpr() {
    if (CurTok.Kind != TokenKind::Identifier)
        throw std::runtime_error("expected identifier");
    std::string Name = CurTok.Text;
    nextToken(); // 吃掉 identifier
    return std::make_unique<VariableExpr>(std::move(Name));
}

ExprPtr Parser::parseParenExpr() {
    // 当前应该是 '('
    nextToken(); // 吃掉 '('
    auto Expr = parseExpression();
    if (CurTok.Kind != TokenKind::RParen)
        throw std::runtime_error("expected ')'");
    nextToken(); // 吃掉 ')'
    return Expr;
}

ExprPtr Parser::parsePrimary() {
    switch (CurTok.Kind) {
    case TokenKind::Identifier:
        return parseIdentifierExpr();
    case TokenKind::Number:
        return parseNumberExpr();
    case TokenKind::LParen:
        return parseParenExpr();
    default:
        throw std::runtime_error("unknown token when expecting an expression");
    }
}

ExprPtr Parser::parseBinOpRHS(int ExprPrec, ExprPtr LHS) {
    while (true) {
        int TokPrec = getTokPrecedence();

        if (TokPrec < ExprPrec)
            return LHS;

        // 记录当前运算符，然后吃掉它
        char BinOp;
        switch (CurTok.Kind) {
        case TokenKind::Plus:  BinOp = '+'; break;
        case TokenKind::Minus: BinOp = '-'; break;
        case TokenKind::Star:  BinOp = '*'; break;
        case TokenKind::Slash: BinOp = '/'; break;
        default:
            return LHS;
        }
        nextToken(); // 吃掉操作符

        // 解析右侧 primary
        auto RHS = parsePrimary();

        // 如果右侧还有更高优先级的运算符，则继续吃掉
        int NextPrec = getTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = parseBinOpRHS(TokPrec + 1, std::move(RHS));
        }

        LHS = std::make_unique<BinaryExpr>(BinOp, std::move(LHS), std::move(RHS));
    }
}

ExprPtr Parser::parseExpression() {
    auto LHS = parsePrimary();
    return parseBinOpRHS(0, std::move(LHS));
}

} // namespace learn_llvm::lang
