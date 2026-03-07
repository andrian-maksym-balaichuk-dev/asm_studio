#include <asmstudio/api/Program.hpp>

#include <print>
#include <utility>

namespace asmstudio {

Program::Program(std::string name)
    : name_(std::move(name))
{
}

std::string_view Program::name() const noexcept { return name_; }

Function& Program::createFunction(std::string name)
{
    functions_.push_back(std::make_unique<Function>(std::move(name)));
    return *functions_.back();
}

std::span<const std::unique_ptr<Function>> Program::functions() const noexcept { return functions_; }

void Program::build()
{
    if(functions_.empty())
    {
        diags_.emit(Severity::Warning, "Program '" + name_ + "' has no functions.");
    }
    // IR lowering, validation, etc. — Milestone 3.
}

void Program::showIR() const
{
    std::println("[IR] Not yet implemented — Milestone 3.");
}

void Program::showAssembly() const
{
    std::println("[ASM] Not yet implemented — Milestone 5.");
}

void Program::showControlFlow() const
{
    std::println("[CFG] Not yet implemented — Milestone 3.");
}

void Program::simulate(std::string_view /*function*/)
{
    std::println("[SIM] Not yet implemented — Milestone 4.");
}

void Program::explain(std::string_view /*function*/) const
{
    std::println("[EXPLAIN] Not yet implemented — Milestone 8.");
}

void Program::visualize(std::string_view /*outputPath*/) const
{
    std::println("[VIZ] Not yet implemented — Milestone 8.");
}

void Program::optimize(OptimizationLevel /*level*/)
{
    std::println("[OPT] Not yet implemented — Milestone 7.");
}

DiagnosticBag& Program::diagnostics() noexcept { return diags_; }

const DiagnosticBag& Program::diagnostics() const noexcept { return diags_; }

} // namespace asmstudio
