#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/backend/Arm64Emitter.hpp>
#include <asmstudio/backend/PseudoEmitter.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/explain/Explain.hpp>
#include <asmstudio/lowering/Lowering.hpp>
#include <asmstudio/visualization/CfgDot.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

namespace
{
IRModule buildModule(std::string_view moduleName, auto&& setup)
{
    auto function = std::make_unique<Function>("main");
    setup(*function);

    std::vector<std::unique_ptr<Function>> functions{};
    functions.push_back(std::move(function));

    DiagnosticBag diagnostics{};
    return lower(moduleName, functions, diagnostics);
}

ValueId appendValue(IRFunction& function, DataType dataType, std::optional<IRConstant> constant = std::nullopt)
{
    const ValueId valueId{ static_cast<std::uint32_t>(function.values.size()) };
    function.values.push_back(IRValue{ dataType, std::move(constant) });
    return valueId;
}

IRFunction makeManualToolingFunction()
{
    IRFunction function{};
    function.name = "manual";

    const ValueId boolValue{ appendValue(function, DataType::Bool) };
    const ValueId intValue{ appendValue(function, DataType::Int64) };
    const ValueId uintValue{ appendValue(function, DataType::UInt64) };
    const ValueId floatValue{ appendValue(function, DataType::Float64) };
    const ValueId cmpValue{ appendValue(function, DataType::Bool) };
    const ValueId callValue{ appendValue(function, DataType::Int64) };
    const ValueId copyValue{ appendValue(function, DataType::Int64) };
    const ValueId mathValue{ appendValue(function, DataType::Int64) };

    IRBlock entryBlock{ "entry \"quoted\"", BlockId{ 0 }, {} };
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, boolValue, {}, {}, {}, {}, IRConstant{ true } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, intValue, {}, {}, {}, {}, IRConstant{ int64_t(7) } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, uintValue, {}, {}, {}, {}, IRConstant{ uint64_t(9) } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, floatValue, {}, {}, {}, {}, IRConstant{ 1.5 } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Copy, { intValue }, copyValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Add, { intValue, copyValue }, mathValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Cmp, { intValue, copyValue }, cmpValue, {}, {}, {}, CmpKind::Eq, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Call, { intValue }, callValue, {}, {}, std::string{ "helper" }, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Load, { intValue }, copyValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Store, { intValue, copyValue }, {}, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::BrTrue, { boolValue }, {}, BlockId{ 1 }, BlockId{ 2 }, {}, {}, {} });

    IRBlock thenBlock{ "then", BlockId{ 1 }, {} };
    thenBlock.instrs.push_back(IRInstr{ IROp::Jmp, {}, {}, BlockId{ 0 }, {}, {}, {}, {} });

    IRBlock elseBlock{ "else", BlockId{ 2 }, {} };
    elseBlock.instrs.push_back(IRInstr{ IROp::Ret, { callValue }, {}, {}, {}, {}, {}, {} });

    function.blocks.push_back(std::move(entryBlock));
    function.blocks.push_back(std::move(thenBlock));
    function.blocks.push_back(std::move(elseBlock));
    return function;
}
} // namespace

// ---------------------------------------------------------------------------
// Tool output smoke tests
// ---------------------------------------------------------------------------

TEST(Tooling, PseudoEmitterPrintsInstructions)
{
    // Given
    const IRModule module = buildModule("pseudo", [](Function& function) {
        Variable& value = function.createInt("value", 0);
        function.assign(value, expr(int64_t(42)));
        function.ret(expr(value));
    });

    // When
    const std::string assembly = emitPseudoAsm(module);

    // Then
    EXPECT_NE(assembly.find("main:"), std::string::npos);
    EXPECT_NE(assembly.find("const"), std::string::npos);
    EXPECT_NE(assembly.find("ret"), std::string::npos);
}

TEST(Tooling, ExplainDescribesFunctionAndModule)
{
    const IRModule module = buildModule("explained", [](Function& function) {
        Variable& value = function.createInt("value", 0);
        function.assign(value, expr(int64_t(7)));
        function.ret(expr(value));
    });

    const std::string functionDescription = explain(module.functions.front());
    EXPECT_NE(functionDescription.find("Function 'main'"), std::string::npos);
    EXPECT_NE(functionDescription.find("returns"), std::string::npos);

    const std::string moduleDescription = explain(module);
    EXPECT_NE(moduleDescription.find("Module 'explained'"), std::string::npos);
}

