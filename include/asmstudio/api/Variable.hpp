#ifndef ASMSTUDIO_API_VARIABLE_HPP
#define ASMSTUDIO_API_VARIABLE_HPP

#include <asmstudio/api/Condition.hpp>
#include <asmstudio/core/Types.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace asmstudio
{
using InitValue = std::variant<std::int64_t, std::uint64_t, double, bool>;

inline Operand toOperand(const Variable& v)
{
    return std::cref(v);
}
template <typename T>
inline Operand toOperand(T v)
{
    return Operand{ v };
}

class Variable
{
public:
    Variable(std::string name, DataType type, InitValue init);

    // Variable properties.
    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] DataType type() const noexcept;
    [[nodiscard]] const InitValue& initVariant() const noexcept;

    // Single compare entry-point; operators delegate here.
    [[nodiscard]] Condition compare(CmpKind kind, Operand rhs) const;

    template <typename T>
    [[nodiscard]] Condition operator<(const T& rhs) const
    {
        return compare(CmpKind::Lt, toOperand(rhs));
    }
    template <typename T>
    [[nodiscard]] Condition operator<=(const T& rhs) const
    {
        return compare(CmpKind::Le, toOperand(rhs));
    }
    template <typename T>
    [[nodiscard]] Condition operator==(const T& rhs) const
    {
        return compare(CmpKind::Eq, toOperand(rhs));
    }
    template <typename T>
    [[nodiscard]] Condition operator!=(const T& rhs) const
    {
        return compare(CmpKind::Ne, toOperand(rhs));
    }
    template <typename T>
    [[nodiscard]] Condition operator>=(const T& rhs) const
    {
        return compare(CmpKind::Ge, toOperand(rhs));
    }
    template <typename T>
    [[nodiscard]] Condition operator>(const T& rhs) const
    {
        return compare(CmpKind::Gt, toOperand(rhs));
    }

private:
    std::string m_name;
    DataType m_type;
    InitValue m_init;
};
} // namespace asmstudio


#endif // ASMSTUDIO_API_VARIABLE_HPP
