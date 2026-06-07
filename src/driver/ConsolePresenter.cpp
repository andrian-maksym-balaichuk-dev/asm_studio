#include <asmstudio/driver/ConsolePresenter.hpp>

#include <asmstudio/support/Types.hpp>

#include <array>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace asmstudio
{
namespace
{
template <typename T>
std::string toString(T&& value)
{
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, std::string>)
    {
        return value;
    }
    else if constexpr (std::is_same_v<ValueType, std::string_view>)
    {
        return std::string{ value };
    }
    else if constexpr (std::is_same_v<ValueType, const char*> || std::is_same_v<ValueType, char*>)
    {
        return value == nullptr ? std::string{} : std::string{ value };
    }
    else if constexpr (std::is_same_v<ValueType, bool>)
    {
        return value ? "true" : "false";
    }
    else
    {
        std::ostringstream outputStream{};
        outputStream << value;
        return outputStream.str();
    }
}

template <typename... Args>
std::string format(std::string_view formatString, Args&&... args)
{
    const std::array<std::string, sizeof...(Args)> argumentStrings{ toString(std::forward<Args>(args))... };

    std::string output{};
    output.reserve(formatString.size() + argumentStrings.size() * 8U);

    std::size_t argumentIndex{ 0 };
    for (std::size_t index{ 0 }; index < formatString.size(); ++index)
    {
        const char currentCharacter{ formatString[index] };
        if (currentCharacter == '{')
        {
            const std::size_t closeIndex{ formatString.find('}', index + 1U) };
            if (closeIndex == std::string_view::npos)
            {
                output += currentCharacter;
                continue;
            }

            if (argumentIndex < argumentStrings.size())
            {
                output += argumentStrings[argumentIndex++];
            }

            index = closeIndex;
            continue;
        }

        output += currentCharacter;
    }

    return output;
}

template <typename... Args>
void write(std::ostream& output, std::string_view fmt, Args&&... args)
{
    output << format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void writeLine(std::ostream& output, std::string_view fmt, Args&&... args)
{
    write(output, fmt, std::forward<Args>(args)...);
    output << '\n';
}
} // namespace

ConsolePresenter::ConsolePresenter(std::ostream& output) noexcept : m_output{ output } {}

void ConsolePresenter::printOptimizeRequiresBuild() const
{
    writeLine(m_output.get(), "[OPT] Call build() before optimize().");
}

void ConsolePresenter::printNotBuilt(std::string_view tag, std::string_view nextStep) const
{
    writeLine(m_output.get(), "[{}] Not built — {}.", tag, nextStep);
}

void ConsolePresenter::printOptimizerSummary(std::size_t passCount, bool changed, const std::vector<std::string_view>& firedPasses) const
{
    writeLine(m_output.get(), "[OPT] {} passes registered, IR {}.", passCount, changed ? "was modified" : "unchanged");
    for (const auto name : firedPasses)
    {
        writeLine(m_output.get(), "  [fired] {}", name);
    }
}

void ConsolePresenter::printIR(std::string_view programName, const IRModule& module) const
{
    std::ostream& output{ m_output.get() };
    writeLine(output, "=== IR: {} ===", programName);
    for (const auto& function : module.functions)
    {
        writeLine(output, "fn {}:", function.name);
        for (const auto& block : function.blocks)
        {
            writeLine(output, "  .{}:", block.name);
            for (const auto& instruction : block.instrs)
            {
                // Keep the IR line assembly-like so debugging lowered output
                // stays close to what the user sees in pseudo assembly.
                write(output, "    ");
                if (instruction.output)
                {
                    write(output, "v{} = ", instruction.output->value);
                }

                write(output, "{}", instruction.opName());

                if (instruction.cmpKind)
                {
                    write(output, ".{}", cmpKindName(*instruction.cmpKind));
                }
                for (const auto inputValueId : instruction.inputs)
                {
                    write(output, " v{}", inputValueId.value);
                }
                if (instruction.constVal)
                {
                    std::visit([&output](auto value) { write(output, " {}", value); }, *instruction.constVal);
                }
                if (instruction.callee)
                {
                    write(output, " {}", *instruction.callee);
                }
                if (instruction.trueTarget)
                {
                    write(output, " -> blk_{}", instruction.trueTarget->value);
                }
                if (instruction.falseTarget)
                {
                    write(output, " else blk_{}", instruction.falseTarget->value);
                }
                output << '\n';
            }
        }
    }
}

void ConsolePresenter::printAssembly(std::string_view programName, std::string_view assembly) const
{
    std::ostream& output{ m_output.get() };
    writeLine(output, "=== Pseudo-Assembly: {} ===", programName);
    write(output, "{}", assembly);
}

void ConsolePresenter::printControlFlow(std::string_view programName, std::string_view dot) const
{
    std::ostream& output{ m_output.get() };
    writeLine(output, "=== Control-Flow Graph (DOT): {} ===", programName);
    write(output, "{}", dot);
}

void ConsolePresenter::printSimulationHeader(std::string_view programName, std::string_view functionName) const
{
    writeLine(m_output.get(), "=== Simulation: {} in '{}' ===", programName, functionName);
}

void ConsolePresenter::printSimulationError(SimError error) const
{
    writeLine(m_output.get(), "  Error: {}", simErrorName(error));
}

void ConsolePresenter::printSimulationTrace(std::string_view trace) const
{
    write(m_output.get(), "{}", trace);
}

void ConsolePresenter::printReturnValue(const std::optional<RegValue>& returnValue) const
{
    std::ostream& output{ m_output.get() };
    if (!returnValue)
    {
        writeLine(output, "  Return value: void");
        return;
    }

    write(output, "  Return value: ");
    // Render the variant here so callers do not need to know how values are
    // spelled in the console output.
    std::visit([&output](auto value) { write(output, "{}", value); }, *returnValue);
    output << '\n';
}

void ConsolePresenter::printExplain(std::string_view explanation) const
{
    write(m_output.get(), "{}", explanation);
}

void ConsolePresenter::printMissingFunction(std::string_view functionName) const
{
    writeLine(m_output.get(), "[EXPLAIN] Function '{}' not found.", functionName);
}

void ConsolePresenter::printVisualizationWritten(std::string_view outputPath) const
{
    writeLine(m_output.get(), "[VIZ] DOT written to '{}'.", outputPath);
}

void ConsolePresenter::printVisualizationOpenError(std::string_view outputPath) const
{
    writeLine(m_output.get(), "[VIZ] Cannot open '{}' for writing.", outputPath);
}
} // namespace asmstudio
