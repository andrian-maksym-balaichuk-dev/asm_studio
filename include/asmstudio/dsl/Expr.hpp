#ifndef ASMSTUDIO_DSL_EXPR_HPP
#define ASMSTUDIO_DSL_EXPR_HPP


#include <asmstudio/support/Types.hpp>

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
    std::reference_wrapper<const Variable> variable;
};

struct BinExpr
{
    BinExpr() = default;
    BinExpr(const BinOp operation, ExprNode left, ExprNode right)
    : op{ operation }, leftOperand{ std::move(left) }, rightOperand{ std::move(right) }
    {}

    BinOp op;
    ExprNode leftOperand;
    ExprNode rightOperand;
};

struct UnaryExpr
{
    UnaryExpr() = default;
    UnaryExpr(const UnaryOp operation, ExprNode value) : op{ operation }, operand{ std::move(value) } {}

    UnaryOp op;
    ExprNode operand;
};


[[nodiscard]] inline ExprNode expr(int64_t value)
{
    return ConstExpr{ { value } };
}
[[nodiscard]] inline ExprNode expr(std::uint64_t value)
{
    return ConstExpr{ { value } };
}
[[nodiscard]] inline ExprNode expr(double value)
{
    return ConstExpr{ { value } };
}


[[nodiscard]] inline ExprNode expr(bool value)
{
    return ConstExpr{ { value } };
}
[[nodiscard]] inline ExprNode expr(const Variable& variable)
{
    return VarExpr{ std::cref(variable) };
}


[[nodiscard]] inline ExprNode add(ExprNode leftOperand, ExprNode rightOperand)
{
    return std::make_shared<BinExpr>(BinOp::Add, std::move(leftOperand), std::move(rightOperand));
}
[[nodiscard]] inline ExprNode sub(ExprNode leftOperand, ExprNode rightOperand)
{
    return std::make_shared<BinExpr>(BinOp::Sub, std::move(leftOperand), std::move(rightOperand));
}
[[nodiscard]] inline ExprNode mul(ExprNode leftOperand, ExprNode rightOperand)
{
    return std::make_shared<BinExpr>(BinOp::Mul, std::move(leftOperand), std::move(rightOperand));
}
[[nodiscard]] inline ExprNode div(ExprNode leftOperand, ExprNode rightOperand)
{
    return std::make_shared<BinExpr>(BinOp::Div, std::move(leftOperand), std::move(rightOperand));
}
[[nodiscard]] inline ExprNode mod(ExprNode leftOperand, ExprNode rightOperand)
{
    return std::make_shared<BinExpr>(BinOp::Mod, std::move(leftOperand), std::move(rightOperand));
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


#endif // ASMSTUDIO_DSL_EXPR_HPP
