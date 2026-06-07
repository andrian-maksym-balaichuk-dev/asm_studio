#include <asmstudio/dsl/Expr.hpp>
#include <asmstudio/dsl/Function.hpp>
#include <asmstudio/support/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/ir/Lowering.hpp>
#include <asmstudio/sim/Simulator.hpp>

#include <gtest/gtest.h>
#include <sstream>
#include <utility>

using namespace asmstudio;

// Helper: lower a single Function to an IRModule.
static IRModule lowerFn(std::string_view name, auto&& setup)
{
    auto fn = std::make_unique<Function>(std::string(name));
    setup(*fn);
    std::vector<std::unique_ptr<Function>> fns;
    fns.push_back(std::move(fn));
    DiagnosticBag diags;
    return lower(name, fns, diags);
}

static ValueId pushValue(IRFunction& function, DataType type, std::optional<IRConstant> constant = std::nullopt)
{
    const ValueId valueId{ static_cast<std::uint32_t>(function.values.size()) };
    function.values.push_back(IRValue{ type, std::move(constant) });
    return valueId;
}

static IRModule makeManualModule()
{
    IRModule module{};
    module.name = "manual";

    IRFunction callee{};
    callee.name = "callee";
    const ValueId calleeReturnValue{ pushValue(callee, DataType::Int64, IRConstant{ int64_t(11) }) };
    callee.blocks.push_back(
        IRBlock{ "entry", BlockId{ 0 }, { IRInstr{ IROp::Ret, { calleeReturnValue }, {}, {}, {}, {}, {}, {} } } });

    IRFunction mainFunction{};
    mainFunction.name = "main";
    const ValueId leftValue{ pushValue(mainFunction, DataType::Int64, IRConstant{ int64_t(8) }) };
    const ValueId rightValue{ pushValue(mainFunction, DataType::Int64, IRConstant{ int64_t(2) }) };
    const ValueId copiedValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId subValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId mulValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId divValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId modValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId andValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId orValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId xorValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId shlValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId shrValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId negValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId notValue{ pushValue(mainFunction, DataType::Int64) };
    const ValueId cmpValue{ pushValue(mainFunction, DataType::Bool) };

    IRBlock entryBlock{ "entry", BlockId{ 0 }, {} };
    entryBlock.instrs.push_back(IRInstr{ IROp::Copy, { leftValue }, copiedValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Sub, { copiedValue, rightValue }, subValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Mul, { subValue, rightValue }, mulValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Div, { mulValue, rightValue }, divValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Mod, { divValue, rightValue }, modValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::And, { modValue, rightValue }, andValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Or, { andValue, rightValue }, orValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Xor, { orValue, rightValue }, xorValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Shl, { xorValue, rightValue }, shlValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Shr, { shlValue, rightValue }, shrValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Neg, { shrValue }, negValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Not, { negValue }, notValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Cmp, { notValue, rightValue }, cmpValue, {}, {}, {}, CmpKind::Ne, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::BrTrue, { cmpValue }, {}, BlockId{ 1 }, BlockId{ 2 }, {}, {}, {} });

    IRBlock thenBlock{ "then", BlockId{ 1 }, {} };
    thenBlock.instrs.push_back(IRInstr{ IROp::Call, {}, {}, {}, {}, std::string{ "callee" }, {}, {} });
    thenBlock.instrs.push_back(IRInstr{ IROp::Jmp, {}, {}, BlockId{ 2 }, {}, {}, {}, {} });

    IRBlock exitBlock{ "exit", BlockId{ 2 }, {} };
    exitBlock.instrs.push_back(IRInstr{ IROp::Ret, { notValue }, {}, {}, {}, {}, {}, {} });

    mainFunction.blocks.push_back(std::move(entryBlock));
    mainFunction.blocks.push_back(std::move(thenBlock));
    mainFunction.blocks.push_back(std::move(exitBlock));

    module.functions.push_back(std::move(mainFunction));
    module.functions.push_back(std::move(callee));
    return module;
}

// ---------------------------------------------------------------------------
// Core simulator execution paths
// ---------------------------------------------------------------------------

TEST(Simulator, UnknownFunctionError)
{
    // Given
    auto module = lowerFn("m", [](Function& function) { function.ret(); });
    Simulator simulator(module);

    // When
    auto result = simulator.run("nonexistent");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::UnknownFunction);
}

