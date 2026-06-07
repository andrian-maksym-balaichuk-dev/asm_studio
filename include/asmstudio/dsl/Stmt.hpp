#ifndef ASMSTUDIO_DSL_STMT_HPP
#define ASMSTUDIO_DSL_STMT_HPP


#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/dsl/Expr.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace asmstudio
{
class Variable;
class Stmt;

struct AssignStmt
{
    std::reference_wrapper<Variable> target;
    ExprNode value;
};
struct WhileStmt
{
    Condition condition;
    std::vector<Stmt> body;
};
struct IfStmt
{
    Condition condition;
    std::vector<Stmt> thenBody;
    std::vector<Stmt> elseBody;
};
struct ReturnStmt
{
    std::optional<ExprNode> value;
};
struct CallStmt
{
    std::string callee;
    std::vector<ExprNode> arguments;
};

class Stmt
{
public:
    using Variant = std::variant<AssignStmt, WhileStmt, IfStmt, ReturnStmt, CallStmt>;

    template <typename T>
    explicit Stmt(T statement) : m_impl{ std::make_shared<Variant>(std::move(statement)) }
    {}

    [[nodiscard]] const Variant& variant() const noexcept
    {
        return *m_impl;
    }
    [[nodiscard]] Variant& variant() noexcept
    {
        return *m_impl;
    }
    [[nodiscard]] const Variant& var() const noexcept
    {
        return variant();
    }
    [[nodiscard]] Variant& var() noexcept
    {
        return variant();
    }

private:
    std::shared_ptr<Variant> m_impl;
};
} // namespace asmstudio


#endif // ASMSTUDIO_DSL_STMT_HPP
