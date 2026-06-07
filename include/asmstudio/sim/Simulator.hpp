#ifndef ASMSTUDIO_SIM_SIMULATOR_HPP
#define ASMSTUDIO_SIM_SIMULATOR_HPP


#include <asmstudio/support/Compat.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/sim/SimState.hpp>

#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace asmstudio
{

enum class SimError
{
    DivisionByZero,
    UnknownFunction,
    StackOverflow,
    TypeMismatch,
    UnreachableInstruction,
    MaxStepsExceeded,
};

[[nodiscard]] constexpr std::string_view simErrorName(SimError e) noexcept
{
    switch (e)
    {
    case SimError::DivisionByZero: return "division by zero";
    case SimError::UnknownFunction: return "unknown function";
    case SimError::StackOverflow: return "stack overflow";
    case SimError::TypeMismatch: return "type mismatch";
    case SimError::UnreachableInstruction: return "unreachable instruction";
    case SimError::MaxStepsExceeded: return "max steps exceeded";
    }
    return "unknown error";
}

class Simulator
{
public:
    static constexpr std::size_t kDefaultMaxSteps{ 100'000 };
    static constexpr std::size_t kDefaultMaxCallDepth{ 64 };

    explicit Simulator(const IRModule& module, std::size_t maxSteps = kDefaultMaxSteps, std::size_t maxCallDepth = kDefaultMaxCallDepth);

    [[nodiscard]] compat::Result<std::optional<RegValue>, SimError> run(std::string_view functionName);

    [[nodiscard]] compat::Result<bool, SimError> step();

    void rewind(std::size_t steps = 1);
    void printTrace(std::ostream& out) const;
    void reset();

    [[nodiscard]] const SimState& state() const noexcept
    {
        return m_state;
    }
    [[nodiscard]] bool done() const noexcept
    {
        return m_done;
    }

private:
    struct FunctionCache
    {
        const IRFunction* function{ nullptr };
        std::unordered_map<BlockId, std::size_t, BlockIdHash> blockIndexById{};
    };

    using StepResult = compat::Result<bool, SimError>;
    using ValueResult = compat::Result<RegValue, SimError>;

    [[nodiscard]] StepResult stepOne();

    [[nodiscard]] StepResult handleConstOp(SimFrame& frame, const IRInstr& instr);
    [[nodiscard]] StepResult handleCopyOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);
    [[nodiscard]] StepResult handleBinaryOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);
    [[nodiscard]] StepResult handleUnaryOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);
    [[nodiscard]] StepResult handleCmpOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);
    [[nodiscard]] StepResult handleJumpOp(SimFrame& frame, const IRInstr& instr);
    [[nodiscard]] StepResult handleBranchOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);
    [[nodiscard]] StepResult handleCallOp(SimFrame& frame, const IRInstr& instr);
    [[nodiscard]] StepResult handleRetOp(SimFrame& frame, const IRInstr& instr, const IRFunction& function);

    [[nodiscard]] ValueResult readValue(const SimFrame& frame, const IRFunction& function, ValueId valueId) const;
    static void writeValue(SimFrame& frame, ValueId id, RegValue val);

    [[nodiscard]] const FunctionCache* findFunctionCache(std::string_view name) const noexcept;
    [[nodiscard]] const IRBlock* findBlock(const FunctionCache& cache, BlockId id) const noexcept;

    const IRModule& m_module;
    std::size_t m_maxSteps;
    std::size_t m_maxCallDepth;
    std::unordered_map<std::string_view, FunctionCache> m_functionCache{};
    SimState m_state{};
    std::vector<SimState> m_history{}; // snapshots for time-travel
    std::vector<RegValue> m_trace{};   // return values per call
    bool m_done{ true };
    std::size_t m_stepCount{ 0 };
};
} // namespace asmstudio


#endif // ASMSTUDIO_SIM_SIMULATOR_HPP
