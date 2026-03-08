#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/api/Program.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/lowering/Lowering.hpp>

#include <gtest/gtest.h>

#include <unordered_map>

using namespace asmstudio;

// Helper: build an IRModule from a single Function built via lambda.
static IRModule buildModule(std::string_view name, auto&& setup)
{
    auto fn = std::make_unique<Function>(std::string(name));
    setup(*fn);
    std::vector<std::unique_ptr<Function>> fns;
    fns.push_back(std::move(fn));
    DiagnosticBag diags;
    return lower(name, fns, diags);
}

TEST(IR, EmptyFunctionHasEntryBlock)
{
    // Given / When
    auto mod = buildModule("empty", [](Function&) {});

    // Then
    ASSERT_EQ(mod.functions.size(), 1u);
    EXPECT_FALSE(mod.functions[0].blocks.empty());
}

TEST(IR, ConstAssignCreatesConstInstr)
{
    // Given / When
    auto mod = buildModule("const_assign", [](Function& fn) {
        Variable& x = fn.createInt("x", 0);
        fn.assign(x, expr(int64_t(42)));
    });
    const IRFunction& irFn = mod.functions[0];
    bool foundConst = false;
    for (const auto& blk : irFn.blocks)
        for (const auto& ins : blk.instrs)
            if (ins.op == IROp::Const && ins.constVal)
            {
                if (std::holds_alternative<int64_t>(*ins.constVal) && std::get<int64_t>(*ins.constVal) == 42)
                    foundConst = true;
            }
    // Then
    EXPECT_TRUE(foundConst);
}

TEST(IR, ReturnEmitsRetInstr)
{
    auto mod = buildModule("return_val", [](Function& fn) { fn.ret(expr(int64_t(99))); });
    const IRFunction& irFn = mod.functions[0];
    bool foundRet = false;
    for (const auto& blk : irFn.blocks)
        for (const auto& ins : blk.instrs)
            if (ins.op == IROp::Ret)
                foundRet = true;
    EXPECT_TRUE(foundRet);
}

TEST(IR, WhileLoopCreatesMultipleBlocks)
{
    auto mod = buildModule("while_fn", [](Function& fn) {
        Variable& i = fn.createInt("i", 0);
        fn.whileLoop(i < int64_t(10), [&] { fn.assign(i, add(expr(i), expr(int64_t(1)))); });
        fn.ret();
    });
    const IRFunction& irFn = mod.functions[0];
    // while → at least 3 blocks: entry, cond/body, after
    EXPECT_GE(irFn.blocks.size(), 3u);
}

TEST(IR, IfStmtCreatesBrTrueInstr)
{
    auto mod = buildModule("if_fn", [](Function& fn) {
        Variable& x = fn.createInt("x", 5);
        fn.ifStmt(x > int64_t(0), [&] { fn.assign(x, expr(int64_t(0))); });
        fn.ret();
    });
    const IRFunction& irFn = mod.functions[0];
    bool foundBranch = false;
    for (const auto& blk : irFn.blocks)
        for (const auto& ins : blk.instrs)
            if (ins.op == IROp::BrTrue)
                foundBranch = true;
    EXPECT_TRUE(foundBranch);
}

TEST(IR, ModuleHasCorrectName)
{
    auto mod = buildModule("mymodule", [](Function&) {});
    EXPECT_EQ(mod.name, "mymodule");
}

TEST(IR, ValuesTablePopulated)
{
    auto mod = buildModule("vals", [](Function& fn) {
        Variable& x = fn.createInt("x", 7);
        fn.assign(x, expr(int64_t(7)));
    });
    const IRFunction& irFn = mod.functions[0];
    EXPECT_FALSE(irFn.values.empty());
}

TEST(IR, ModuleLookupFindsFunction)
{
    auto module = buildModule("lookup_mod", [](Function& function) { function.ret(); });
    EXPECT_NE(module.findFunction("lookup_mod"), nullptr);
    EXPECT_EQ(module.findFunction("missing"), nullptr);
}

TEST(IR, FunctionLookupFindsEntryBlock)
{
    auto module = buildModule("lookup_block", [](Function& function) { function.ret(); });
    const IRFunction& irFunction = module.functions[0];
    ASSERT_NE(irFunction.entryBlock(), nullptr);
    EXPECT_EQ(irFunction.findBlock(irFunction.entryBlock()->id), irFunction.entryBlock());
}

TEST(IR, InstructionExposesOpcodeBehavior)
{
    IRInstr branchInstruction{};
    branchInstruction.op = IROp::BrTrue;

    IRInstr addInstruction{};
    addInstruction.op = IROp::Add;

    EXPECT_EQ(branchInstruction.opName(), "brtrue");
    EXPECT_TRUE(branchInstruction.isTerminator());
    EXPECT_TRUE(addInstruction.isArithmetic());
    EXPECT_FALSE(addInstruction.isTerminator());
}

TEST(IR, MutableHelpersAndHashesWorkWithIrContainers)
{
    // Given
    IRModule module{};
    module.name = "manual";
    module.functions.push_back(IRFunction{ "main", { IRBlock{ "entry", BlockId{ 0 }, {} } }, {} });

    // When
    IRFunction* function = module.findFunction("main");

    // Then
    ASSERT_NE(function, nullptr);
    ASSERT_NE(function->entryBlock(), nullptr);
    EXPECT_TRUE(function->entryBlock()->empty());

    function->entryBlock()->instrs.push_back(IRInstr{ IROp::Ret, {}, {}, {}, {}, {}, {}, {} });
    EXPECT_FALSE(function->entryBlock()->empty());
    EXPECT_EQ(function->findBlock(BlockId{ 0 }), function->entryBlock());
    EXPECT_EQ(function->findBlock(BlockId{ 99 }), nullptr);
    EXPECT_EQ(module.findFunction("missing"), nullptr);

    std::unordered_map<ValueId, int, ValueIdHash> valueMap{};
    valueMap.emplace(ValueId{ 1 }, 11);
    EXPECT_EQ(valueMap.at(ValueId{ 1 }), 11);

    std::unordered_map<BlockId, int, BlockIdHash> blockMap{};
    blockMap.emplace(BlockId{ 2 }, 22);
    EXPECT_EQ(blockMap.at(BlockId{ 2 }), 22);
}

TEST(IR, OpcodeHelpersCoverArithmeticAndTerminators)
{
    // Given / When / Then
    EXPECT_EQ(irOpName(IROp::Load), "load");
    EXPECT_EQ(irOpName(IROp::Store), "store");
    EXPECT_FALSE(isArithmeticOp(IROp::Call));
    EXPECT_TRUE(isArithmeticOp(IROp::Neg));

    IRInstr jumpInstruction{};
    jumpInstruction.op = IROp::Jmp;
    EXPECT_TRUE(jumpInstruction.isTerminator());

    IRInstr retInstruction{};
    retInstruction.op = IROp::Ret;
    EXPECT_TRUE(retInstruction.isTerminator());
}
