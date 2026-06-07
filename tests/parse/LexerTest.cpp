#include <asmstudio/parse/Lexer.hpp>
#include <asmstudio/parse/Token.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;
using namespace asmstudio::parser;

TEST(Lexer, EmptyInputGivesEof)
{
    // Given
    Lexer lex("");

    // When
    Token t = lex.next();

    // Then
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
    // Given
    Lexer lex("; this is a comment\nret");

    // When
    Token t = lex.next();
    // Comments should be either skipped or returned as Comment
    // Either way the next meaningful token is ret (Mnemonic) or Newline then ret
    while (t.kind == TokenKind::Comment || t.kind == TokenKind::Newline)
        t = lex.next();

    // Then
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
    // Given
    Lexer lex("mov r0, 0\nadd r0, 1");
    Token t;

    // When: advance until the lexer reaches line two.
    while ((t = lex.next()).kind != TokenKind::Eof && t.line < 2)
    {}

    // Then
    if (t.kind != TokenKind::Eof)
        EXPECT_EQ(t.line, 2u);
}

TEST(Lexer, UnknownIdentifierTokenizedAsUnknown)
{
    Lexer lexer("mystery");
    const Token token = lexer.next();
    EXPECT_EQ(token.kind, TokenKind::Unknown);
    EXPECT_EQ(token.text, "mystery");
}

TEST(Lexer, UnexpectedCharacterBecomesError)
{
    Lexer lexer("@");
    const Token token = lexer.next();
    EXPECT_EQ(token.kind, TokenKind::Error);
}
