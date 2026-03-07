#include <asmstudio/api/Program.hpp>
#include <asmstudio/api/Variable.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

TEST(Program, NameRoundtrip)
{
    Program p("MyProg");
    EXPECT_EQ(p.name(), "MyProg");
}

TEST(Program, StartsWithNoFunctions)
{
    Program p("empty");
    EXPECT_TRUE(p.functions().empty());
}

TEST(Program, CreateFunctionReturnsRef)
{
    Program p("prog");
    Function& fn = p.createFunction("main");
    EXPECT_EQ(fn.name(), "main");
    EXPECT_EQ(p.functions().size(), 1u);
}

TEST(Program, CreateMultipleFunctions)
{
    Program p("prog");
    p.createFunction("init");
    p.createFunction("loop");
    p.createFunction("done");
    EXPECT_EQ(p.functions().size(), 3u);
}

TEST(Program, BuildEmptyProgramEmitsWarning)
{
    Program p("prog");
    p.build();
    EXPECT_FALSE(p.diagnostics().hasErrors());
    EXPECT_FALSE(p.diagnostics().empty()); // warning for no functions
}

TEST(Program, BuildWithFunctionNoDiagnostics)
{
    Program p("prog");
    p.createFunction("main");
    p.build();
    EXPECT_TRUE(p.diagnostics().empty());
}

TEST(Function, NameRoundtrip)
{
    Function fn("compute");
    EXPECT_EQ(fn.name(), "compute");
}

TEST(Function, CreateIntVariable)
{
    Function fn("fn");
    Variable& v = fn.createInt("counter", 42);
    EXPECT_EQ(v.name(), "counter");
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), 42);
    EXPECT_EQ(v.type(), DataType::Int32);
    EXPECT_EQ(fn.variables().size(), 1u);
}

TEST(Function, CreateInt64Variable)
{
    Function fn("fn");
    Variable& v = fn.createInt64("big", 1'000'000'000LL);
    EXPECT_EQ(v.type(), DataType::Int64);
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), 1'000'000'000LL);
}

TEST(Function, CreateMultipleVariables)
{
    Function fn("fn");
    fn.createInt("a", 0);
    fn.createInt("b", 1);
    fn.createInt("c", 2);
    EXPECT_EQ(fn.variables().size(), 3u);
}

TEST(Variable, BasicProperties)
{
    Variable v("x", DataType::Int32, InitValue{int64_t(-7)});
    EXPECT_EQ(v.name(), "x");
    EXPECT_EQ(v.type(), DataType::Int32);
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), -7);
}
