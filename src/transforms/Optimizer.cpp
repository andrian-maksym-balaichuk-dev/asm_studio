#include <asmstudio/transforms/Optimizer.hpp>

#include <algorithm>

namespace asmstudio
{
[[nodiscard]] bool Optimizer::runFixpoint(IRFunction& function, std::vector<std::string_view>* fired) const
{
    bool anyChanged{ false };
    bool changed{ true };

    while (changed)
    {
        changed = false;
        for (const auto& pass : m_passes)
        {
            if (pass.run(function))
            {
                changed = true;
                anyChanged = true;
                if (fired)
                {
                    const std::string_view name{ pass.name };
                    if (std::find(fired->begin(), fired->end(), name) == fired->end())
                    {
                        fired->push_back(name);
                    }
                }
            }
        }
    }
    return anyChanged;
}

[[nodiscard]] bool Optimizer::run(IRFunction& function) const
{
    return runFixpoint(function, nullptr);
}

[[nodiscard]] bool Optimizer::run(IRModule& module) const
{
    m_lastFiredPasses.clear();
    bool changed{ false };
    for (auto& function : module.functions)
    {
        if (runFixpoint(function, &m_lastFiredPasses))
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
