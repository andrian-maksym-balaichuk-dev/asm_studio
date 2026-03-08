#include <asmstudio/optimizer/Optimizer.hpp>

namespace asmstudio
{
[[nodiscard]] bool Optimizer::run(IRFunction& function) const
{
    bool anyChanged{ false };
    bool changed{ true };

    while (changed)
    {
        changed = false;
        for (const auto& [passName, runPass] : m_passes)
        {
            (void)passName;
            if (runPass(function))
            {
                changed = true;
                anyChanged = true;
            }
        }
    }
    return anyChanged;
}

[[nodiscard]] bool Optimizer::run(IRModule& module) const
{
    bool changed{ false };
    for (auto& function : module.functions)
    {
        if (run(function))
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

[[nodiscard]] Optimizer makeOptimizer(const OptimizationLevel level)
{
    Optimizer optimizer{};
    optimizer.setLevel(level);
    return optimizer;
}
} // namespace asmstudio
