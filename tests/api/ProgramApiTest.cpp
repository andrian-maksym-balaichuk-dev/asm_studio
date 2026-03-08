#include <asmstudio/api/Program.hpp>
#include <asmstudio/api/Variable.hpp>
#include <asmstudio/explain/Explain.hpp>
#include <asmstudio/optimizer/Optimizer.hpp>
#include <asmstudio/visualization/CfgDot.hpp>

#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace asmstudio;

// ---------------------------------------------------------------------------
// Program lifecycle and build orchestration
// ---------------------------------------------------------------------------

TEST(Program, NameRoundtrip)
{
    // Given / When
    Program program("MyProg");

    // Then
    EXPECT_EQ(program.name(), "MyProg");
}

TEST(Program, StartsWithNoFunctions)
{
    // Given / When
    Program program("empty");

    // Then
    EXPECT_TRUE(program.functions().empty());
}

TEST(Program, CreateFunctionReturnsRef)
{
    // Given
    Program program("prog");

    // When
    Function& function = program.createFunction("main");

    // Then
    EXPECT_EQ(function.name(), "main");
    EXPECT_EQ(program.functions().size(), 1u);
}

TEST(Program, CreateMultipleFunctions)
{
    // Given
    Program program("prog");

    // When
    program.createFunction("init");
    program.createFunction("loop");
    program.createFunction("done");

    // Then
    EXPECT_EQ(program.functions().size(), 3u);
}

TEST(Program, BuildEmptyProgramEmitsWarning)
{
    // Given
    Program program("prog");

    // When
    program.build();

    // Then
    EXPECT_FALSE(program.diagnostics().hasErrors());
    EXPECT_FALSE(program.diagnostics().empty()); // warning for no functions
}

TEST(Program, BuildWithFunctionNoDiagnostics)
{
    // Given
    Program program("prog");
    program.createFunction("main");

    // When
    program.build();

    // Then
    EXPECT_TRUE(program.diagnostics().empty());
}

TEST(Program, IrModuleThrowsBeforeBuild)
{
    // Given / When
    Program program("prog");

    // Then
    EXPECT_THROW(static_cast<void>(program.irModule()), std::runtime_error);
}

TEST(Program, HasIrAfterBuild)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();

    // When
    program.build();

    // Then
    EXPECT_TRUE(program.hasIR());
}

TEST(Program, OptimizeBuildsPipelineAndKeepsIrAccessible)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    auto& value = function.createInt("value", 0);
    function.assign(value, expr(int64_t(4)));
    function.ret(expr(value));
    program.build();

    // When
    program.optimize(OptimizationLevel::Aggressive);

    // Then
    EXPECT_FALSE(program.irModule().functions.empty());
}