TEST(Tooling, CfgDotIncludesBlocksAndEdges)
{
    const IRModule module = buildModule("cfg", [](Function& function) {
        Variable& index = function.createInt("i", 0);
        function.whileLoop(index < int64_t(2), [&] { function.assign(index, add(expr(index), expr(int64_t(1)))); });
        function.ret();
    });

    const std::string functionDot = toDot(module.functions.front());
    EXPECT_NE(functionDot.find("digraph main"), std::string::npos);
    EXPECT_NE(functionDot.find("brtrue"), std::string::npos);

    const std::string moduleDot = toDot(module);
    EXPECT_NE(moduleDot.find("digraph cfg"), std::string::npos);
}

TEST(Tooling, Arm64EmitterGeneratesAssemblyText)
{
    const IRModule module = buildModule("arm64", [](Function& function) {
        Variable& leftValue = function.createInt("lhs", 0);
        Variable& rightValue = function.createInt("rhs", 0);
        function.assign(leftValue, expr(int64_t(2)));
        function.assign(rightValue, expr(int64_t(3)));
        function.ret(add(expr(leftValue), expr(rightValue)));
    });

    const std::string functionAssembly = emitArm64AsmVolatile(module.functions.front(), Arm64Platform::Linux);
    EXPECT_NE(functionAssembly.find("asm volatile"), std::string::npos);
    EXPECT_NE(functionAssembly.find("add"), std::string::npos);
    EXPECT_NE(functionAssembly.find("ret"), std::string::npos);

    const std::string moduleAssembly = emitArm64AsmVolatile(module, Arm64Platform::macOS);
    EXPECT_NE(moduleAssembly.find("ARM64 asm volatile"), std::string::npos);
    EXPECT_NE(moduleAssembly.find("macOS"), std::string::npos);
}

TEST(Tooling, PseudoEmitterFormatsMetadataAndControlFlow)
{
    const IRFunction function = makeManualToolingFunction();

    const std::string assembly = emitPseudoAsm(function);
    EXPECT_NE(assembly.find("const true"), std::string::npos);
    EXPECT_NE(assembly.find("const 9"), std::string::npos);
    EXPECT_NE(assembly.find("const 1.500000"), std::string::npos);
    EXPECT_NE(assembly.find("cmp.eq"), std::string::npos);
    EXPECT_NE(assembly.find("call helper"), std::string::npos);
    EXPECT_NE(assembly.find("else .blk_2"), std::string::npos);
    EXPECT_NE(assembly.find("; i64"), std::string::npos);
}

