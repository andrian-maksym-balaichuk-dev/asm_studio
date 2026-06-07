#ifndef ASMSTUDIO_DSL_VARIABLE_HPP
#define ASMSTUDIO_DSL_VARIABLE_HPP


#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/support/Types.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace asmstudio
{
using InitValue = std::variant<std::int64_t, std::uint64_t, double, bool>;

inline Operand toOperand(const Variable& variable)
{
    return std::cref(variable);
}
template <typename T>
inline Operand toOperand(T value)
{
    return Operand{ value };
}

class Variable
{
public:
    Variable(std::string name, DataType type, InitValue initialValue);

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] DataType type() const noexcept;
    [[nodiscard]] const InitValue& initVariant() const noexcept;

    [[nodiscard]] Condition compare(CmpKind comparisonKind, Operand rightOperand) const;

    template <typename T>
    [[nodiscard]] Condition operator<(const T& rightOperand) const
    {
        return compare(CmpKind::Lt, toOperand(rightOperand));
    }
    template <typename T>
    [[nodiscard]] Condition operator<=(const T& rightOperand) const
    {
        return compare(CmpKind::Le, toOperand(rightOperand));
    }
    template <typename T>
    [[nodiscard]] Condition operator==(const T& rightOperand) const
    {
        return compare(CmpKind::Eq, toOperand(rightOperand));
    }
    template <typename T>
    [[nodiscard]] Condition operator!=(const T& rightOperand) const
    {
        return compare(CmpKind::Ne, toOperand(rightOperand));
    }
    template <typename T>
    [[nodiscard]] Condition operator>=(const T& rightOperand) const
    {
        return compare(CmpKind::Ge, toOperand(rightOperand));
    }
    template <typename T>
    [[nodiscard]] Condition operator>(const T& rightOperand) const
    {
        return compare(CmpKind::Gt, toOperand(rightOperand));
    }

private:
    std::string m_name;
    DataType m_type;
    InitValue m_init;
};
} // namespace asmstudio


#endif // ASMSTUDIO_DSL_VARIABLE_HPP
