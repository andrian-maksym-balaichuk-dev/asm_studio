#include <asmstudio/explain/Explain.hpp>

#include <algorithm>
#include <format>
#include <ranges>
#include <sstream>

namespace asmstudio
{
namespace
{
std::size_t countLoops(const IRFunction& fn)
{
    // A loop exists if any block has a back-edge (target with smaller block id).
    std::size_t loops = 0;
    for (const auto& blk : fn.blocks)
    {
        for (const auto& instr : blk.instrs)
        {
            if (instr.trueTarget && instr.trueTarget->value <= blk.id.value)
            {
                ++loops;
            }
        }
    }
    return loops;
}

std::size_t countBranches(const IRFunction& fn)
{
    std::size_t branches = 0;
    for (const auto& blk : fn.blocks)
    {
        for (const auto& instr : blk.instrs)
        {
            if (instr.op == IROp::BrTrue)
            {
                ++branches;
            }
        }
    }

    return branches;
}

bool hasReturn(const IRFunction& fn)
{
    for (const auto& blk : fn.blocks)
    {
        for (const auto& instr : blk.instrs)
        {
            if (instr.op == IROp::Ret)
            {
                return true;
            }
        }
    }

    return false;
}

bool returnsValue(const IRFunction& fn)
{
    for (const auto& blk : fn.blocks)
    {
        for (const auto& instr : blk.instrs)
        {
            if (instr.op == IROp::Ret && !instr.inputs.empty())
            {
                return true;
            }
        }
    }

    return false;
}

} // namespace

std::string explain(const IRFunction& fn)
{
    std::ostringstream out;
    out << "Function '" << fn.name << "':\n";

    out << std::format("  blocks    : {}\n", fn.blocks.size());
    out << std::format("  values    : {}\n", fn.values.size());

    std::size_t loops = countLoops(fn);
    std::size_t branches = countBranches(fn);

    if (loops == 0 && branches == 0)
    {
        out << "  structure : linear (no control flow)\n";
    }
    else
    {
        if (loops > 0)
            out << std::format("  loops     : {} (back-edge count)\n", loops);
        if (branches > 0)
            out << std::format("  branches  : {} conditional branch(es)\n", branches);
    }

    if (hasReturn(fn))
    {
        if (returnsValue(fn))
            out << "  returns   : value\n";
        else
            out << "  returns   : void\n";
    }
    else
    {
        out << "  returns   : falls through (no explicit ret)\n";
    }

    // Count instruction types.
    std::size_t arithmetic = 0, comparisons = 0, calls = 0, loads = 0, stores = 0;
    for (const auto& blk : fn.blocks)
        for (const auto& instr : blk.instrs)
        {
            switch (instr.op)
            {
            case IROp::Add:
            case IROp::Sub:
            case IROp::Mul:
            case IROp::Div:
            case IROp::Mod:
            case IROp::Neg:
            case IROp::And:
            case IROp::Or:
            case IROp::Xor:
            case IROp::Shl:
            case IROp::Shr: ++arithmetic; break;
            case IROp::Cmp: ++comparisons; break;
            case IROp::Call: ++calls; break;
            case IROp::Load: ++loads; break;
            case IROp::Store: ++stores; break;
            default: break;
            }
        }

    if (arithmetic)
    {
        out << std::format("  arithmetic: {} operation(s)\n", arithmetic);
    }
    if (comparisons)
    {
        out << std::format("  comparisons: {}\n", comparisons);
    }
    if (calls)
    {
        out << std::format("  calls     : {} function call(s)\n", calls);
    }
    if (loads)
    {
        out << std::format("  memory loads : {}\n", loads);
    }
    if (stores)
    {
        out << std::format("  memory stores: {}\n", stores);
    }

    return out.str();
}

std::string explain(const IRModule& module)
{
    std::ostringstream out;
    out << "Module '" << module.name << "' — " << module.functions.size() << " function(s)\n\n";

    for (const auto& fn : module.functions)
    {
        out << explain(fn) << '\n';
    }

    return out.str();
}
} // namespace asmstudio
