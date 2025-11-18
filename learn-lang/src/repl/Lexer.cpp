#include "learn_llvm/lang/repl/Lexer.h"

#include <cctype>

namespace learn_llvm::lang {

Lexer::Lexer(std::string Input)
    : Input(std::move(Input)), Pos(0) {}

char Lexer::peek() const {
    if (Pos >= Input.size())
        return '\0';
    return Input[Pos];
}

char Lexer::get() {
    char C = peek();
    if (C != '\0')
        ++Pos;
    return C;
}

void Lexer::skipWhitespace() {
    while (std::isspace(static_cast<unsigned char>(peek()))) {
        get();
    }
}

Token Lexer::next() {
    skipWhitespace();

    char C = peek();
    if (C == '\0')
        return Token(TokenKind::Eof);

    // 标识符 [a-zA-Z_][a-zA-Z0-9_]*
    if (std::isalpha(static_cast<unsigned char>(C)) || C == '_') {
        std::string Id;
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            Id.push_back(get());
        }
        Token T(TokenKind::Identifier);
        T.Text = std::move(Id);
        return T;
    }

    // 数字 [0-9.]+（简化版）
    if (std::isdigit(static_cast<unsigned char>(C)) || C == '.') {
        std::string NumStr;
        bool HasDot = false;
        while (true) {
            char P = peek();
            if (std::isdigit(static_cast<unsigned char>(P))) {
                NumStr.push_back(get());
            } else if (P == '.' && !HasDot) {
                HasDot = true;
                NumStr.push_back(get());
            } else {
                break;
            }
        }
        Token T(TokenKind::Number);
        T.Text = NumStr;
        T.NumberValue = std::stod(NumStr);
        return T;
    }

    // 单字符符号
    switch (C) {
    case '+':
        get();
        return Token(TokenKind::Plus);
    case '-':
        get();
        return Token(TokenKind::Minus);
    case '*':
        get();
        return Token(TokenKind::Star);
    case '/':
        get();
        return Token(TokenKind::Slash);
    case '(':
        get();
        return Token(TokenKind::LParen);
    case ')':
        get();
        return Token(TokenKind::RParen);
    case ';':
        get();
        return Token(TokenKind::Semicolon);
    default:
        // 未知字符，先吞掉，当成 EOF 结束（以后可以加错误处理）
        get();
        return Token(TokenKind::Eof);
    }
}

} // namespace learn_llvm::lang
