#include <asmstudio/dsl/Program.hpp>

#include <asmstudio/codegen/PseudoEmitter.hpp>
#include <asmstudio/support/Compat.hpp>
#include <asmstudio/analysis/Explain.hpp>
#include <asmstudio/ir/Lowering.hpp>
#include <asmstudio/transforms/Optimizer.hpp>
#include <asmstudio/driver/ConsolePresenter.hpp>
#include <asmstudio/sim/Simulator.hpp>
#include <asmstudio/analysis/CfgDot.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace asmstudio
{

Program::Program(std::string name, std::ostream& output) : m_name{ std::move(name) }, m_output{ output } {}

std::string_view Program::name() const noexcept
{
    return m_name;
}

Function& Program::createFunction(std::string name)
{
    m_functions.push_back(std::make_unique<Function>(std::move(name)));
    return *m_functions.back();
}

compat::Span<const std::unique_ptr<Function>> Program::functions() const noexcept
{
    return m_functions;
}

const IRModule& Program::irModule() const
{
    if (!m_irModule)
    {
        throw std::runtime_error{ "build() has not been called" };
    }
    return *m_irModule;
}

void Program::build()
{
    m_diagnostics.clear();

    if (m_functions.empty())
    {
        m_diagnostics.emit(Severity::Warning, "Program '" + std::string{ m_name } + "' has no functions.");
        return;
    }

    m_irModule = lower(m_name, m_functions, m_diagnostics);
}

void Program::optimize(OptimizationLevel level)
{
    // Program orchestrates workflow only; the presenter owns console wording.
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printOptimizeRequiresBuild();
        return;
    }

    Optimizer optimizer{ makeOptimizer(level) };
    const bool changed{ optimizer.run(*m_irModule) };
    presenter.printOptimizerSummary(optimizer.passCount(), changed, optimizer.lastFiredPasses());
}

void Program::showIR() const
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("IR", "call build() first");
        return;
    }

    presenter.printIR(m_name, *m_irModule);
}

void Program::showAssembly() const
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("ASM", "call build() first");
        return;
    }
    presenter.printAssembly(m_name, emitPseudoAsm(*m_irModule));
}

void Program::showControlFlow() const
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("CFG", "call build() first");
        return;
    }
    presenter.printControlFlow(m_name, toDot(*m_irModule));
}

void Program::simulate(std::string_view functionName)
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("SIM", "call build() first");
        return;
    }

    presenter.printSimulationHeader(m_name, functionName);
    Simulator simulator{ *m_irModule };

    auto simulationResult{ simulator.run(functionName) };
    if (!simulationResult)
    {
        presenter.printSimulationError(simulationResult.error());
        return;
    }

    // Capture the trace first so Program stays independent from the final sink.
    std::ostringstream traceOutput{};
    simulator.printTrace(traceOutput);
    presenter.printSimulationTrace(traceOutput.str());
    presenter.printReturnValue(*simulationResult);
}

void Program::explain(std::string_view functionName) const
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("EXPLAIN", "call build() first");
        return;
    }

    const IRFunction* function{ m_irModule->findFunction(functionName) };
    if (function)
    {
        presenter.printExplain(asmstudio::explain(*function));
        return;
    }

    presenter.printMissingFunction(functionName);
}

void Program::visualize(std::string_view outputPath) const
{
    ConsolePresenter presenter{ m_output.get() };
    if (!m_irModule)
    {
        presenter.printNotBuilt("VIZ", "call build() first");
        return;
    }

    const std::string dotOutput{ toDot(*m_irModule) };
    std::ofstream outputFile{ std::string{ outputPath } };
    if (!outputFile)
    {
        presenter.printVisualizationOpenError(outputPath);
        return;
    }

    outputFile << dotOutput;
    presenter.printVisualizationWritten(outputPath);
}

void Program::setOutput(std::ostream& output) noexcept
{
    // Swapping the sink here updates every console-facing feature at once.
    m_output = output;
}

std::ostream& Program::output() noexcept
{
    return m_output.get();
}

const std::ostream& Program::output() const noexcept
{
    return m_output.get();
}

DiagnosticBag& Program::diagnostics() noexcept
{
    return m_diagnostics;
}

const DiagnosticBag& Program::diagnostics() const noexcept
{
    return m_diagnostics;
}

} // namespace asmstudio
