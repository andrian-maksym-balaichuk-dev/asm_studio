#include <asmstudio/optimizer/Optimizer.hpp>

#include <asmstudio/core/Compat.hpp>

#include <optional>

namespace asmstudio
{
namespace
{

[[nodiscard]] bool isFoldableOp(IROp opcode) noexcept
{
    switch (opcode)
    {
    case IROp::Add:
    case IROp::Sub:
    case IROp::Mul:
    case IROp::Div:
    case IROp::Mod:
    case IROp::And:
    case IROp::Or:
    case IROp::Xor:
    case IROp::Neg:
    case IROp::Not:
    case IROp::Cmp: return true;
    default: return false;
    }
}

[[nodiscard]] bool allInputsAreConstant(const IRInstr& instruction, const IRFunction& function) noexcept
{
    if (instruction.inputs.empty())
    {
        return false;
    }
    for (const auto inputId : instruction.inputs)
    {
        if (inputId.value >= function.values.size() || !function.values[inputId.value].constant)
        {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::optional<IRConstant> foldCmp(CmpKind kind, int64_t leftValue, int64_t rightValue) noexcept
{
    switch (kind)
    {
    case CmpKind::Lt: return bool{ leftValue < rightValue };
    case CmpKind::Le: return bool{ leftValue <= rightValue };
    case CmpKind::Eq: return bool{ leftValue == rightValue };
    case CmpKind::Ne: return bool{ leftValue != rightValue };
    case CmpKind::Ge: return bool{ leftValue >= rightValue };
    case CmpKind::Gt: return bool{ leftValue > rightValue };
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<IRConstant>
foldBinaryInt(IROp opcode, int64_t leftValue, int64_t rightValue, std::optional<CmpKind> cmpKind) noexcept
{
    switch (opcode)
    {
    case IROp::Add: return leftValue + rightValue;
    case IROp::Sub: return leftValue - rightValue;
    case IROp::Mul: return leftValue * rightValue;
    case IROp::Div: return (rightValue != 0) ? std::optional<IRConstant>{ leftValue / rightValue } : std::nullopt;
    case IROp::Mod: return (rightValue != 0) ? std::optional<IRConstant>{ leftValue % rightValue } : std::nullopt;
    case IROp::And: return leftValue & rightValue;
    case IROp::Or: return leftValue | rightValue;
    case IROp::Xor: return leftValue ^ rightValue;
    case IROp::Cmp: return cmpKind ? foldCmp(*cmpKind, leftValue, rightValue) : std::nullopt;
    default: return std::nullopt;
    }
}

[[nodiscard]] std::optional<IRConstant> foldUnary(IROp opcode, const IRConstant& operand) noexcept
{
    switch (opcode)
    {
    case IROp::Neg:
        return std::visit(
            compat::Overloaded{
                [](int64_t v) noexcept -> IRConstant { return -v; },
                [](uint64_t v) noexcept -> IRConstant { return static_cast<uint64_t>(-static_cast<int64_t>(v)); },
                [](double v) noexcept -> IRConstant { return -v; },
                [](bool v) noexcept -> IRConstant { return !v; },
            },
            operand);
    case IROp::Not:
        return std::visit(
            compat::Overloaded{
                [](int64_t v) noexcept -> IRConstant { return ~v; },
                [](uint64_t v) noexcept -> IRConstant { return ~v; },
                [](double) noexcept -> IRConstant { return double{ 0 }; },
                [](bool v) noexcept -> IRConstant { return !v; },
            },
            operand);
    default:
        return std::nullopt;
    }
}
} // namespace

[[nodiscard]] bool ConstantFolding::run(IRFunction& function) const
{
    static constexpr std::size_t kBinaryInputCount{ 2 };

    bool changed{ false };

    for (auto& block : function.blocks)
    {
        for (auto& instruction : block.instrs)
        {
            if (!allInputsAreConstant(instruction, function) || !instruction.output)
            {
                continue;
            }
            if (!isFoldableOp(instruction.op))
            {
                continue;
            }

            if (instruction.inputs.size() == kBinaryInputCount)
            {
                const IRConstant& leftConstant{ *function.values[instruction.inputs[0].value].constant };
                const IRConstant& rightConstant{ *function.values[instruction.inputs[1].value].constant };

                if (std::holds_alternative<int64_t>(leftConstant) && std::holds_alternative<int64_t>(rightConstant))
                {
                    const int64_t leftValue{ std::get<int64_t>(leftConstant) };
                    const int64_t rightValue{ std::get<int64_t>(rightConstant) };
                    const auto foldedConstant{ foldBinaryInt(instruction.op, leftValue, rightValue, instruction.cmpKind) };

                    if (foldedConstant)
                    {
                        function.values[instruction.output->value].constant = foldedConstant;
                        instruction.op = IROp::Const;
                        instruction.constVal = foldedConstant;
                        instruction.cmpKind = std::nullopt;
                        instruction.inputs.clear();
                        changed = true;
                    }
                }
            }
            else if (instruction.inputs.size() == 1)
            {
                const IRConstant& operand{ *function.values[instruction.inputs[0].value].constant };
                const auto foldedConstant{ foldUnary(instruction.op, operand) };

                if (foldedConstant)
                {
                    function.values[instruction.output->value].constant = foldedConstant;
                    instruction.op = IROp::Const;
                    instruction.constVal = foldedConstant;
                    instruction.inputs.clear();
                    changed = true;
                }
            }
        }
    }
    return changed;
}
} // namespace asmstudio
