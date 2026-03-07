#ifndef ASMSTUDIO_SIMULATOR_SIMULATOR_HPP
#define ASMSTUDIO_SIMULATOR_SIMULATOR_HPP


#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/simulator/SimState.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <ostream>
#include <string_view>
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

[[nodiscard]] constexpr std::string_view simErrorName(const SimError e) noexcept
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
    static constexpr std::size_t DefaultMaxSteps = 100'000;
    static constexpr std::size_t DefaultMaxCallDepth = 64;

    explicit Simulator(const IRModule& module, std::size_t maxSteps = DefaultMaxSteps, std::size_t maxCallDepth = DefaultMaxCallDepth);

    [[nodiscard]] std::expected<std::optional<RegValue>, SimError> run(std::string_view functionName);

    [[nodiscard]] std::expected<bool, SimError> step();

    void rewind(std::size_t steps = 1);

    void printTrace(std::ostream& out) const;

    [[nodiscard]] const SimState& state() const noexcept
    {
        return m_state;
    }
    [[nodiscard]] bool done() const noexcept
    {
        return m_done;
    }

    void reset();

private:
    [[nodiscard]] std::expected<bool, SimError> stepOne();
    [[nodiscard]] const IRFunction* findFunction(std::string_view name) const noexcept;

    const IRModule& m_module;
    std::size_t m_maxSteps;
    std::size_t m_maxCallDepth;
    SimState m_state;
    std::vector<SimState> m_history; // snapshots for time-travel
    std::vector<RegValue> m_trace;   // return values per call
    bool m_done{ true };
    std::size_t m_stepCount{ 0 };
};
} // namespace asmstudio


#endif // ASMSTUDIO_SIMULATOR_SIMULATOR_HPP