TEST(Tooling, ExplainCountsControlFlowAndInstructionKinds)
{
    // Given
    IRModule module{};
    module.name = "manual_mod";
    module.functions.push_back(makeManualToolingFunction());

    IRFunction linearFunction{};
    linearFunction.name = "linear";
    linearFunction.blocks.push_back(IRBlock{ "entry", BlockId{ 0 }, {} });
    module.functions.push_back(linearFunction);

    // When
    const std::string manualDescription = explain(module.functions.front());
    const std::string linearDescription = explain(module.functions.back());
    const std::string moduleDescription = explain(module);

    // Then
    EXPECT_NE(manualDescription.find("loops     : 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("branches  : 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("comparisons: 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("calls     : 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("memory loads : 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("memory stores: 1"), std::string::npos);
    EXPECT_NE(manualDescription.find("returns   : value"), std::string::npos);

    EXPECT_NE(linearDescription.find("linear (no control flow)"), std::string::npos);
    EXPECT_NE(linearDescription.find("falls through"), std::string::npos);

    EXPECT_NE(moduleDescription.find("Module 'manual_mod'"), std::string::npos);
    EXPECT_NE(moduleDescription.find("Function 'linear'"), std::string::npos);
}

TEST(Tooling, CfgDotEscapesQuotedLabelsAndEdges)
{
    const IRFunction function = makeManualToolingFunction();

    const std::string dot = toDot(function);
    EXPECT_NE(dot.find("entry \\\"quoted\\\""), std::string::npos);
    EXPECT_NE(dot.find("label=\"T\""), std::string::npos);
    EXPECT_NE(dot.find("label=\"F\""), std::string::npos);
    EXPECT_NE(dot.find("v0 = const"), std::string::npos);
}

TEST(Tooling, Arm64EmitterCoversMajorOpcodesAndPlatforms)
{
    // Given
    IRFunction function{};
    function.name = "arm64_manual";

    const ValueId boolValue{ appendValue(function, DataType::Bool) };
    const ValueId leftValue{ appendValue(function, DataType::Int64) };
    const ValueId rightValue{ appendValue(function, DataType::Int64) };
    const ValueId copyValue{ appendValue(function, DataType::Int64) };
    const ValueId addValue{ appendValue(function, DataType::Int64) };
    const ValueId subValue{ appendValue(function, DataType::Int64) };
    const ValueId andValue{ appendValue(function, DataType::Int64) };
    const ValueId orValue{ appendValue(function, DataType::Int64) };
    const ValueId xorValue{ appendValue(function, DataType::Int64) };
    const ValueId shlValue{ appendValue(function, DataType::Int64) };
    const ValueId shrValue{ appendValue(function, DataType::Int64) };
    const ValueId mulValue{ appendValue(function, DataType::Int64) };
    const ValueId divValue{ appendValue(function, DataType::Int64) };
    const ValueId modValue{ appendValue(function, DataType::Int64) };
    const ValueId negValue{ appendValue(function, DataType::Int64) };
    const ValueId notValue{ appendValue(function, DataType::Int64) };
    const ValueId cmpValue{ appendValue(function, DataType::Bool) };
    const ValueId callValue{ appendValue(function, DataType::Int64) };

    IRBlock entryBlock{ "entry", BlockId{ 0 }, {} };
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, boolValue, {}, {}, {}, {}, IRConstant{ false } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, leftValue, {}, {}, {}, {}, IRConstant{ int64_t(5) } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Const, {}, rightValue, {}, {}, {}, {}, IRConstant{ int64_t(3) } });
    entryBlock.instrs.push_back(IRInstr{ IROp::Copy, { leftValue }, copyValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Add, { leftValue, rightValue }, addValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Sub, { leftValue, rightValue }, subValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::And, { leftValue, rightValue }, andValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Or, { leftValue, rightValue }, orValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Xor, { leftValue, rightValue }, xorValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Shl, { leftValue, rightValue }, shlValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Shr, { leftValue, rightValue }, shrValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Mul, { leftValue, rightValue }, mulValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Div, { leftValue, rightValue }, divValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Mod, { leftValue, rightValue }, modValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Neg, { leftValue }, negValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Not, { rightValue }, notValue, {}, {}, {}, {}, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::Cmp, { leftValue, rightValue }, cmpValue, {}, {}, {}, CmpKind::Le, {} });
    entryBlock.instrs.push_back(IRInstr{ IROp::BrTrue, { boolValue }, {}, BlockId{ 1 }, BlockId{ 2 }, {}, {}, {} });

    IRBlock thenBlock{ "then", BlockId{ 1 }, {} };
    thenBlock.instrs.push_back(IRInstr{ IROp::Call, { leftValue }, callValue, {}, {}, std::string{ "callee" }, {}, {} });
    thenBlock.instrs.push_back(IRInstr{ IROp::Jmp, {}, {}, BlockId{ 0 }, {}, {}, {}, {} });

    IRBlock elseBlock{ "else", BlockId{ 2 }, {} };
    elseBlock.instrs.push_back(IRInstr{ IROp::Load, { leftValue }, copyValue, {}, {}, {}, {}, {} });
    elseBlock.instrs.push_back(IRInstr{ IROp::Store, { leftValue, copyValue }, {}, {}, {}, {}, {}, {} });
    elseBlock.instrs.push_back(IRInstr{ IROp::Ret, { callValue }, {}, {}, {}, {}, {}, {} });

    function.blocks.push_back(std::move(entryBlock));
    function.blocks.push_back(std::move(thenBlock));
    function.blocks.push_back(std::move(elseBlock));

    // When
    const std::string linuxAssembly = emitArm64AsmVolatile(function, Arm64Platform::Linux);
    const std::string macAssembly = emitArm64AsmVolatile(function, Arm64Platform::macOS);

    // Then
    EXPECT_NE(linuxAssembly.find("add "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("sub "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("and "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("orr "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("eor "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("lsl "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("asr "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("mul "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("sdiv "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("msub "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("neg "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("mvn "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("cmp "), std::string::npos);
    EXPECT_NE(linuxAssembly.find("cset"), std::string::npos);
    EXPECT_NE(linuxAssembly.find("cbnz"), std::string::npos);
    EXPECT_NE(linuxAssembly.find("b 0b"), std::string::npos);
    EXPECT_NE(linuxAssembly.find("b 2f"), std::string::npos);
    EXPECT_NE(linuxAssembly.find("bl callee"), std::string::npos);
    EXPECT_NE(linuxAssembly.find("\"x0\""), std::string::npos);

    EXPECT_NE(macAssembly.find("bl _callee"), std::string::npos);

    EXPECT_TRUE(
        hostArm64Platform() == Arm64Platform::Linux || hostArm64Platform() == Arm64Platform::macOS ||
        hostArm64Platform() == Arm64Platform::Windows);
}
