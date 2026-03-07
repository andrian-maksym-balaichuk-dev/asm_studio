#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Variable.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

TEST(Condition, VarLtInt)
{
    Variable x("x", DataType::Int32, InitValue{ int64_t(0) });
    Condition c = x < int64_t(10);
    EXPECT_EQ(c.kind, CmpKind::Lt);
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.lhs));
    EXPECT_TRUE(std::holds_alternative<int64_t>(c.rhs));
    EXPECT_EQ(std::get<int64_t>(c.rhs), 10);
}

TEST(Condition, VarLeVar)
{
    Variable a("a", DataType::Int32, InitValue{ int64_t(0) });
    Variable b("b", DataType::Int32, InitValue{ int64_t(5) });
    Condition c = a <= b;
    EXPECT_EQ(c.kind, CmpKind::Le);
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.lhs));
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.rhs));
    auto& lhsRef = std::get<std::reference_wrapper<const Variable>>(c.lhs).get();
    EXPECT_EQ(lhsRef.name(), "a");
    auto& rhsRef = std::get<std::reference_wrapper<const Variable>>(c.rhs).get();
    EXPECT_EQ(rhsRef.name(), "b");
}

TEST(Condition, VarEqInt)
{
    Variable v("v", DataType::Int64, InitValue{ int64_t(42) });
    Condition c = v == int64_t(42);
    EXPECT_EQ(c.kind, CmpKind::Eq);
    EXPECT_EQ(std::get<int64_t>(c.rhs), 42);
}

TEST(Condition, VarNeDouble)
{
    Variable v("v", DataType::Float64, InitValue{ 3.14 });
    Condition c = v != 3.14;
    EXPECT_EQ(c.kind, CmpKind::Ne);
    EXPECT_TRUE(std::holds_alternative<double>(c.rhs));
}

TEST(Condition, VarGtInt)
{
    Variable v("v", DataType::Int32, InitValue{ int64_t(0) });
    Condition c = v > int64_t(0);
    EXPECT_EQ(c.kind, CmpKind::Gt);
}

TEST(Condition, VarGeInt)
{
    Variable v("v", DataType::Int32, InitValue{ int64_t(0) });
    Condition c = v >= int64_t(0);
    EXPECT_EQ(c.kind, CmpKind::Ge);
}
