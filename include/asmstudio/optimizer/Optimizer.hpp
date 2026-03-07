#ifndef ASMSTUDIO_OPTIMIZER_OPTIMIZER_HPP
#define ASMSTUDIO_OPTIMIZER_OPTIMIZER_HPP

#include <asmstudio/core/Types.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/optimizer/PassBase.hpp>

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
    template <Pass P>
    void addPass(P pass)
    {
        m_passes.push_back(
            { std::string(pass.name()), [p = std::move(pass)](IRFunction& fn) mutable { return p.run(fn); } });
    }

    bool run(IRFunction& fn) const;
    bool run(IRModule& module) const;

    void setLevel(OptimizationLevel level);

    [[nodiscard]] std::size_t passCount() const noexcept
    {
        return m_passes.size();
    }

private:
    std::vector<PassEntry> m_passes;
};

// ---------------------------------------------------------------------------
// Built-in passes — satisfy the Pass concept.
// ---------------------------------------------------------------------------

struct ConstantFolding
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "constant-folding";
    }
    bool run(IRFunction& fn) const;
};

struct DeadCodeElim
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "dead-code-elim";
    }
    bool run(IRFunction& fn) const;
};

struct UnreachableBlockElim
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "unreachable-block-elim";
    }
    bool run(IRFunction& fn) const;
};

struct JumpSimplification
{
    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return "jump-simplification";
    }
    bool run(IRFunction& fn) const;
};

[[nodiscard]] Optimizer makeOptimizer(OptimizationLevel level);
} // namespace asmstudio


#endif // ASMSTUDIO_OPTIMIZER_OPTIMIZER_HPP
