#ifndef ASMSTUDIO_DSL_FUNCTION_HPP
#define ASMSTUDIO_DSL_FUNCTION_HPP


#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/dsl/Expr.hpp>
#include <asmstudio/dsl/Stmt.hpp>
#include <asmstudio/dsl/Variable.hpp>
#include <asmstudio/support/Compat.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace asmstudio
{
class Function
{
public:
    explicit Function(std::string name);

    [[nodiscard]] std::string_view name() const noexcept;

    Variable& createInt8(std::string name, int8_t initialValue = 0);
    Variable& createInt16(std::string name, int16_t initialValue = 0);
    Variable& createInt(std::string name, int32_t initialValue = 0);
    Variable& createInt64(std::string name, int64_t initialValue = 0);
    Variable& createUInt32(std::string name, uint32_t initialValue = 0);
    Variable& createUInt64(std::string name, uint64_t initialValue = 0);
    Variable& createFloat(std::string name, float initialValue = 0.f);
    Variable& createDouble(std::string name, double initialValue = 0.0);
    Variable& createBool(std::string name, bool initialValue = false);
    Variable& createPtr(std::string name, uint64_t initialAddress = 0);

    [[nodiscard]] compat::Span<const std::unique_ptr<Variable>> variables() const noexcept;

    void assign(Variable& target, ExprNode value);

#ifdef ASM_CXX20
#include <concepts>


    void whileLoop(const Condition cond, std::invocable auto&& body)
    {
        std::vector<Stmt> bodyStmts{};
        auto& prev{ m_currentBody.get() };
        m_currentBody = std::ref(bodyStmts);
        body();
        m_currentBody = std::ref(prev);
        pushStmt(Stmt{ WhileStmt{ cond, std::move(bodyStmts) } });
    }

    void ifStmt(Condition cond, std::invocable auto&& thenBody)
    {
        ifElse(std::move(cond), std::forward<decltype(thenBody)>(thenBody), [] {});
    }

    void ifElse(const Condition cond, std::invocable auto&& thenBody, std::invocable auto&& elseBody)
    {
        std::vector<Stmt> thenStmts{};
        std::vector<Stmt> elseStmts{};
        auto& prev{ m_currentBody.get() };
        m_currentBody = std::ref(thenStmts);
        thenBody();
        m_currentBody = std::ref(elseStmts);
        elseBody();
        m_currentBody = std::ref(prev);
        pushStmt(Stmt{ IfStmt{ cond, std::move(thenStmts), std::move(elseStmts) } });
    }

#else // C++17: SFINAE instead of std::invocable

    template <typename F, typename = std::enable_if_t<std::is_invocable_v<F>>>
    void whileLoop(const Condition cond, F&& body)
    {
        std::vector<Stmt> bodyStmts{};
        auto& prev{ m_currentBody.get() };
        m_currentBody = std::ref(bodyStmts);
        body();
        m_currentBody = std::ref(prev);
        pushStmt(Stmt{ WhileStmt{ cond, std::move(bodyStmts) } });
    }

    template <typename F, typename = std::enable_if_t<std::is_invocable_v<F>>>
    void ifStmt(Condition cond, F&& thenBody)
    {
        ifElse(std::move(cond), std::forward<F>(thenBody), [] {});
    }

    template <typename F, typename G, typename = std::enable_if_t<std::is_invocable_v<F> && std::is_invocable_v<G>>>
    void ifElse(const Condition cond, F&& thenBody, G&& elseBody)
    {
        std::vector<Stmt> thenStmts{};
        std::vector<Stmt> elseStmts{};
        auto& prev{ m_currentBody.get() };
        m_currentBody = std::ref(thenStmts);
        thenBody();
        m_currentBody = std::ref(elseStmts);
        elseBody();
        m_currentBody = std::ref(prev);
        pushStmt(Stmt{ IfStmt{ cond, std::move(thenStmts), std::move(elseStmts) } });
    }

#endif // ASM_CXX20

    void ret(std::optional<ExprNode> value = std::nullopt);
    void call(std::string_view callee, std::vector<ExprNode> arguments = {});

    [[nodiscard]] compat::Span<const Stmt> statements() const noexcept;

private:
    Variable& makeVar(std::string name, DataType type, InitValue initialValue);
    void pushStmt(Stmt statement) const;

    std::string m_name;
    std::vector<std::unique_ptr<Variable>> m_vars;
    std::vector<Stmt> m_stmts;
    std::reference_wrapper<std::vector<Stmt>> m_currentBody{ m_stmts };
};
} // namespace asmstudio


#endif // ASMSTUDIO_DSL_FUNCTION_HPP
