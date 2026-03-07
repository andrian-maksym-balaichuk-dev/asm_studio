#include <asmstudio/asmstudio.hpp>

#include <iostream>
#include <print>

int main()
{
    std::println("=== AsmStudio v0.1.0 — Milestone 1 ===\n");

    asmstudio::Program studio("demo");

    auto& main_fn = studio.createFunction("main");
    main_fn.createInt("counter", 0);
    main_fn.createInt("total", 0);

    auto& helper = studio.createFunction("helper");
    helper.createInt64("accumulator", 0);

    studio.build();

    std::println("Program  : {}", studio.name());
    std::println("Functions: {}", studio.functions().size());

    for(const auto& fn : studio.functions())
    {
        std::println("  fn '{}' — {} variable(s)", fn->name(), fn->variables().size());
        for(const auto& v : fn->variables())
        {
            std::println("    var '{}' init={}", v->name(), v->initialValue());
        }
    }

    std::println("");
    if(!studio.diagnostics().empty())
    {
        std::println("Diagnostics:");
        studio.diagnostics().print(std::cout);
    }
    else
    {
        std::println("No diagnostics.");
    }

    std::println("\n--- Studio stubs ---");
    studio.showIR();
    studio.showAssembly();
    studio.showControlFlow();

    return 0;
}
