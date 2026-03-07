#ifndef ASMSTUDIO_API_STMT_HPP
#define ASMSTUDIO_API_STMT_HPP

#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Expr.hpp>

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
    Condition cond;
    std::vector<Stmt> body;
};
struct IfStmt
{
    Condition cond;
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
    std::vector<ExprNode> args;
};

class Stmt
{
public:
    using V = std::variant<AssignStmt, WhileStmt, IfStmt, ReturnStmt, CallStmt>;

    template <typename T>
    explicit Stmt(T t) : m_impl(std::make_shared<V>(std::move(t)))
    {}

    [[nodiscard]] const V& var() const noexcept
    {
        return *m_impl;
    }
    [[nodiscard]] V& var() noexcept
    {
        return *m_impl;
    }

private:
    std::shared_ptr<V> m_impl;
};
} // namespace asmstudio


#endif // ASMSTUDIO_API_STMT_HPP
