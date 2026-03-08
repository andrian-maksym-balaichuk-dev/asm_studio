# AsmStudio

AsmStudio is a C++ library and demo application for building, lowering, inspecting, optimizing, and simulating small assembly-like programs.

It sits between a teaching tool and a compiler playground:

- You can describe programs through an object-oriented DSL.
- You can parse inline assembly-like text at compile time or runtime.
- You can lower both representations into a shared IR.
- You can inspect pseudo assembly, explain structure, emit CFG DOT output, run optimizer passes, and simulate execution.

## Why This Project Exists

I built AsmStudio to make low-level program representation easier to reason about.

Most compiler and systems examples jump too quickly from syntax to opaque internals. That is useful for experts, but it is a poor learning surface for people who want to understand how source representation becomes control flow, intermediate form, optimization, and execution behavior.

AsmStudio is meant to close that gap.

The motivation is simple:

- make program transformation visible
- keep the codebase small enough to understand end to end
- treat architecture and naming seriously instead of writing a throwaway demo
- keep the project useful both as a learning artifact and as a base for future experimentation

This repository is also a deliberate engineering exercise. It is a place to practice clean interfaces, explicit modeling, readable C++, and tool-oriented design around IR rather than around a single parser or single backend.

## Project Goals

- Provide a clear, inspectable IR pipeline.
- Support both compile-time and runtime assembly parsing.
- Keep the public API approachable for demos, tests, and experiments.
- Show how optimization, simulation, and visualization can share one intermediate model.
- Stay portable across modern C++ toolchains.

## Non-Goals

- A full production assembler
- A full optimizing compiler
- A complete CPU emulator
- A backend with ABI-complete code generation

AsmStudio is intentionally focused. It prefers clarity and extensibility over feature volume.

## What It Can Do

- Build programs through an OOP DSL with variables, expressions, loops, branches, returns, and calls
- Parse assembly-like text through `asm_parse_runtime(...)`
- Parse assembly-like text at compile time in C++20+ through `asm_parse<"...">()`
- Lower high-level program structure into IR
- Run optimization passes such as constant folding, dead code elimination, jump simplification, and unreachable block elimination
- Print pseudo assembly
- Emit ARM64 `asm volatile(...)` text
- Simulate IR execution
- Explain function structure in readable text
- Export control-flow graphs as DOT

## High-Level Architecture

The codebase is organized around a small set of responsibilities:

- `include/asmstudio/api`
  Defines the user-facing DSL for building programs and functions.
- `include/asmstudio/parser`
  Defines tokenization and parsing for the assembly-like syntax.
- `include/asmstudio/ir`
  Defines the shared intermediate representation used by the rest of the system.
- `src/lowering`
  Translates API-built programs into IR.
- `src/optimizer`
  Runs IR transformation passes.
- `src/simulator`
  Executes IR in a lightweight runtime model.
- `src/backend`
  Produces pseudo assembly and ARM64 inline-assembly text.
- `src/explain`
  Produces human-readable summaries of IR structure.
- `src/visualization`
  Produces DOT graph output.

This is the central design decision in the repository: the IR is the product boundary. Parsing, optimization, simulation, explanation, and visualization all meet there.

## Design Principles

- Single responsibility where it matters: IR objects own IR behavior, tooling layers consume it.
- Pragmatic SOLID: avoid abstraction for its own sake, but remove duplicated knowledge aggressively.
- Inspectability first: if a transformation exists, there should be a straightforward way to observe it.
- Portable modern C++: the project supports C++17, C++20, and C++23 configurations.
- Testable components: most subsystems can be exercised directly without going through the demo executable.

## Example

```cpp
#include <asmstudio/asmstudio.hpp>

int main()
{
    asmstudio::Program program{ "demo" };

    auto& sumFunction{ program.createFunction("sum") };
    auto& index{ sumFunction.createInt("i", 0) };
    auto& total{ sumFunction.createInt("total", 0) };

    sumFunction.assign(index, asmstudio::expr(int64_t(0)));
    sumFunction.assign(total, asmstudio::expr(int64_t(0)));

    sumFunction.whileLoop(index < int64_t(10), [&] {
        sumFunction.assign(total, asmstudio::add(asmstudio::expr(total), asmstudio::expr(index)));
        sumFunction.assign(index, asmstudio::add(asmstudio::expr(index), asmstudio::expr(int64_t(1))));
    });

    sumFunction.ret(asmstudio::expr(total));

    program.build();
    program.optimize(asmstudio::OptimizationLevel::Aggressive);
    program.showAssembly();
    program.simulate("sum");
}
```

## Build

Requirements:

- CMake 3.20+
- A C++ compiler with C++17 support or newer

Default build:

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

Run the demo:

```bash
./cmake-build-debug/asm_studio
```

Choose the language standard explicitly:

```bash
cmake -S . -B build-cpp20 -DCMAKE_BUILD_TYPE=Debug -DASM_CXX_STANDARD=20
cmake --build build-cpp20
```

Supported values:

- `17`
- `20`
- `23`

## Tests

The repository includes focused GoogleTest binaries and an aggregate test binary.

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

If GoogleTest is not already available, CMake fetches it during configuration.

## Demo Coverage

The main demo in [src/main.cpp](/Users/andrian/mycode/asm_studio/src/main.cpp) shows the intended workflow:

- build functions with the OOP DSL
- lower to IR
- optimize the IR
- print pseudo assembly
- simulate execution
- explain structure
- print CFG output
- parse assembly text at compile time or runtime depending on the language standard

## Intended Audience

AsmStudio is useful for:

- engineers exploring compiler pipeline fundamentals
- students learning IR and control flow
- developers who want a compact C++ codebase for backend and optimization experiments
- anyone who prefers a readable systems project over a framework-heavy compiler stack

## Current Status

The project already has a strong core pipeline and a substantial automated test suite, but it is still an evolving codebase.

Current strengths:

- clear API surface
- shared IR model
- test coverage across parser, IR, optimizer, tooling, and simulator
- multiple inspection surfaces for the same program

Areas still open for improvement:

- richer assembly grammar
- stronger backend realism
- broader optimization coverage
- deeper simulation semantics
- cleaner coverage tooling and reporting

## Roadmap Direction

Good next steps for the repository are:

- add more expression and type coverage in lowering
- expand optimizer pass sophistication
- improve module-level examples and documentation
- make backend emission more ABI-aware
- add richer diagnostics around invalid programs

## Philosophy

This project is not trying to be the biggest thing in the room.

It is trying to be the kind of repository a senior engineer can open and immediately understand:

- why it exists
- where the boundaries are
- how the pieces fit
- what is solid already
- what still needs work

That is the standard this README and this codebase are aiming for.