TEST(Simulator, ReturnVoid)
{
    // Given
    auto module = lowerFn("m", [](Function& function) { function.ret(); });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST(Simulator, ReturnConstant)
{
    // Given
    auto module = lowerFn("m", [](Function& function) { function.ret(expr(int64_t(42))); });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 42);
}

TEST(Simulator, AssignAndReturn)
{
    // Given
    auto mod = lowerFn("m", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(7)));
        fn.ret(expr(x));
    });
    Simulator sim(mod);

    // When
    auto result = sim.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 7);
}

TEST(Simulator, SimpleAddition)
{
    // Given
    auto module = lowerFn("m", [](Function& function) {
        Variable& leftValue = function.createInt("a", 0);
        Variable& rightValue = function.createInt("b", 0);
        function.assign(leftValue, expr(int64_t(3)));
        function.assign(rightValue, expr(int64_t(4)));
        function.ret(add(expr(leftValue), expr(rightValue)));
    });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 7);
}

TEST(Simulator, WhileLoopSum)
{
    // Given: a loop that sums 0+1+2+...+9 = 45
    auto mod = lowerFn("sum", [](Function& fn) {
        Variable& i = fn.createInt("i", 0);
        Variable& total = fn.createInt("total", 0);
        fn.assign(i, expr(int64_t(0)));
        fn.assign(total, expr(int64_t(0)));
        fn.whileLoop(i < int64_t(10), [&] {
            fn.assign(total, add(expr(total), expr(i)));
            fn.assign(i, add(expr(i), expr(int64_t(1))));
        });
        fn.ret(expr(total));
    });
    Simulator sim(mod);

    // When
    auto result = sim.run("sum");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 45);
}

TEST(Simulator, IfBranchTaken)
{
    // Given
    auto module = lowerFn("m", [](Function& function) {
        Variable& value = function.createInt("x", 0);
        function.assign(value, expr(int64_t(1)));
        function.ifStmt(value > int64_t(0), [&] { function.assign(value, expr(int64_t(100))); });
        function.ret(expr(value));
    });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 100);
}

TEST(Simulator, IfBranchNotTaken)
{
    // Given
    auto module = lowerFn("m", [](Function& function) {
        Variable& value = function.createInt("x", 0);
        function.assign(value, expr(int64_t(-1)));
        function.ifStmt(value > int64_t(0), [&] { function.assign(value, expr(int64_t(100))); });
        function.ret(expr(value));
    });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), -1);
}

TEST(Simulator, IfElseFalseArm)
{
    // Given
    auto module = lowerFn("m", [](Function& function) {
        Variable& value = function.createInt("x", 0);
        function.assign(value, expr(int64_t(0)));
        function.ifElse(
            value > int64_t(0), [&] { function.assign(value, expr(int64_t(1))); },
            [&] { function.assign(value, expr(int64_t(-1))); });
        function.ret(expr(value));
    });
    Simulator simulator(module);

    // When
    auto result = simulator.run("m");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), -1);
}

TEST(Simulator, RewindRestoresState)
{
    // Given
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(expr(int64_t(1))); });
    Simulator sim(mod);
    std::ignore = sim.run("m");

    // When
    EXPECT_TRUE(sim.done()); // run() finished
    sim.rewind(1);

    // Then: rewind restores a prior snapshot, so execution is no longer complete.
    EXPECT_FALSE(sim.done()); // rewound to mid-execution state
}

TEST(Simulator, Reset)
{
    // Given
    auto mod = lowerFn("m", [](Function& fn) { fn.ret(expr(int64_t(5))); });
    Simulator sim(mod);
    std::ignore = sim.run("m");

    // When
    sim.reset();
    auto result = sim.run("m");

    // Then: after reset, the same program should run cleanly again.
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<int64_t>(**result), 5);
}

TEST(Simulator, DivisionByZeroError)
{
    // Given
    auto module = lowerFn("m", [](Function& function) {
        Variable& numerator = function.createInt("a", 0);
        Variable& denominator = function.createInt("b", 0);
        function.assign(numerator, expr(int64_t(10)));
        function.assign(denominator, expr(int64_t(0)));
        function.ret(div(expr(numerator), expr(denominator)));
    });

    Simulator simulator(module);

    // When
    const auto result = simulator.run("m");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::DivisionByZero);
}

