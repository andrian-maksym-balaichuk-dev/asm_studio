#ifndef ASMSTUDIO_API_CONDITION_HPP
#define ASMSTUDIO_API_CONDITION_HPP


#include <asmstudio/core/Types.hpp>

#include <functional>
#include <variant>

namespace asmstudio
{
class Variable;

using Operand = std::variant<std::reference_wrapper<const Variable>, std::int64_t, std::uint64_t, double, bool>;

struct Condition
{
    Operand leftOperand;
    CmpKind comparisonKind;
    Operand rightOperand;
};
} // namespace asmstudio


#endif // ASMSTUDIO_API_CONDITION_HPP
