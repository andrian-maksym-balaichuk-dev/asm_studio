#ifndef ASMSTUDIO_API_PROGRAM_HPP
#define ASMSTUDIO_API_PROGRAM_HPP

#include <asmstudio/api/Function.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/core/Types.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio
{
class Program
{
public:
    explicit Program(std::string name);

    // Program name (for diagnostics and visualization).
    [[nodiscard]] std::string_view name() const noexcept;

    // Create a new function in the program.
    Function& createFunction(std::string name);
    [[nodiscard]] std::span<const std::unique_ptr<Function>> functions() const noexcept;

    // Lower all functions into IR and validate.
    void build();

    // Run optimizer passes on the IR module.
    void optimize(OptimizationLevel level = OptimizationLevel::Basic);

    // Studio features.
    void showIR() const;
    void showAssembly() const;
    void showControlFlow() const;
    void simulate(std::string_view fn = "main");
    void explain(std::string_view fn) const;
    void visualize(std::string_view outputPath) const;

    // Access to the IR module (if built).
    [[nodiscard]] bool hasIR() const noexcept
    {
        return m_irModule.has_value();
    }
    [[nodiscard]] const IRModule& irModule() const;

    // Diagnostics collected during building/optimizing.
    [[nodiscard]] DiagnosticBag& diagnostics() noexcept;
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept;

private:
    std::string m_name;
    std::vector<std::unique_ptr<Function>> m_functions;
    DiagnosticBag m_diagnostics;
    std::optional<IRModule> m_irModule;
};
} // namespace asmstudio


#endif // ASMSTUDIO_API_PROGRAM_HPP
