#include <asmstudio/api/Function.hpp>

#include <utility>

namespace asmstudio
{
Function::Function(std::string name) : m_name(std::move(name)), m_currentBody(m_stmts) {}

std::string_view Function::name() const noexcept
{
    return m_name;
}

Variable& Function::makeVar(std::string name, DataType type, const InitValue init)
{
    m_vars.push_back(std::make_unique<Variable>(std::move(name), type, init));
    return *m_vars.back();
}

Variable& Function::createInt8(std::string n, const std::int8_t v)
{
    return makeVar(std::move(n), DataType::Int8, InitValue{ static_cast<int64_t>(v) });
}
Variable& Function::createInt16(std::string n, const std::int16_t v)
{
    return makeVar(std::move(n), DataType::Int16, InitValue{ static_cast<int64_t>(v) });
}
Variable& Function::createInt(std::string n, const std::int32_t v)
{
    return makeVar(std::move(n), DataType::Int32, InitValue{ static_cast<int64_t>(v) });
}
Variable& Function::createInt64(std::string n, const std::int64_t v)
{
    return makeVar(std::move(n), DataType::Int64, InitValue{ v });
}
Variable& Function::createUInt32(std::string n, const std::uint32_t v)
{
    return makeVar(std::move(n), DataType::UInt32, InitValue{ static_cast<uint64_t>(v) });
}
Variable& Function::createUInt64(std::string n, const std::uint64_t v)
{
    return makeVar(std::move(n), DataType::UInt64, InitValue{ v });
}
Variable& Function::createFloat(std::string n, const float v)
{
    return makeVar(std::move(n), DataType::Float32, InitValue{ static_cast<double>(v) });
}
Variable& Function::createDouble(std::string n, const double v)
{
    return makeVar(std::move(n), DataType::Float64, InitValue{ v });
}
Variable& Function::createBool(std::string n, const bool v)
{
    return makeVar(std::move(n), DataType::Bool, InitValue{ v });
}
Variable& Function::createPtr(std::string n, const std::uint64_t v)
{
    return makeVar(std::move(n), DataType::Ptr, InitValue{ v });
}

std::span<const std::unique_ptr<Variable>> Function::variables() const noexcept
{
    return m_vars;
}

void Function::pushStmt(Stmt stmt) const
{
    m_currentBody.get().push_back(std::move(stmt));
}

void Function::assign(Variable& target, ExprNode value)
{
    pushStmt(Stmt{ AssignStmt{ std::ref(target), std::move(value) } });
}

void Function::ret(std::optional<ExprNode> value)
{
    pushStmt(Stmt{ ReturnStmt{ std::move(value) } });
}

void Function::call(std::string_view callee, std::vector<ExprNode> args)
{
    pushStmt(Stmt{ CallStmt{ std::string(callee), std::move(args) } });
}

std::span<const Stmt> Function::statements() const noexcept
{
    return m_stmts;
}
} // namespace asmstudio
