#include <asmstudio/dsl/Function.hpp>

#include <utility>

namespace asmstudio
{
Function::Function(std::string name) : m_name{ std::move(name) }, m_currentBody{ m_stmts } {}

std::string_view Function::name() const noexcept
{
    return m_name;
}

Variable& Function::makeVar(std::string name, DataType type, const InitValue initialValue)
{
    m_vars.push_back(std::make_unique<Variable>(std::move(name), type, initialValue));
    return *m_vars.back();
}

Variable& Function::createInt8(std::string name, const std::int8_t initialValue)
{
    return makeVar(std::move(name), DataType::Int8, InitValue{ static_cast<int64_t>(initialValue) });
}
Variable& Function::createInt16(std::string name, const std::int16_t initialValue)
{
    return makeVar(std::move(name), DataType::Int16, InitValue{ static_cast<int64_t>(initialValue) });
}
Variable& Function::createInt(std::string name, const std::int32_t initialValue)
{
    return makeVar(std::move(name), DataType::Int32, InitValue{ static_cast<int64_t>(initialValue) });
}
Variable& Function::createInt64(std::string name, const std::int64_t initialValue)
{
    return makeVar(std::move(name), DataType::Int64, InitValue{ initialValue });
}
Variable& Function::createUInt32(std::string name, const std::uint32_t initialValue)
{
    return makeVar(std::move(name), DataType::UInt32, InitValue{ static_cast<uint64_t>(initialValue) });
}
Variable& Function::createUInt64(std::string name, const std::uint64_t initialValue)
{
    return makeVar(std::move(name), DataType::UInt64, InitValue{ initialValue });
}
Variable& Function::createFloat(std::string name, const float initialValue)
{
    return makeVar(std::move(name), DataType::Float32, InitValue{ static_cast<double>(initialValue) });
}
Variable& Function::createDouble(std::string name, const double initialValue)
{
    return makeVar(std::move(name), DataType::Float64, InitValue{ initialValue });
}
Variable& Function::createBool(std::string name, const bool initialValue)
{
    return makeVar(std::move(name), DataType::Bool, InitValue{ initialValue });
}
Variable& Function::createPtr(std::string name, const std::uint64_t initialAddress)
{
    return makeVar(std::move(name), DataType::Ptr, InitValue{ initialAddress });
}

compat::Span<const std::unique_ptr<Variable>> Function::variables() const noexcept
{
    return m_vars;
}

void Function::pushStmt(Stmt statement) const
{
    m_currentBody.get().push_back(std::move(statement));
}

void Function::assign(Variable& target, ExprNode value)
{
    pushStmt(Stmt{ AssignStmt{ std::ref(target), std::move(value) } });
}

void Function::ret(std::optional<ExprNode> value)
{
    pushStmt(Stmt{ ReturnStmt{ std::move(value) } });
}

void Function::call(std::string_view callee, std::vector<ExprNode> arguments)
{
    pushStmt(Stmt{ CallStmt{ std::string(callee), std::move(arguments) } });
}

compat::Span<const Stmt> Function::statements() const noexcept
{
    return m_stmts;
}
} // namespace asmstudio
