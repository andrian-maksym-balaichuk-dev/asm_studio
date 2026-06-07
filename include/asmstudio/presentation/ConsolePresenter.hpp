#ifndef ASMSTUDIO_PRESENTATION_CONSOLEPRESENTER_HPP
#define ASMSTUDIO_PRESENTATION_CONSOLEPRESENTER_HPP


#include <asmstudio/core/Compat.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/simulator/Simulator.hpp>

#include <functional>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

namespace asmstudio
{
class ConsolePresenter
{
public:
    explicit ConsolePresenter(std::ostream& output) noexcept;

    void printOptimizeRequiresBuild() const;
    void printNotBuilt(std::string_view tag, std::string_view nextStep) const;
    void printOptimizerSummary(std::size_t passCount, bool changed, const std::vector<std::string_view>& firedPasses) const;
    void printIR(std::string_view programName, const IRModule& module) const;
    void printAssembly(std::string_view programName, std::string_view assembly) const;
    void printControlFlow(std::string_view programName, std::string_view dot) const;
    void printSimulationHeader(std::string_view programName, std::string_view functionName) const;
    void printSimulationError(SimError error) const;
    void printSimulationTrace(std::string_view trace) const;
    void printReturnValue(const std::optional<RegValue>& returnValue) const;
    void printExplain(std::string_view explanation) const;
    void printMissingFunction(std::string_view functionName) const;
    void printVisualizationWritten(std::string_view outputPath) const;
    void printVisualizationOpenError(std::string_view outputPath) const;

private:
    std::reference_wrapper<std::ostream> m_output;
};
} // namespace asmstudio

#endif // ASMSTUDIO_PRESENTATION_CONSOLEPRESENTER_HPP
