#include <asmstudio/api/Program.hpp>

#include <asmstudio/backend/PseudoEmitter.hpp>
#include <asmstudio/explain/Explain.hpp>
#include <asmstudio/lowering/Lowering.hpp>
#include <asmstudio/optimizer/Optimizer.hpp>
#include <asmstudio/simulator/Simulator.hpp>
#include <asmstudio/visualization/CfgDot.hpp>

#include <fstream>
#include <iostream>
#include <print>
#include <stdexcept>
#include <utility>

namespace asmstudio
{
Program::Program(std::string name) : m_name(std::move(name)) {}

std::string_view Program::name() const noexcept
{
    return m_name;
}

Function& Program::createFunction(std::string name)
{
    m_functions.push_back(std::make_unique<Function>(std::move(name)));
    return *m_functions.back();
}

std::span<const std::unique_ptr<Function>> Program::functions() const noexcept
{
    return m_functions;
}

const IRModule& Program::irModule() const
{
    if (!m_irModule)
    {
        throw std::runtime_error("build() has not been called");
    }

    return *m_irModule;
}

void Program::build()
{
    m_diagnostics.clear();

    if (m_functions.empty())
    {
        m_diagnostics.emit(Severity::Warning, "Program '" + std::string(m_name) + "' has no functions.");
        return;
    }

    m_irModule = lower(m_name, m_functions, m_diagnostics);
}

void Program::optimize(OptimizationLevel level)
{
    if (!m_irModule)
    {
        std::println("[OPT] Call build() before optimize().");
        return;
    }

    Optimizer opt = makeOptimizer(level);
    bool changed = opt.run(*m_irModule);

    std::println("[OPT] {} passes ran, IR {}.", opt.passCount(), changed ? "was modified" : "unchanged");
}

void Program::showIR() const
{
    if (!m_irModule)
    {
        std::println("[IR] Not built — call build() first.");
        return;
    }

    std::println("=== IR: {} ===", m_name);
    for (const auto& fn : m_irModule->functions)
    {
        std::println("fn {}:", fn.name);
        for (const auto& blk : fn.blocks)
        {
            std::println("  .{}:", blk.name);
            for (const auto& instr : blk.instrs)
            {
                std::print("    ");
                if (instr.output)
                {
                    std::print("v{} = ", instr.output->value);
                }

                std::print("{}", [&] -> const char* {
                    switch (instr.op)
                    {
                    case IROp::Const: return "const";
                    case IROp::Copy: return "copy";
                    case IROp::Add: return "add";
                    case IROp::Sub: return "sub";
                    case IROp::Mul: return "mul";
                    case IROp::Div: return "div";
                    case IROp::Cmp: return "cmp";
                    case IROp::Jmp: return "jmp";
                    case IROp::BrTrue: return "brtrue";
                    case IROp::Ret: return "ret";
                    case IROp::Call: return "call";
                    default: return "...";
                    }
                }());
                if (instr.cmpKind)
                {
                    std::print(".{}", cmpKindName(*instr.cmpKind));
                }
                for (auto id : instr.inputs)
                {
                    std::print(" v{}", id.value);
                }
                if (instr.constVal)
                {
                    std::visit([](auto v) { std::print(" {}", v); }, *instr.constVal);
                }
                if (instr.callee)
                {
                    std::print(" {}", *instr.callee);
                }
                if (instr.trueTarget)
                {
                    std::print(" -> blk_{}", instr.trueTarget->value);
                }
                if (instr.falseTarget)
                {
                    std::print(" else blk_{}", instr.falseTarget->value);
                }
                std::println();
            }
        }
    }
}

void Program::showAssembly() const
{
    if (!m_irModule)
    {
        std::println("[ASM] Not built — call build() first.");
        return;
    }
    std::println("=== Pseudo-Assembly: {} ===", m_name);
    std::print("{}", emitPseudoAsm(*m_irModule));
}

void Program::showControlFlow() const
{
    if (!m_irModule)
    {
        std::println("[CFG] Not built — call build() first.");
        return;
    }
    std::println("=== Control-Flow Graph (DOT): {} ===", m_name);
    std::print("{}", toDot(*m_irModule));
}

void Program::simulate(std::string_view functionName)
{
    if (!m_irModule)
    {
        std::println("[SIM] Not built — call build() first.");
        return;
    }

    std::println("=== Simulation: {} in '{}' ===", m_name, functionName);
    Simulator sim(*m_irModule);

    auto result = sim.run(functionName);
    if (!result)
    {
        std::println("  Error: {}", simErrorName(result.error()));
        return;
    }

    sim.printTrace(std::cout);
    if (*result)
    {
        std::print("  Return value: ");
        std::visit([](auto v) { std::print("{}", v); }, **result);
        std::println();
    }
    else
    {
        std::println("  Return value: void");
    }
}

void Program::explain(std::string_view functionName) const
{
    if (!m_irModule)
    {
        std::println("[EXPLAIN] Not built — call build() first.");
        return;
    }

    for (const auto& fn : m_irModule->functions)
    {
        if (fn.name == functionName)
        {
            std::print("{}", asmstudio::explain(fn));
            return;
        }
    }

    std::println("[EXPLAIN] Function '{}' not found.", functionName);
}

void Program::visualize(std::string_view outputPath) const
{
    if (!m_irModule)
    {
        std::println("[VIZ] Not built — call build() first.");
        return;
    }

    std::string dot = toDot(*m_irModule);
    std::ofstream file{ std::string(outputPath) };
    if (!file)
    {
        std::println("[VIZ] Cannot open '{}' for writing.", outputPath);
        return;
    }

    file << dot;
    std::println("[VIZ] DOT written to '{}'.", outputPath);
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
