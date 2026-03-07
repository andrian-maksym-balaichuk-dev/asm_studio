#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Variable.hpp>


namespace asmstudio
{
Variable::Variable(std::string name, const DataType type, const InitValue init)
: m_name(std::move(name)), m_type(type), m_init(init)
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

Condition Variable::compare(const CmpKind kind, const Operand rhs) const
{
    return Condition{ std::cref(*this), kind, rhs };
}
} // namespace asmstudio