TEST(Simulator, StepReturnsFalseWhenDone)
{
    auto module = lowerFn("m", [](Function& function) { function.ret(); });
    Simulator simulator(module);
    const auto result = simulator.step();
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(*result);
}

TEST(Simulator, MaxStepsExceeded)
{
    // Given
    auto module = lowerFn("spin", [](Function& function) {
        Variable& counter = function.createInt("counter", 0);
        function.assign(counter, expr(int64_t(0)));
        function.whileLoop(counter >= int64_t(0), [&] { function.assign(counter, expr(counter)); });
        function.ret();
    });

    Simulator simulator(module, 4);

    // When
    const auto result = simulator.run("spin");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::MaxStepsExceeded);
}

TEST(Simulator, ManualIrCoversArithmeticCallsAndTrace)
{
    // Given
    const IRModule module = makeManualModule();
    Simulator simulator(module);

    // When
    const auto result = simulator.run("main");

    // Then
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(std::get<int64_t>(**result), -1);

    std::ostringstream traceStream{};
    simulator.printTrace(traceStream);
    const std::string trace = traceStream.str();
    EXPECT_NE(trace.find("fn=main"), std::string::npos);
    EXPECT_NE(trace.find("fn=callee"), std::string::npos);
    EXPECT_NE(trace.find("return value: -1"), std::string::npos);
}

TEST(Simulator, TypeMismatchWhenReturningUnsetValue)
{
    // Given
    IRModule module{};
    module.name = "mismatch";

    IRFunction function{};
    function.name = "main";
    const ValueId missingValue{ pushValue(function, DataType::Int64) };
    const ValueId outputValue{ pushValue(function, DataType::Int64) };
    function.blocks.push_back(
        IRBlock{ "entry",
                 BlockId{ 0 },
                 {
                     IRInstr{ IROp::Copy, { missingValue }, outputValue, {}, {}, {}, {}, {} },
                     IRInstr{ IROp::Ret, { outputValue }, {}, {}, {}, {}, {}, {} },
                 } });
    module.functions.push_back(std::move(function));

    Simulator simulator(module);

    // When
    const auto result = simulator.run("main");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::TypeMismatch);
}

TEST(Simulator, UnreachableInstructionOnMissingTargetBlock)
{
    // Given
    IRModule module{};
    module.name = "unreachable";

    IRFunction function{};
    function.name = "main";
    function.blocks.push_back(IRBlock{ "entry", BlockId{ 0 }, { IRInstr{ IROp::Jmp, {}, {}, BlockId{ 99 }, {}, {}, {}, {} } } });
    module.functions.push_back(std::move(function));

    Simulator simulator(module);

    // When
    const auto result = simulator.run("main");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::UnreachableInstruction);
}

TEST(Simulator, StackOverflowWhenDepthLimitIsTooSmall)
{
    // Given
    IRModule module{};
    module.name = "stack";

    IRFunction function{};
    function.name = "main";
    function.blocks.push_back(IRBlock{ "entry", BlockId{ 0 }, { IRInstr{ IROp::Ret, {}, {}, {}, {}, {}, {}, {} } } });
    module.functions.push_back(std::move(function));

    Simulator simulator(module, Simulator::kDefaultMaxSteps, 0);

    // When
    const auto result = simulator.run("main");

    // Then
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SimError::StackOverflow);
}

TEST(Simulator, StepAdvancesAfterLargeRewind)
{
    // Given
    const IRModule module = makeManualModule();
    Simulator simulator(module);

    ASSERT_TRUE(simulator.run("main").has_value());

    // When
    simulator.rewind(1);
    const auto stepResult = simulator.step();

    // Then
    ASSERT_TRUE(stepResult.has_value());
    EXPECT_FALSE(simulator.done());
}

TEST(Simulator, ErrorNamesRemainStable)
{
    EXPECT_EQ(simErrorName(SimError::DivisionByZero), "division by zero");
    EXPECT_EQ(simErrorName(SimError::UnknownFunction), "unknown function");
    EXPECT_EQ(simErrorName(SimError::StackOverflow), "stack overflow");
    EXPECT_EQ(simErrorName(SimError::TypeMismatch), "type mismatch");
    EXPECT_EQ(simErrorName(SimError::UnreachableInstruction), "unreachable instruction");
    EXPECT_EQ(simErrorName(SimError::MaxStepsExceeded), "max steps exceeded");
}
