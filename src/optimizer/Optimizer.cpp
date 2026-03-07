#include <asmstudio/optimizer/Optimizer.hpp>

#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace asmstudio
{
bool Optimizer::run(IRFunction& fn) const
{
    bool anyChanged = false;
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto& [name, run] : m_passes)
        {
            if (run(fn))
            {
                changed = true;
                anyChanged = true;
            }
        }
    }
    return anyChanged;
}

bool Optimizer::run(IRModule& module) const
{
    bool changed = false;
    for (auto& fn : module.functions)
    {
        if (run(fn))
        {
            changed = true;
        }
    }
    return changed;
}

void Optimizer::setLevel(const OptimizationLevel level)
{
    m_passes.clear();

    switch (level)
    {
    case OptimizationLevel::None: break;
    case OptimizationLevel::Basic:
        addPass(ConstantFolding{});
        addPass(DeadCodeElim{});
        break;
    case OptimizationLevel::Aggressive:
    case OptimizationLevel::Experimental:
        addPass(ConstantFolding{});
        addPass(DeadCodeElim{});
        addPass(UnreachableBlockElim{});
        addPass(JumpSimplification{});
        break;
    }
}

Optimizer makeOptimizer(OptimizationLevel level)
{
    Optimizer opt;
    opt.setLevel(level);
    return opt;
}

bool ConstantFolding::run(IRFunction& fn) const
{
    bool changed = false;

    for (auto& blk : fn.blocks)
    {
        for (auto& instr : blk.instrs)
        {
            // Check if all inputs are constant values.
            bool allConst = !instr.inputs.empty();
            for (auto id : instr.inputs)
            {
                if (id.value >= fn.values.size() || !fn.values[id.value].constant)
                {
                    allConst = false;
                    break;
                }
            }

            if (!allConst || !instr.output)
            {
                continue;
            }

            // Only fold simple binary/unary arithmetic and cmp.
            auto isFoldable = [](IROp op) {
                switch (op)
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
            };

            if (!isFoldable(instr.op))
            {
                continue;
            }

            // Simple folding for constant integer binary ops.
            if (instr.inputs.size() == 2)
            {
                const IRConstant& lhsC = *fn.values[instr.inputs[0].value].constant;
                const IRConstant& rhsC = *fn.values[instr.inputs[1].value].constant;

                if (std::holds_alternative<int64_t>(lhsC) && std::holds_alternative<int64_t>(rhsC))
                {
                    int64_t l = std::get<int64_t>(lhsC);
                    int64_t r = std::get<int64_t>(rhsC);
                    std::optional<IRConstant> result;

                    switch (instr.op)
                    {
                    case IROp::Add: result = l + r; break;
                    case IROp::Sub: result = l - r; break;
                    case IROp::Mul: result = l * r; break;
                    case IROp::Div:
                        if (r != 0)
                        {
                            result = l / r;
                        }
                        break;
                    case IROp::Mod:
                        if (r != 0)
                        {
                            result = l % r;
                        }
                        break;
                    case IROp::And: result = l & r; break;
                    case IROp::Or: result = l | r; break;
                    case IROp::Xor: result = l ^ r; break;
                    case IROp::Cmp:
                        if (instr.cmpKind)
                        {
                            bool res = false;
                            switch (*instr.cmpKind)
                            {
                            case CmpKind::Lt: res = l < r; break;
                            case CmpKind::Le: res = l <= r; break;
                            case CmpKind::Eq: res = l == r; break;
                            case CmpKind::Ne: res = l != r; break;
                            case CmpKind::Ge: res = l >= r; break;
                            case CmpKind::Gt: res = l > r; break;
                            }
                            result = res;
                        }
                        break;
                    default: break;
                    }
                    if (result)
                    {
                        fn.values[instr.output->value].constant = result;
                        instr.op = IROp::Const;
                        instr.inputs.clear();
                        instr.constVal = result;
                        instr.cmpKind = std::nullopt;
                        changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

bool DeadCodeElim::run(IRFunction& fn) const
{
    // Collect all used value IDs.
    std::unordered_set<std::uint32_t> used;
    for (const auto& blk : fn.blocks)
    {
        for (const auto& instr : blk.instrs)
        {
            for (auto id : instr.inputs)
            {
                used.insert(id.value);
            }
        }
    }

    bool changed = false;
    for (auto& blk : fn.blocks)
    {
        auto oldSize = blk.instrs.size();
        std::erase_if(blk.instrs, [&](const IRInstr& instr) {
            if (!instr.output)
                return false;
            // Don't remove terminators or calls.
            if (instr.op == IROp::Ret || instr.op == IROp::Jmp || instr.op == IROp::BrTrue || instr.op == IROp::Call ||
                instr.op == IROp::Store)
            {
                return false;
            }

            return used.find(instr.output->value) == used.end();
        });
        if (blk.instrs.size() != oldSize)
            changed = true;
    }
    return changed;
}

bool UnreachableBlockElim::run(IRFunction& fn) const
{
    if (fn.blocks.empty())
        return false;

    std::unordered_set<std::uint32_t> reachable;
    std::vector<BlockId> worklist;
    worklist.push_back(fn.blocks[0].id);
    reachable.insert(fn.blocks[0].id.value);

    while (!worklist.empty())
    {
        BlockId cur = worklist.back();
        worklist.pop_back();
        if (cur.value >= fn.blocks.size())
        {
            continue;
        }

        for (const auto& instr : fn.blocks[cur.value].instrs)
        {
            auto add = [&](BlockId b) {
                if (reachable.insert(b.value).second)
                {
                    worklist.push_back(b);
                }
            };
            if (instr.trueTarget)
            {
                add(*instr.trueTarget);
            }
            if (instr.falseTarget)
            {
                add(*instr.falseTarget);
            }
        }
    }

    auto oldSize = fn.blocks.size();
    std::erase_if(fn.blocks, [&](const IRBlock& blk) { return reachable.find(blk.id.value) == reachable.end(); });
    return fn.blocks.size() != oldSize;
}

bool JumpSimplification::run(IRFunction& fn) const
{
    bool changed = false;
    for (auto& blk : fn.blocks)
    {
        for (auto& instr : blk.instrs)
        {
            if (instr.op != IROp::BrTrue || instr.inputs.empty())
            {
                continue;
            }

            ValueId condId = instr.inputs[0];
            if (condId.value >= fn.values.size())
            {
                continue;
            }

            const auto& val = fn.values[condId.value];
            if (!val.constant)
            {
                continue;
            }

            bool taken = std::visit([]<typename T>(T v) -> bool { return static_cast<bool>(v); }, *val.constant);
            instr.op = IROp::Jmp;
            instr.inputs.clear();
            instr.trueTarget = taken ? instr.trueTarget : instr.falseTarget;
            instr.falseTarget = std::nullopt;
            changed = true;
        }
    }

    return changed;
}
} // namespace asmstudio
