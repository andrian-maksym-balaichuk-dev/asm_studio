#include <asmstudio/explain/Explain.hpp>

#include <asmstudio/core/Compat.hpp>

#include <sstream>

namespace asmstudio
{
namespace
{
[[nodiscard]] std::size_t countLoops(const IRFunction& function) noexcept
{
    // A back-edge (jump to a block with an equal or smaller index) indicates a loop.
    std::size_t loopCount{ 0 };
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            if (instruction.trueTarget && instruction.trueTarget->value <= block.id.value)
            {
                ++loopCount;
            }
        }
    }
    return loopCount;
}

[[nodiscard]] std::size_t countBranches(const IRFunction& function) noexcept
{
    std::size_t branchCount{ 0 };
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            if (instruction.op == IROp::BrTrue)
            {
                ++branchCount;
            }
        }
    }
    return branchCount;
}

[[nodiscard]] bool hasReturnInstruction(const IRFunction& function) noexcept
{
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            if (instruction.isTerminator() && instruction.op == IROp::Ret)
            {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool returnsValue(const IRFunction& function) noexcept
{
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            if (instruction.op == IROp::Ret && !instruction.inputs.empty())
            {
                return true;
            }
        }
    }
    return false;
}

struct InstructionCounts
{
    std::size_t arithmetic{ 0 };
    std::size_t comparisons{ 0 };
    std::size_t calls{ 0 };
    std::size_t memoryLoads{ 0 };
    std::size_t memoryStores{ 0 };
};

[[nodiscard]] InstructionCounts countInstructionTypes(const IRFunction& function) noexcept
{
    InstructionCounts counts{};
    for (const auto& block : function.blocks)
    {
        for (const auto& instruction : block.instrs)
        {
            if (instruction.isArithmetic())
            {
                ++counts.arithmetic;
                continue;
            }

            switch (instruction.op)
            {
            case IROp::Cmp: ++counts.comparisons; break;
            case IROp::Call: ++counts.calls; break;
            case IROp::Load: ++counts.memoryLoads; break;
            case IROp::Store: ++counts.memoryStores; break;
            default: break;
            }
        }
    }
    return counts;
}

} // namespace

std::string explain(const IRFunction& function)
{
    std::ostringstream output{};
    output << "Function '" << function.name << "':\n";
    output << "  blocks    : " << function.blocks.size() << '\n';
    output << "  values    : " << function.values.size() << '\n';

    const std::size_t loopCount{ countLoops(function) };
    const std::size_t branchCount{ countBranches(function) };

    if (loopCount == 0 && branchCount == 0)
    {
        output << "  structure : linear (no control flow)\n";
    }
    else
    {
        if (loopCount > 0)
        {
            output << "  loops     : " << loopCount << " (back-edge count)\n";
        }
        if (branchCount > 0)
        {
            output << "  branches  : " << branchCount << " conditional branch(es)\n";
        }
    }

    if (hasReturnInstruction(function))
    {
        output << (returnsValue(function) ? "  returns   : value\n" : "  returns   : void\n");
    }
    else
    {
        output << "  returns   : falls through (no explicit ret)\n";
    }

    const auto [arithmetic, comparisons, calls, memoryLoads, memoryStores]{ countInstructionTypes(function) };
    if (arithmetic > 0)
    {
        output << "  arithmetic: " << arithmetic << " operation(s)\n";
    }
    if (comparisons > 0)
    {
        output << "  comparisons: " << comparisons << '\n';
    }
    if (calls > 0)
    {
        output << "  calls     : " << calls << " function call(s)\n";
    }
    if (memoryLoads > 0)
    {
        output << "  memory loads : " << memoryLoads << '\n';
    }
    if (memoryStores > 0)
    {
        output << "  memory stores: " << memoryStores << '\n';
    }

    return output.str();
}

std::string explain(const IRModule& module)
{
    std::ostringstream output{};
    output << "Module '" << module.name << "' — " << module.functions.size() << " function(s)\n\n";

    for (const auto& function : module.functions)
    {
        output << explain(function) << '\n';
    }

    return output.str();
}

} // namespace asmstudio
