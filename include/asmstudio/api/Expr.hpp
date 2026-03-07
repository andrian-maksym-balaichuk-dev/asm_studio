#ifndef ASMSTUDIO_API_EXPR_HPP
#define ASMSTUDIO_API_EXPR_HPP

#include <asmstudio/core/Types.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <variant>

namespace asmstudio
{
class Variable;
struct BinExpr;
struct UnaryExpr;
struct ConstExpr;
struct VarExpr;

using ExprNode = std::variant<ConstExpr, VarExpr, std::shared_ptr<BinExpr>, std::shared_ptr<UnaryExpr>>;

struct ConstExpr
{
    std::variant<int64_t, std::uint64_t, double, bool> value;
};

struct VarExpr
{
    std::reference_wrapper<const Variable> var;
};

struct BinExpr
{
    BinOp op;
    ExprNode lhs;
    ExprNode rhs;
};

struct UnaryExpr
{
    UnaryOp op;
    ExprNode operand;
};


[[nodiscard]] inline constexpr ExprNode expr(int64_t v)
{
    return ConstExpr{ { v } };
}
[[nodiscard]] inline constexpr ExprNode expr(std::uint64_t v)
{
    return ConstExpr{ { v } };
}
[[nodiscard]] inline constexpr ExprNode expr(double v)
{
    return ConstExpr{ { v } };
}
[[nodiscard]] inline constexpr ExprNode expr(bool v)
{
    return ConstExpr{ { v } };
}
[[nodiscard]] inline ExprNode expr(const Variable& v)
{
    return VarExpr{ std::cref(v) };
}


[[nodiscard]] inline ExprNode add(ExprNode lhs, ExprNode rhs)
{
    return std::make_shared<BinExpr>(BinOp::Add, std::move(lhs), std::move(rhs));
}
[[nodiscard]] inline ExprNode sub(ExprNode lhs, ExprNode rhs)
{
    return std::make_shared<BinExpr>(BinOp::Sub, std::move(lhs), std::move(rhs));
}
[[nodiscard]] inline ExprNode mul(ExprNode lhs, ExprNode rhs)
{
    return std::make_shared<BinExpr>(BinOp::Mul, std::move(lhs), std::move(rhs));
}
[[nodiscard]] inline ExprNode div(ExprNode lhs, ExprNode rhs)
{
    return std::make_shared<BinExpr>(BinOp::Div, std::move(lhs), std::move(rhs));
}
[[nodiscard]] inline ExprNode mod(ExprNode lhs, ExprNode rhs)
{
    return std::make_shared<BinExpr>(BinOp::Mod, std::move(lhs), std::move(rhs));
}
[[nodiscard]] inline ExprNode neg(ExprNode operand)
{
    return std::make_shared<UnaryExpr>(UnaryOp::Neg, std::move(operand));
}
[[nodiscard]] inline ExprNode bitnot(ExprNode operand)
{
    return std::make_shared<UnaryExpr>(UnaryOp::BitNot, std::move(operand));
}
} // namespace asmstudio


#endif // ASMSTUDIO_API_EXPR_HPP
