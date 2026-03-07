#include <asmstudio/asmstudio.hpp>

#include <iostream>
#include <print>

constexpr auto kSumLoop = asmstudio::asm_parse<R"(
    mov r0, 0
    mov r1, 0
loop:
    cmp r0, 10
    jge done
    add r1, r0
    add r0, 1
    jmp loop
done:
    ret
)">();

static_assert(kSumLoop.ok(), "Inline assembly must be valid");
static_assert(kSumLoop.instructionCount() == 8u, "Expected 8 instructions");
static_assert(kSumLoop.labelCount() == 2u, "Expected 2 labels");

namespace
{
void printSeparator(std::string_view title)
{
    std::println("\n{:=<60}", "");
    std::println("  {}", title);
    std::println("{:=<60}", "");
}
} // namespace


int main()
{
    std::println("=== AsmStudio v0.2.0 — Full Feature Demo ===\n");

    printSeparator("Compile-Time Assembly Parser");
    std::println("Parsed at compile time (consteval):");
    std::println("  instructions : {}", kSumLoop.instructionCount());
    std::println("  labels       : {}", kSumLoop.labelCount());
    std::println("  ok           : {}", kSumLoop.ok());

    printSeparator("OOP DSL Program Builder");

    asmstudio::Program studio("demo");

    // --- Function: sum(0..9) via while loop ---
    auto& sum_fn = studio.createFunction("sum");
    auto& i = sum_fn.createInt("i", 0);
    auto& total = sum_fn.createInt("total", 0);

    sum_fn.assign(i, asmstudio::expr(int64_t(0)));
    sum_fn.assign(total, asmstudio::expr(int64_t(0)));

    sum_fn.whileLoop(i < int64_t(10), [&] {
        sum_fn.assign(total, asmstudio::add(asmstudio::expr(total), asmstudio::expr(i)));
        sum_fn.assign(i, asmstudio::add(asmstudio::expr(i), asmstudio::expr(int64_t(1))));
    });
    sum_fn.ret(asmstudio::expr(total));

    // --- Function: abs(x) using if/else ---
    auto& abs_fn = studio.createFunction("abs_val");
    auto& x = abs_fn.createInt("x", 0);
    abs_fn.assign(x, asmstudio::expr(int64_t(-5)));
    abs_fn.ifElse(x < int64_t(0), [&] { abs_fn.assign(x, asmstudio::neg(asmstudio::expr(x))); }, [&] { /* x already positive */ });
    abs_fn.ret(asmstudio::expr(x));

    // --- Function: factorial (loop) ---
    auto& fact_fn = studio.createFunction("factorial");
    auto& n = fact_fn.createInt("n", 0);
    auto& result = fact_fn.createInt("result", 0);
    fact_fn.assign(n, asmstudio::expr(int64_t(6)));
    fact_fn.assign(result, asmstudio::expr(int64_t(1)));
    fact_fn.whileLoop(n > int64_t(1), [&] {
        fact_fn.assign(result, asmstudio::mul(asmstudio::expr(result), asmstudio::expr(n)));
        fact_fn.assign(n, asmstudio::sub(asmstudio::expr(n), asmstudio::expr(int64_t(1))));
    });
    fact_fn.ret(asmstudio::expr(result));

    std::println("Functions defined: {}", studio.functions().size());
    for (const auto& fn : studio.functions())
    {
        std::println("  '{}' — {} variable(s), {} statement(s)", fn->name(), fn->variables().size(), fn->statements().size());
    }

    printSeparator("IR Lowering");
    studio.build();

    if (studio.diagnostics().hasErrors())
    {
        std::println("Build errors:");
        studio.diagnostics().print(std::cout);
        return 1;
    }
    std::println("Build OK. IR module: '{}'", studio.irModule().name);
    std::println("IR functions: {}", studio.irModule().functions.size());
    for (const auto& fn : studio.irModule().functions)
    {
        std::println("  '{}' — {} blocks, {} values", fn.name, fn.blocks.size(), fn.values.size());
    }

    printSeparator("Pseudo-Assembly Output");
    studio.showAssembly();

    printSeparator("Optimizer");
    studio.optimize(asmstudio::OptimizationLevel::Aggressive);
    std::println("Optimization complete (aggressive level).");
    for (const auto& fn : studio.irModule().functions)
        std::println("  '{}' — {} blocks, {} values after optimization", fn.name, fn.blocks.size(), fn.values.size());

    printSeparator("Simulator");
    std::println("Running 'sum':");
    studio.simulate("sum");

    std::println("\nRunning 'abs_val':");
    studio.simulate("abs_val");

    std::println("\nRunning 'factorial':");
    studio.simulate("factorial");

    printSeparator("Explain Mode");
    studio.explain("sum");
    studio.explain("abs_val");
    studio.explain("factorial");

    printSeparator("Control Flow Graph (DOT)");
    studio.showControlFlow();

    std::println("\n=== Demo complete ===");
    return 0;
}
