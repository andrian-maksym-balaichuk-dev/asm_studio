#include <asmstudio/transforms/Optimizer.hpp>

#include <asmstudio/support/Compat.hpp>

#include <unordered_set>

namespace asmstudio
{
namespace
{
[[nodiscard]] bool isNonElidableOp(IROp opcode) noexcept
{
    switch (opcode)
    {
    case IROp::Ret:
    case IROp::Jmp:
    case IROp::BrTrue:
    case IROp::Call:
    case IROp::Store: return true;
    default: return false;
    }
}

[[nodiscard]] std::unordered_set<std::uint32_t> collectUsedValueIds(const IRFunction& function)
{
    std::unordered_set<std::uint32_t> usedValueIds{};
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            for (const auto inputId : instruction.inputs)
            {
                usedValueIds.insert(inputId.value);
            }
        }
    }
    return usedValueIds;
}
} // namespace

[[nodiscard]] bool DeadCodeElim::run(IRFunction& function) const
{
    const auto usedValueIds{ collectUsedValueIds(function) };

    bool changed{ false };

    for (auto& block : function.blocks)
    {
        const auto previousInstructionCount{ block.instrs.size() };

        asmstudio::compat::ranges::erase_if(block.instrs, [&](const IRInstr& instruction) {
            if (!instruction.output)
            {
                return false;
            }
            if (isNonElidableOp(instruction.op))
            {
                return false;
            }
            return usedValueIds.find(instruction.output->value) == usedValueIds.end();
        });

        if (block.instrs.size() != previousInstructionCount)
        {
            changed = true;
        }
    }
    return changed;
}
} // namespace asmstudio
