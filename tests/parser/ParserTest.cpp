#include <asmstudio/parser/AsmParse.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;
using namespace asmstudio::parser;

TEST(Parser, EmptyInput)
{
    auto prog = asm_parse_runtime("");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 0u);
}

TEST(Parser, SingleRet)
{
    auto prog = asm_parse_runtime("ret");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 1u);
}

TEST(Parser, ThreeInstructions)
{
    auto prog = asm_parse_runtime("mov r0, 0\nadd r0, 1\nret");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 3u);
}

TEST(Parser, LabelCounted)
{
    auto prog = asm_parse_runtime("loop:\n  jmp loop");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.labelCount(), 1u);
}

TEST(Parser, MultipleLabels)
{
    auto prog = asm_parse_runtime("start:\n  mov r0, 0\nend:\n  ret");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.labelCount(), 2u);
}

TEST(Parser, CommentIgnored)
{
    auto prog = asm_parse_runtime("; header comment\nmov r0, 0\nret");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 2u);
}

TEST(Parser, AllMnemonics)
{
    auto prog = asm_parse_runtime(
        "mov r0, 0\n"
        "add r0, 1\n"
        "sub r0, 1\n"
        "mul r0, 2\n"
        "div r0, 2\n"
        "ret");
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 6u);
}

TEST(Parser, JumpWithLabel)
{
    auto prog = asm_parse_runtime("loop:\n  jmp loop");
    EXPECT_TRUE(prog.ok());
}

TEST(Parser, ConditionalJumps)
{
    auto prog = asm_parse_runtime(
        "cmp r0, 10\n"
        "jl done\n"
        "done:\n"
        "ret");
    EXPECT_TRUE(prog.ok());
}

// ---------------------------------------------------------------------------
// Compile-time parser tests (consteval)
// ---------------------------------------------------------------------------

TEST(Parser, ConstexprSingleRet)
{
    constexpr auto prog = asm_parse<"ret">();
    static_assert(prog.ok());
    static_assert(prog.instructionCount() == 1u);
    EXPECT_TRUE(prog.ok());
    EXPECT_EQ(prog.instructionCount(), 1u);
}

TEST(Parser, ConstexprThreeInstrs)
{
    constexpr auto prog = asm_parse<"mov r0, 0\nadd r0, 1\nret">();
    static_assert(prog.ok());
    static_assert(prog.instructionCount() == 3u);
    EXPECT_EQ(prog.instructionCount(), 3u);
}

TEST(Parser, ConstexprWithLabel)
{
    constexpr auto prog = asm_parse<"loop:\n  jmp loop">();
    static_assert(prog.ok());
    static_assert(prog.labelCount() == 1u);
    EXPECT_EQ(prog.labelCount(), 1u);
}
