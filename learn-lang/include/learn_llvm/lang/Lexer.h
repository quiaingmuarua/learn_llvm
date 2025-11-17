#pragma once

#include <string>

namespace learn_llvm::lang {

enum class TokenKind {
    Eof,
    Identifier,
    Number,
    Plus,      // +
    Minus,     // -
    Star,      // *
    Slash,     // /
    LParen,    // (
    RParen,    // )
    Semicolon, // ;
};

struct Token {
    TokenKind Kind;
    std::string Text; // 源文本（标识符名字、数字字符串等）
    double NumberValue = 0.0; // 如果是数字，解析后的值

    Token(TokenKind K = TokenKind::Eof) : Kind(K) {}
};

class Lexer {
public:
    explicit Lexer(std::string Input);

    // 取下一个 token
    Token next();

private:
    std::string Input;
    std::size_t Pos = 0;

    char peek() const;
    char get();
    void skipWhitespace();
};

} // namespace learn_llvm::lang
