#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "lexer.h"


class LexerTest : public ::testing::Test
{
public:
    void SetUp() override {
        auto path = std::string(LEXER_TEST_DATA_DIR) + "/lexer_01.txt";
        auto buf = llvm::MemoryBuffer::getFile(path);
        if (!buf) {
            llvm::errs() << "can't open file: " << path << "\n";
            return;
        }

        mgr = std::make_unique<llvm::SourceMgr>();
        diagEngine = std::make_unique<DiagEngine>(*mgr);

        mgr->AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());

        lexer = std::make_unique<Lexer>(*mgr, *diagEngine);
    }

protected:
    std::unique_ptr<llvm::SourceMgr> mgr;
    std::unique_ptr<DiagEngine> diagEngine;
    std::unique_ptr<Lexer> lexer;
};

/*
int aa, b = 4;
aa=1 ;
*/

TEST_F(LexerTest, NextToken) {
    ASSERT_NE(lexer, nullptr);

    /// 正确集
    /// 当前集
    std::vector<Token> expectedVec, curVec;
    expectedVec.push_back(Token{TokenType::kw_int, 1, 1});
    expectedVec.push_back(Token{TokenType::identifier, 1, 5});
    expectedVec.push_back(Token{TokenType::comma, 1, 7});
    expectedVec.push_back(Token{TokenType::identifier, 1, 9});
    expectedVec.push_back(Token{TokenType::equal, 1, 11});
    expectedVec.push_back(Token{TokenType::number, 1, 13});
    expectedVec.push_back(Token{TokenType::semi, 1, 14});
    expectedVec.push_back(Token{TokenType::identifier, 2, 1});
    expectedVec.push_back(Token{TokenType::equal, 2, 3});
    expectedVec.push_back(Token{TokenType::number, 2, 4});
    expectedVec.push_back(Token{TokenType::semi, 2, 6});

    Token tok;
    while (true) {
        lexer->NextToken(tok);
        if (tok.tokenType == TokenType::eof)
            break;
        curVec.push_back(tok);
    }

    ASSERT_EQ(expectedVec.size(), curVec.size());
    for (int i = 0; i < expectedVec.size(); i++) {
        const auto &expected_tok = expectedVec[i];
        const auto &cur_tok = curVec[i];

        EXPECT_EQ(expected_tok.tokenType, cur_tok.tokenType);
        EXPECT_EQ(expected_tok.row, cur_tok.row);
        EXPECT_EQ(expected_tok.col, cur_tok.col);
    }
}
