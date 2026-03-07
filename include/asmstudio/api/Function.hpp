#ifndef ASMSTUDIO_API_FUNCTION_HPP
#define ASMSTUDIO_API_FUNCTION_HPP

#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Stmt.hpp>
#include <asmstudio/api/Variable.hpp>

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio
{
class Function
{
public:
    explicit Function(std::string name);

    [[nodiscard]] std::string_view name() const noexcept;

    Variable& createInt8(std::string name, int8_t init = 0);
    Variable& createInt16(std::string name, int16_t init = 0);
    Variable& createInt(std::string name, int32_t init = 0);
    Variable& createInt64(std::string name, int64_t init = 0);
    Variable& createUInt32(std::string name, uint32_t init = 0);
    Variable& createUInt64(std::string name, uint64_t init = 0);
    Variable& createFloat(std::string name, float init = 0.f);
    Variable& createDouble(std::string name, double init = 0.0);
    Variable& createBool(std::string name, bool init = false);
    Variable& createPtr(std::string name, uint64_t addr = 0);

    [[nodiscard]] std::span<const std::unique_ptr<Variable>> variables() const noexcept;

    void assign(Variable& target, ExprNode value);

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

    void ret(std::optional<ExprNode> value = std::nullopt);
    void call(std::string_view callee, std::vector<ExprNode> args = {});

    [[nodiscard]] std::span<const Stmt> statements() const noexcept;

private:
    Variable& makeVar(std::string name, DataType type, InitValue init);
    void pushStmt(Stmt stmt) const;

    std::string m_name;
    std::vector<std::unique_ptr<Variable>> m_vars;
    std::vector<Stmt> m_stmts;
    std::reference_wrapper<std::vector<Stmt>> m_currentBody{ m_stmts };
};
} // namespace asmstudio


#endif // ASMSTUDIO_API_FUNCTION_HPP
