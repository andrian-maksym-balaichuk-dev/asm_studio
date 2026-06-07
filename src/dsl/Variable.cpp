#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/dsl/Variable.hpp>

namespace asmstudio
{
Variable::Variable(std::string name, const DataType type, const InitValue initialValue)
: m_name{ std::move(name) }, m_type{ type }, m_init{ initialValue }
{}

std::string_view Variable::name() const noexcept
{
    return m_name;
}
DataType Variable::type() const noexcept
{
    return m_type;
}
const InitValue& Variable::initVariant() const noexcept
{
    return m_init;
}

Condition Variable::compare(const CmpKind comparisonKind, const Operand rightOperand) const
{
    return Condition{ std::cref(*this), comparisonKind, rightOperand };
}
} // namespace asmstudio
