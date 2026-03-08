#include <asmstudio/core/Types.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <vector>

using namespace asmstudio;

// ---------------------------------------------------------------------------
// Identifier wrappers
// ---------------------------------------------------------------------------

TEST(Types, RegisterIdentifiersCompareByValue)
{
    // Given
    const RegisterId first{ 1 };
    const RegisterId second{ 2 };
    const RegisterId sameAsFirst{ 1 };

    // Then
    EXPECT_EQ(first, sameAsFirst);
    EXPECT_NE(first, second);
    EXPECT_LT(first, second);
}

TEST(Types, BlockIdentifiersCompareByValue)
{
    const BlockId entry{ 0 };
    const BlockId body{ 1 };

    EXPECT_EQ(entry, BlockId{ 0 });
    EXPECT_NE(entry, body);
    EXPECT_LT(entry, body);
}

TEST(Types, ValueIdentifiersCompareByValue)
{
    const ValueId leftValue{ 4 };
    const ValueId rightValue{ 9 };

    EXPECT_EQ(leftValue, ValueId{ 4 });
    EXPECT_NE(leftValue, rightValue);
    EXPECT_LT(leftValue, rightValue);
}

TEST(Types, InstructionIdentifiersCompareByValue)
{
    const InstrId leftInstruction{ 7 };
    const InstrId rightInstruction{ 8 };

    EXPECT_EQ(leftInstruction, InstrId{ 7 });
    EXPECT_NE(leftInstruction, rightInstruction);
    EXPECT_LT(leftInstruction, rightInstruction);
}

TEST(Types, IntegerTypeClassificationMatchesDefinitions)
{
    // Given
    const std::vector<DataType> integerTypes{
        DataType::Int8,   DataType::Int16,  DataType::Int32,  DataType::Int64, DataType::UInt8,
        DataType::UInt16, DataType::UInt32, DataType::UInt64, DataType::Bool,  DataType::Ptr,
    };

    // Then
    for (const DataType dataType : integerTypes)
    {
        EXPECT_TRUE(isIntegerType(dataType));
    }

    EXPECT_FALSE(isIntegerType(DataType::Float32));
    EXPECT_FALSE(isIntegerType(DataType::Float64));
    EXPECT_FALSE(isIntegerType(DataType::Void));
}

TEST(Types, FloatTypeClassificationMatchesDefinitions)
{
    EXPECT_TRUE(isFloatType(DataType::Float32));
    EXPECT_TRUE(isFloatType(DataType::Float64));

    EXPECT_FALSE(isFloatType(DataType::Int32));
    EXPECT_FALSE(isFloatType(DataType::Bool));
    EXPECT_FALSE(isFloatType(DataType::Void));
}

TEST(Types, SignedTypeClassificationMatchesDefinitions)
{
    EXPECT_TRUE(isSignedType(DataType::Int8));
    EXPECT_TRUE(isSignedType(DataType::Int16));
    EXPECT_TRUE(isSignedType(DataType::Int32));
    EXPECT_TRUE(isSignedType(DataType::Int64));

    EXPECT_FALSE(isSignedType(DataType::UInt32));
    EXPECT_FALSE(isSignedType(DataType::Float64));
    EXPECT_FALSE(isSignedType(DataType::Bool));
    EXPECT_FALSE(isSignedType(DataType::Ptr));
}

TEST(Types, DataTypeNamesCoverAllPublicDataTypes)
{
    // Then
    EXPECT_EQ(dataTypeName(DataType::Int8), "i8");
    EXPECT_EQ(dataTypeName(DataType::Int16), "i16");
    EXPECT_EQ(dataTypeName(DataType::Int32), "i32");
    EXPECT_EQ(dataTypeName(DataType::Int64), "i64");
    EXPECT_EQ(dataTypeName(DataType::UInt8), "u8");
    EXPECT_EQ(dataTypeName(DataType::UInt16), "u16");
    EXPECT_EQ(dataTypeName(DataType::UInt32), "u32");
    EXPECT_EQ(dataTypeName(DataType::UInt64), "u64");
    EXPECT_EQ(dataTypeName(DataType::Float32), "f32");
    EXPECT_EQ(dataTypeName(DataType::Float64), "f64");
    EXPECT_EQ(dataTypeName(DataType::Bool), "bool");
    EXPECT_EQ(dataTypeName(DataType::Ptr), "ptr");
    EXPECT_EQ(dataTypeName(DataType::Void), "void");
}

TEST(Types, ComparisonNamesCoverAllKinds)
{
    EXPECT_EQ(cmpKindName(CmpKind::Lt), "lt");
    EXPECT_EQ(cmpKindName(CmpKind::Le), "le");
    EXPECT_EQ(cmpKindName(CmpKind::Eq), "eq");
    EXPECT_EQ(cmpKindName(CmpKind::Ne), "ne");
    EXPECT_EQ(cmpKindName(CmpKind::Ge), "ge");
    EXPECT_EQ(cmpKindName(CmpKind::Gt), "gt");
}

TEST(Types, BinaryOperatorNamesCoverAllKinds)
{
    EXPECT_EQ(binOpName(BinOp::Add), "add");
    EXPECT_EQ(binOpName(BinOp::Sub), "sub");
    EXPECT_EQ(binOpName(BinOp::Mul), "mul");
    EXPECT_EQ(binOpName(BinOp::Div), "div");
    EXPECT_EQ(binOpName(BinOp::Mod), "mod");
    EXPECT_EQ(binOpName(BinOp::And), "and");
    EXPECT_EQ(binOpName(BinOp::Or), "or");
    EXPECT_EQ(binOpName(BinOp::Xor), "xor");
    EXPECT_EQ(binOpName(BinOp::Shl), "shl");
    EXPECT_EQ(binOpName(BinOp::Shr), "shr");
}

TEST(Types, OptimizationLevelsRemainDistinct)
{
    EXPECT_NE(OptimizationLevel::None, OptimizationLevel::Basic);
    EXPECT_NE(OptimizationLevel::Basic, OptimizationLevel::Aggressive);
    EXPECT_NE(OptimizationLevel::Aggressive, OptimizationLevel::Experimental);
}
