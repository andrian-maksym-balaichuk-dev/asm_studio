#ifndef ASMSTUDIO_TRANSFORMS_OPTIMIZER_HPP
#define ASMSTUDIO_TRANSFORMS_OPTIMIZER_HPP


#include <asmstudio/support/Compat.hpp>
#include <asmstudio/support/Types.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/transforms/PassBase.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio
{
struct PassEntry
{
    std::string name;
    std::function<bool(IRFunction&)> run;
};

class Optimizer
{
public:
    // Register an optimisation pass. Constrained by Pass concept (C++20)
    // or IsPassV trait (C++17).
#ifdef ASM_CXX20
    template <Pass P>
    void addPass(P pass)
    {
        m_passes.push_back({ std::string{ pass.name() }, [passImpl = std::move(pass)](IRFunction& function) mutable {
                                return passImpl.run(function);
                            } });
    }
#else
    template <typename P, typename = std::enable_if_t<IsPassV<P> > >
    void addPass(P pass)
    {
        m_passes.push_back({ std::string{ pass.name() }, [passImpl = std::move(pass)](IRFunction& function) mutable {
                                return passImpl.run(function);
                            } });
    }
#endif

    [[nodiscard]] bool run(IRFunction& function) const;
    [[nodiscard]] bool run(IRModule& module) const;

    void setLevel(OptimizationLevel level);

    [[nodiscard]] std::size_t passCount() const noexcept
    {
        return m_passes.size();
    }

    [[nodiscard]] const std::vector<std::string_view>& lastFiredPasses() const noexcept
    {
        return m_lastFiredPasses;
    }

private:
    [[nodiscard]] bool runFixpoint(IRFunction& function, std::vector<std::string_view>* fired) const;

    std::vector<PassEntry> m_passes;
    mutable std::vector<std::string_view> m_lastFiredPasses;
};

struct ConstantFolding
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "constant-folding";
    }
    [[nodiscard]] bool run(IRFunction& function) const;
};
struct DeadCodeElim
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "dead-code-elim";
    }
    [[nodiscard]] bool run(IRFunction& function) const;
};
struct UnreachableBlockElim
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "unreachable-block-elim";
    }
    [[nodiscard]] bool run(IRFunction& function) const;
};
struct JumpSimplification
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "jump-simplification";
    }
    [[nodiscard]] bool run(IRFunction& function) const;
};

[[nodiscard]] Optimizer makeOptimizer(OptimizationLevel level);

} // namespace asmstudio

#endif // ASMSTUDIO_TRANSFORMS_OPTIMIZER_HPP
