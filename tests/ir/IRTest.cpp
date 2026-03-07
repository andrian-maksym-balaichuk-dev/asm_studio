#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/api/Program.hpp>
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/lowering/Lowering.hpp>

#include <gtest/gtest.h>

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
    auto mod = buildModule("empty", [](Function&) {});
    ASSERT_EQ(mod.functions.size(), 1u);
    EXPECT_FALSE(mod.functions[0].blocks.empty());
}

TEST(IR, ConstAssignCreatesConstInstr)
{
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
