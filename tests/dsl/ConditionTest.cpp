#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/dsl/Variable.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

// ---------------------------------------------------------------------------
// Variable-to-literal conditions
// ---------------------------------------------------------------------------

TEST(Condition, VarLtInt)
{
    // Given
    Variable x("x", DataType::Int32, InitValue{ int64_t(0) });

    // When
    Condition c = x < int64_t(10);

    // Then
    EXPECT_EQ(c.comparisonKind, CmpKind::Lt);
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.leftOperand));
    EXPECT_TRUE(std::holds_alternative<int64_t>(c.rightOperand));
    EXPECT_EQ(std::get<int64_t>(c.rightOperand), 10);
}

TEST(Condition, VarLeVar)
{
    // Given
    Variable a("a", DataType::Int32, InitValue{ int64_t(0) });
    Variable b("b", DataType::Int32, InitValue{ int64_t(5) });

    // When
    Condition c = a <= b;

    // Then
    EXPECT_EQ(c.comparisonKind, CmpKind::Le);
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.leftOperand));
    EXPECT_TRUE(std::holds_alternative<std::reference_wrapper<const Variable>>(c.rightOperand));
    auto& leftOperandReference = std::get<std::reference_wrapper<const Variable>>(c.leftOperand).get();
    EXPECT_EQ(leftOperandReference.name(), "a");
    auto& rightOperandReference = std::get<std::reference_wrapper<const Variable>>(c.rightOperand).get();
    EXPECT_EQ(rightOperandReference.name(), "b");
}

TEST(Condition, VarEqInt)
{
    Variable v("v", DataType::Int64, InitValue{ int64_t(42) });
    Condition c = v == int64_t(42);
    EXPECT_EQ(c.comparisonKind, CmpKind::Eq);
    EXPECT_EQ(std::get<int64_t>(c.rightOperand), 42);
}

TEST(Condition, VarNeDouble)
{
    // Given
    Variable v("v", DataType::Float64, InitValue{ 3.14 });

    // When
    Condition c = v != 3.14;

    // Then
    EXPECT_EQ(c.comparisonKind, CmpKind::Ne);
    EXPECT_TRUE(std::holds_alternative<double>(c.rightOperand));
}

TEST(Condition, VarGtInt)
{
    Variable v("v", DataType::Int32, InitValue{ int64_t(0) });
    Condition c = v > int64_t(0);
    EXPECT_EQ(c.comparisonKind, CmpKind::Gt);
}

TEST(Condition, VarGeInt)
{
    Variable v("v", DataType::Int32, InitValue{ int64_t(0) });
    Condition c = v >= int64_t(0);
    EXPECT_EQ(c.comparisonKind, CmpKind::Ge);
}
