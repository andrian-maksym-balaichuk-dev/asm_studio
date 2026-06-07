#include <asmstudio/transforms/Optimizer.hpp>

#include <asmstudio/support/Compat.hpp>

namespace asmstudio
{
[[nodiscard]] bool JumpSimplification::run(IRFunction& function) const
{
    bool changed{ false };

    for (auto& block : function.blocks)
    {
        for (auto& instruction : block.instrs)
        {
            if (instruction.op != IROp::BrTrue || instruction.inputs.empty()) { continue; }

            const ValueId conditionValueId{ instruction.inputs[0] };
            if (conditionValueId.value >= function.values.size()) { continue; }

            const auto& conditionIRValue{ function.values[conditionValueId.value] };
            if (!conditionIRValue.constant) { continue; }

            const bool taken{ std::visit(
                compat::Overloaded{
                    [](int64_t value)  noexcept -> bool { return value != 0; },
                    [](uint64_t value) noexcept -> bool { return value != 0u; },
                    [](double value)   noexcept -> bool { return value != 0.0; },
                    [](bool value)     noexcept -> bool { return value; },
                },
                *conditionIRValue.constant) };

            instruction.op          = IROp::Jmp;
            instruction.trueTarget  = taken ? instruction.trueTarget : instruction.falseTarget;
            instruction.falseTarget = std::nullopt;
            instruction.inputs.clear();
            changed = true;
        }
    }

    return changed;
}

} // namespace asmstudio
