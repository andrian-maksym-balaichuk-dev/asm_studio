#include <asmstudio/asmstudio.hpp>

#include <iostream>

#ifdef ASM_CXX20
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
#endif

namespace
{
void printSeparator(std::string_view title)
{
    asmstudio::compat::println("\n{:=<60}", "");
    asmstudio::compat::println("  {}", title);
    asmstudio::compat::println("{:=<60}", "");
}
} // namespace


int main()
{
    asmstudio::compat::println("=== AsmStudio v0.2.0 — Full Feature Demo ===\n");

#ifdef ASM_CXX20
    printSeparator("Compile-Time Assembly Parser");
    asmstudio::compat::println("Parsed at compile time (consteval):");
    asmstudio::compat::println("  instructions : {}", kSumLoop.instructionCount());
    asmstudio::compat::println("  labels       : {}", kSumLoop.labelCount());
    asmstudio::compat::println("  ok           : {}", kSumLoop.ok());
#else
    printSeparator("Runtime Assembly Parser");
    const auto parsedSumLoopProgram{ asmstudio::asm_parse_runtime(R"(
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
    )") };
    asmstudio::compat::println("Parsed at runtime:");
    asmstudio::compat::println("  instructions : {}", parsedSumLoopProgram.instructionCount());
    asmstudio::compat::println("  labels       : {}", parsedSumLoopProgram.labelCount());
    asmstudio::compat::println("  ok           : {}", parsedSumLoopProgram.ok());
#endif

    printSeparator("OOP DSL Program Builder");

    asmstudio::Program studio{ "demo" };

    // --- Function: sum(0..9) via while loop ---
    auto& sumFunction{ studio.createFunction("sum") };
    auto& index{ sumFunction.createInt("i", 0) };
    auto& total{ sumFunction.createInt("total", 0) };

    sumFunction.assign(index, asmstudio::expr(int64_t(0)));
    sumFunction.assign(total, asmstudio::expr(int64_t(0)));

    sumFunction.whileLoop(index < int64_t(10), [&] {
        sumFunction.assign(total, asmstudio::add(asmstudio::expr(total), asmstudio::expr(index)));
        sumFunction.assign(index, asmstudio::add(asmstudio::expr(index), asmstudio::expr(int64_t(1))));
    });
    sumFunction.ret(asmstudio::expr(total));

    // --- Function: abs(x) using if/else ---
    auto& absoluteValueFunction{ studio.createFunction("abs_val") };
    auto& inputValue{ absoluteValueFunction.createInt("x", 0) };
    absoluteValueFunction.assign(inputValue, asmstudio::expr(int64_t(-5)));
    absoluteValueFunction.ifElse(
        inputValue < int64_t(0),
        [&] { absoluteValueFunction.assign(inputValue, asmstudio::neg(asmstudio::expr(inputValue))); },
        [&] { /* x already positive */ });
    absoluteValueFunction.ret(asmstudio::expr(inputValue));

    // --- Function: factorial (loop) ---
    auto& factorialFunction{ studio.createFunction("factorial") };
    auto& currentFactor{ factorialFunction.createInt("n", 0) };
    auto& factorialResult{ factorialFunction.createInt("result", 0) };
    factorialFunction.assign(currentFactor, asmstudio::expr(int64_t(6)));
    factorialFunction.assign(factorialResult, asmstudio::expr(int64_t(1)));
    factorialFunction.whileLoop(currentFactor > int64_t(1), [&] {
        factorialFunction.assign(factorialResult, asmstudio::mul(asmstudio::expr(factorialResult), asmstudio::expr(currentFactor)));
        factorialFunction.assign(currentFactor, asmstudio::sub(asmstudio::expr(currentFactor), asmstudio::expr(int64_t(1))));
    });
    factorialFunction.ret(asmstudio::expr(factorialResult));

    asmstudio::compat::println("Functions defined: {}", studio.functions().size());
    for (const auto& function : studio.functions())
    {
        asmstudio::compat::println(
            "  '{}' — {} variable(s), {} statement(s)", function->name(), function->variables().size(),
            function->statements().size());
    }

    printSeparator("IR Lowering");
    studio.build();

    if (studio.diagnostics().hasErrors())
    {
        asmstudio::compat::println("Build errors:");
        studio.diagnostics().print(std::cout);
        return 1;
    }
    asmstudio::compat::println("Build OK. IR module: '{}'", studio.irModule().name);
    asmstudio::compat::println("IR functions: {}", studio.irModule().functions.size());
    for (const auto& function : studio.irModule().functions)
    {
        asmstudio::compat::println(
            "  '{}' — {} blocks, {} values", function.name, function.blocks.size(), function.values.size());
    }

    printSeparator("Pseudo-Assembly Output");
    studio.showAssembly();

    printSeparator("Optimizer");
    studio.optimize(asmstudio::OptimizationLevel::Aggressive);
    asmstudio::compat::println("Optimization complete (aggressive level).");
    for (const auto& function : studio.irModule().functions)
    {
        asmstudio::compat::println(
            "  '{}' — {} blocks, {} values after optimization", function.name, function.blocks.size(), function.values.size());
    }

    printSeparator("Simulator");
    asmstudio::compat::println("Running 'sum':");
    studio.simulate("sum");

    asmstudio::compat::println("\nRunning 'abs_val':");
    studio.simulate("abs_val");

    asmstudio::compat::println("\nRunning 'factorial':");
    studio.simulate("factorial");

    printSeparator("Explain Mode");
    studio.explain("sum");
    studio.explain("abs_val");
    studio.explain("factorial");

    printSeparator("Control Flow Graph (DOT)");
    studio.showControlFlow();

    asmstudio::compat::println("\n=== Demo complete ===");
    return 0;
}
