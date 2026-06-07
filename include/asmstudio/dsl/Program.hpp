#ifndef ASMSTUDIO_DSL_PROGRAM_HPP
#define ASMSTUDIO_DSL_PROGRAM_HPP


#include <asmstudio/dsl/Function.hpp>
#include <asmstudio/support/Compat.hpp>
#include <asmstudio/support/Diagnostic.hpp>
#include <asmstudio/support/Types.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio
{
class Program
{
public:
    explicit Program(std::string name, std::ostream& output = std::cout);

    [[nodiscard]] std::string_view name() const noexcept;

    Function& createFunction(std::string name);
    [[nodiscard]] compat::Span<const std::unique_ptr<Function>> functions() const noexcept;

    void build();

    void optimize(OptimizationLevel level = OptimizationLevel::Basic);

    void showIR() const;
    void showAssembly() const;
    void showControlFlow() const;
    void simulate(std::string_view functionName = "main");
    void explain(std::string_view functionName) const;
    void visualize(std::string_view outputPath) const;

    void setOutput(std::ostream& output) noexcept;
    [[nodiscard]] std::ostream& output() noexcept;
    [[nodiscard]] const std::ostream& output() const noexcept;

    [[nodiscard]] bool hasIR() const noexcept
    {
        return m_irModule.has_value();
    }
    [[nodiscard]] const IRModule& irModule() const;

    [[nodiscard]] DiagnosticBag& diagnostics() noexcept;
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept;

private:
    std::string m_name;
    std::vector<std::unique_ptr<Function>> m_functions;
    DiagnosticBag m_diagnostics;
    std::optional<IRModule> m_irModule;
    std::reference_wrapper<std::ostream> m_output;
};
} // namespace asmstudio


#endif // ASMSTUDIO_DSL_PROGRAM_HPP
