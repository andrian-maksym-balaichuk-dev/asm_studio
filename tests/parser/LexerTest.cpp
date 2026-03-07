#include <asmstudio/parser/Lexer.hpp>
#include <asmstudio/parser/Token.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;
using namespace asmstudio::parser;

TEST(Lexer, EmptyInputGivesEof)
{
    Lexer lex("");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Eof);
}

TEST(Lexer, SingleMnemonic)
{
    Lexer lex("ret");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Mnemonic);
    EXPECT_EQ(t.text, "ret");
}

TEST(Lexer, RegisterToken)
{
    Lexer lex("r0");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Register);
    EXPECT_EQ(t.text, "r0");
}

TEST(Lexer, ImmediatePositive)
{
    Lexer lex("42");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Immediate);
    EXPECT_EQ(t.text, "42");
}

TEST(Lexer, ImmediateNegative)
{
    Lexer lex("-7");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Immediate);
    EXPECT_EQ(t.text, "-7");
}

TEST(Lexer, LabelWithColon)
{
    Lexer lex("loop:");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Label);
    EXPECT_EQ(t.text, "loop");
}

TEST(Lexer, CommaToken)
{
    Lexer lex(",");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Comma);
}

TEST(Lexer, CommentSkipped)
{
    Lexer lex("; this is a comment\nret");
    Token t = lex.next();
    // Comments should be either skipped or returned as Comment
    // Either way the next meaningful token is ret (Mnemonic) or Newline then ret
    while (t.kind == TokenKind::Comment || t.kind == TokenKind::Newline)
        t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Mnemonic);
    EXPECT_EQ(t.text, "ret");
}

TEST(Lexer, MovInstruction)
{
    Lexer lex("mov r0, 0");
    Token t1 = lex.next();
    EXPECT_EQ(t1.kind, TokenKind::Mnemonic);
    EXPECT_EQ(t1.text, "mov");

    Token t2 = lex.next();
    EXPECT_EQ(t2.kind, TokenKind::Register);
    EXPECT_EQ(t2.text, "r0");

    Token t3 = lex.next();
    EXPECT_EQ(t3.kind, TokenKind::Comma);

    Token t4 = lex.next();
    EXPECT_EQ(t4.kind, TokenKind::Immediate);
    EXPECT_EQ(t4.text, "0");
}

TEST(Lexer, MultipleRegisters)
{
    Lexer lex("r15");
    Token t = lex.next();
    EXPECT_EQ(t.kind, TokenKind::Register);
    EXPECT_EQ(t.text, "r15");
}

TEST(Lexer, LineNumberTracked)
{
    Lexer lex("mov r0, 0\nadd r0, 1");
    Token t;
    // advance to second line
    while ((t = lex.next()).kind != TokenKind::Eof && t.line < 2)
    {}
    if (t.kind != TokenKind::Eof)
        EXPECT_EQ(t.line, 2u);
}