TEST(Program, VisualizeWritesDotFile)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    const std::string outputPath{ "/tmp/asmstudio_program_visualize.dot" };

    // When
    program.visualize(outputPath);

    // Then
    std::ifstream inputStream(outputPath);
    ASSERT_TRUE(inputStream.good());
    const std::string dotContents((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
    EXPECT_NE(dotContents.find("digraph"), std::string::npos);
    EXPECT_NE(dotContents.find("main"), std::string::npos);
}

TEST(Program, SimulateAndExplainDoNotThrowAfterBuild)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret(expr(int64_t(7)));
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.simulate("main");
    const std::string simulationOutput{ testing::internal::GetCapturedStdout() };

    testing::internal::CaptureStdout();
    program.explain("main");
    const std::string explainOutput{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(simulationOutput.find("Return value"), std::string::npos);
    EXPECT_NE(explainOutput.find("Function 'main'"), std::string::npos);
}

TEST(Program, ShowAssemblyAndShowControlFlowPrintOutput)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.showAssembly();
    const std::string assemblyOutput{ testing::internal::GetCapturedStdout() };

    testing::internal::CaptureStdout();
    program.showControlFlow();
    const std::string dotOutput{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(assemblyOutput.find("Pseudo-Assembly"), std::string::npos);
    EXPECT_NE(dotOutput.find("digraph"), std::string::npos);
}

TEST(Program, NotBuiltOperationsPrintHelpfulMessages)
{
    // Given
    Program program("prog");

    // When
    testing::internal::CaptureStdout();
    program.optimize();
    program.showIR();
    program.showAssembly();
    program.showControlFlow();
    program.simulate();
    program.explain("main");
    program.visualize("/tmp/unused.dot");
    const std::string output{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(output.find("[OPT] Call build() before optimize()."), std::string::npos);
    EXPECT_NE(output.find("[IR] Not built"), std::string::npos);
    EXPECT_NE(output.find("[ASM] Not built"), std::string::npos);
    EXPECT_NE(output.find("[CFG] Not built"), std::string::npos);
    EXPECT_NE(output.find("[SIM] Not built"), std::string::npos);
    EXPECT_NE(output.find("[EXPLAIN] Not built"), std::string::npos);
    EXPECT_NE(output.find("[VIZ] Not built"), std::string::npos);
}

TEST(Program, ShowIrPrintsDetailedInstructionFields)
{
    // Given
    Program program("prog");
    auto& helperFunction = program.createFunction("helper");
    helperFunction.ret(expr(int64_t(2)));

    auto& mainFunction = program.createFunction("main");
    auto& value = mainFunction.createInt("value", 0);
    mainFunction.assign(value, expr(int64_t(4)));
    mainFunction.ifElse(
        value > int64_t(0), [&] { mainFunction.call("helper", { expr(int64_t(1)) }); },
        [&] { mainFunction.assign(value, expr(int64_t(0))); });
    mainFunction.ret(expr(value));
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.showIR();
    const std::string output{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(output.find("=== IR: prog ==="), std::string::npos);
    EXPECT_NE(output.find("fn main:"), std::string::npos);
    EXPECT_NE(output.find("const 4"), std::string::npos);
    EXPECT_NE(output.find("cmp.gt"), std::string::npos);
    EXPECT_NE(output.find("-> blk_"), std::string::npos);
    EXPECT_NE(output.find("else blk_"), std::string::npos);
}

TEST(Program, SimulateVoidReturnAndUnknownFunctionPrintOutput)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.simulate("main");
    const std::string voidOutput{ testing::internal::GetCapturedStdout() };

    testing::internal::CaptureStdout();
    program.simulate("missing");
    const std::string errorOutput{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(voidOutput.find("Return value: void"), std::string::npos);
    EXPECT_NE(errorOutput.find("Error: unknown function"), std::string::npos);
}

TEST(Program, ExplainMissingFunctionPrintsMessage)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.explain("missing");
    const std::string output{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(output.find("Function 'missing' not found"), std::string::npos);
}

TEST(Program, VisualizeInvalidPathPrintsError)
{
    // Given
    Program program("prog");
    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    // When
    testing::internal::CaptureStdout();
    program.visualize("/tmp/asmstudio_missing_dir/output.dot");
    const std::string output{ testing::internal::GetCapturedStdout() };

    // Then
    EXPECT_NE(output.find("Cannot open"), std::string::npos);
}

TEST(Program, OutputCanBeRedirectedToCustomStream)
{
    // Given
    std::ostringstream outputStream{};
    Program program("prog", outputStream);

    auto& function = program.createFunction("main");
    function.ret();
    program.build();

    // When
    program.showAssembly();

    // Then
    EXPECT_NE(outputStream.str().find("Pseudo-Assembly"), std::string::npos);

    // Given
    std::ostringstream replacementStream{};

    // When
    program.setOutput(replacementStream);
    program.showControlFlow();

    // Then
    EXPECT_EQ(&program.output(), &replacementStream);
    EXPECT_NE(replacementStream.str().find("Control-Flow Graph"), std::string::npos);
}

TEST(Function, NameRoundtrip)
{
    Function fn("compute");
    EXPECT_EQ(fn.name(), "compute");
}

TEST(Function, CreateIntVariable)
{
    Function fn("fn");
    Variable& v = fn.createInt("counter", 42);
    EXPECT_EQ(v.name(), "counter");
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), 42);
    EXPECT_EQ(v.type(), DataType::Int32);
    EXPECT_EQ(fn.variables().size(), 1u);
}

TEST(Function, CreateInt64Variable)
{
    Function fn("fn");
    Variable& v = fn.createInt64("big", 1'000'000'000LL);
    EXPECT_EQ(v.type(), DataType::Int64);
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), 1'000'000'000LL);
}

TEST(Function, CreateMultipleVariables)
{
    Function fn("fn");
    fn.createInt("a", 0);
    fn.createInt("b", 1);
    fn.createInt("c", 2);
    EXPECT_EQ(fn.variables().size(), 3u);
}

TEST(Function, CreateAllVariableKinds)
{
    Function function("fn");

    EXPECT_EQ(function.createInt8("i8", 1).type(), DataType::Int8);
    EXPECT_EQ(function.createInt16("i16", 2).type(), DataType::Int16);
    EXPECT_EQ(function.createUInt32("u32", 3).type(), DataType::UInt32);
    EXPECT_EQ(function.createUInt64("u64", 4).type(), DataType::UInt64);
    EXPECT_EQ(function.createFloat("f32", 1.5f).type(), DataType::Float32);
    EXPECT_EQ(function.createDouble("f64", 2.5).type(), DataType::Float64);
    EXPECT_EQ(function.createBool("flag", true).type(), DataType::Bool);
    EXPECT_EQ(function.createPtr("ptr", 0x10u).type(), DataType::Ptr);
    EXPECT_EQ(function.variables().size(), 8u);
}

TEST(Variable, BasicProperties)
{
    Variable v("x", DataType::Int32, InitValue{ int64_t(-7) });
    EXPECT_EQ(v.name(), "x");
    EXPECT_EQ(v.type(), DataType::Int32);
    EXPECT_EQ(std::get<int64_t>(v.initVariant()), -7);
}

TEST(Program, DiagnosticsConstAccessorMatchesMutableAccessor)
{
    Program program("prog");
    program.build();

    const Program& constProgram = program;
    EXPECT_FALSE(constProgram.diagnostics().empty());
    EXPECT_EQ(constProgram.diagnostics().all().size(), program.diagnostics().all().size());
}
