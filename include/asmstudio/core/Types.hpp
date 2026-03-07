#ifndef ASMSTUDIO_CORE_TYPES_HPP
#define ASMSTUDIO_CORE_TYPES_HPP


#include <cstdint>
#include <string_view>

namespace asmstudio
{
struct RegisterId
{
    std::uint16_t value{};
    auto operator<=>(const RegisterId&) const noexcept = default;
};
struct BlockId
{
    std::uint32_t value{};
    auto operator<=>(const BlockId&) const noexcept = default;
};
struct ValueId
{
    std::uint32_t value{};
    auto operator<=>(const ValueId&) const noexcept = default;
};
struct InstrId
{
    std::uint32_t value{};
    auto operator<=>(const InstrId&) const noexcept = default;
};

// ---------------------------------------------------------------------------
// Enumerations.
// ---------------------------------------------------------------------------

enum class DataType
{
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    Bool,
    Ptr,
    Void,
};

enum class OptimizationLevel
{
    None,
    Basic,
    Aggressive,
    Experimental,
};

enum class BinOp
{
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Shl,
    Shr,
};

enum class UnaryOp
{
    Neg,
    Not,
    BitNot,
};

enum class CmpKind
{
    Lt,
    Le,
    Eq,
    Ne,
    Ge,
    Gt,
};

[[nodiscard]] constexpr bool isIntegerType(const DataType t) noexcept
{
    switch (t)
    {
    case DataType::Int8:
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64:
    case DataType::UInt8:
    case DataType::UInt16:
    case DataType::UInt32:
    case DataType::UInt64:
    case DataType::Bool:
    case DataType::Ptr: return true;
    default: return false;
    }
}

[[nodiscard]] constexpr bool isFloatType(const DataType t) noexcept
{
    return t == DataType::Float32 || t == DataType::Float64;
}

[[nodiscard]] constexpr bool isSignedType(const DataType t) noexcept
{
    return t == DataType::Int8 || t == DataType::Int16 || t == DataType::Int32 || t == DataType::Int64;
}

[[nodiscard]] constexpr std::string_view dataTypeName(const DataType t) noexcept
{
    switch (t)
    {
    case DataType::Int8: return "i8";
    case DataType::Int16: return "i16";
    case DataType::Int32: return "i32";
    case DataType::Int64: return "i64";
    case DataType::UInt8: return "u8";
    case DataType::UInt16: return "u16";
    case DataType::UInt32: return "u32";
    case DataType::UInt64: return "u64";
    case DataType::Float32: return "f32";
    case DataType::Float64: return "f64";
    case DataType::Bool: return "bool";
    case DataType::Ptr: return "ptr";
    case DataType::Void: return "void";
    }
    return "?";
}

[[nodiscard]] constexpr std::string_view cmpKindName(const CmpKind k) noexcept
{
    switch (k)
    {
    case CmpKind::Lt: return "lt";
    case CmpKind::Le: return "le";
    case CmpKind::Eq: return "eq";
    case CmpKind::Ne: return "ne";
    case CmpKind::Ge: return "ge";
    case CmpKind::Gt: return "gt";
    }
    return "?";
}

[[nodiscard]] constexpr std::string_view binOpName(const BinOp op) noexcept
{
    switch (op)
    {
    case BinOp::Add: return "add";
    case BinOp::Sub: return "sub";
    case BinOp::Mul: return "mul";
    case BinOp::Div: return "div";
    case BinOp::Mod: return "mod";
    case BinOp::And: return "and";
    case BinOp::Or: return "or";
    case BinOp::Xor: return "xor";
    case BinOp::Shl: return "shl";
    case BinOp::Shr: return "shr";
    }
    return "?";
}
} // namespace asmstudio


#endif // ASMSTUDIO_CORE_TYPES_HPP
