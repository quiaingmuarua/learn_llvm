#pragma once

#include <memory>
#include <string>

namespace learn_llvm::lang {

enum class ExprKind {
    Number,
    Variable,
    Binary,
};

struct Expr {
    ExprKind Kind;
    explicit Expr(ExprKind K) : Kind(K) {}
    virtual ~Expr() = default;
};

struct NumberExpr : Expr {
    double Value;
    explicit NumberExpr(double V)
        : Expr(ExprKind::Number), Value(V) {}
};

struct VariableExpr : Expr {
    std::string Name;
    explicit VariableExpr(std::string N)
        : Expr(ExprKind::Variable), Name(std::move(N)) {}
};

struct BinaryExpr : Expr {
    char Op;
    std::unique_ptr<Expr> LHS;
    std::unique_ptr<Expr> RHS;

    BinaryExpr(char Op,
               std::unique_ptr<Expr> LHS,
               std::unique_ptr<Expr> RHS)
        : Expr(ExprKind::Binary),
          Op(Op),
          LHS(std::move(LHS)),
          RHS(std::move(RHS)) {}
};

using ExprPtr = std::unique_ptr<Expr>;

} // namespace learn_llvm::lang
