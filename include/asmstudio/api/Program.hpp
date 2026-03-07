#pragma once

#include <asmstudio/api/Function.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/core/Types.hpp>

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace asmstudio {

class Program
{
public:
    explicit Program(std::string name);

    [[nodiscard]] std::string_view name() const noexcept;

    Function& createFunction(std::string name);
    [[nodiscard]] std::span<const std::unique_ptr<Function>> functions() const noexcept;

    // Pipeline entry point — validates and prepares for downstream passes.
    void build();

    // Studio tools — stubs until the relevant milestones land.
    void showIR() const;
    void showAssembly() const;
    void showControlFlow() const;
    void simulate(std::string_view function = "main");
    void explain(std::string_view function) const;
    void visualize(std::string_view outputPath) const;
    void optimize(OptimizationLevel level = OptimizationLevel::Basic);

    [[nodiscard]] DiagnosticBag&       diagnostics() noexcept;
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept;

private:
    std::string                             name_;
    std::vector<std::unique_ptr<Function>>  functions_;
    DiagnosticBag                           diags_;
};

} // namespace asmstudio
