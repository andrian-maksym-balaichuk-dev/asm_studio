#include <asmstudio/ir/IRTypes.hpp>
#include <asmstudio/optimizer/Optimizer.hpp>

#include <gtest/gtest.h>

using namespace asmstudio;

static IRFunction makeSimpleFn(std::string_view name)
{
    IRFunction fn;
    fn.name = std::string(name);
    fn.blocks.push_back(IRBlock{ "entry", BlockId{ 0 }, {} });
    return fn;
}

static ValueId addValue(IRFunction& fn, DataType type, std::optional<IRConstant> constant = std::nullopt)
{
    ValueId id{ static_cast<uint32_t>(fn.values.size()) };
    fn.values.push_back({ type, constant });
    return id;
}

static void addInstr(IRFunction& fn, std::size_t blkIdx, IRInstr instr)
{
    fn.blocks[blkIdx].instrs.push_back(std::move(instr));
}

TEST(Optimizer, ConstFoldAdd)
{
    IRFunction fn = makeSimpleFn("cf_add");

    // v0 = const 3, v1 = const 4, v2 = add v0, v1
    ValueId v0 = addValue(fn, DataType::Int64, IRConstant{ int64_t(3) });
    ValueId v1 = addValue(fn, DataType::Int64, IRConstant{ int64_t(4) });
    ValueId v2 = addValue(fn, DataType::Int64);

    IRInstr c0;
    c0.op = IROp::Const;
    c0.output = v0;
    c0.constVal = IRConstant{ int64_t(3) };
    IRInstr c1;
    c1.op = IROp::Const;
    c1.output = v1;
    c1.constVal = IRConstant{ int64_t(4) };
    IRInstr add;
    add.op = IROp::Add;
    add.inputs = { v0, v1 };
    add.output = v2;

    addInstr(fn, 0, c0);
    addInstr(fn, 0, c1);
    addInstr(fn, 0, add);

    ConstantFolding cf;
    bool changed = cf.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_TRUE(fn.values[v2.value].constant.has_value());
    EXPECT_EQ(std::get<int64_t>(*fn.values[v2.value].constant), 7);
}

TEST(Optimizer, ConstFoldNoDivByZero)
{
    IRFunction fn = makeSimpleFn("cf_div0");

    ValueId v0 = addValue(fn, DataType::Int64, IRConstant{ int64_t(10) });
    ValueId v1 = addValue(fn, DataType::Int64, IRConstant{ int64_t(0) });
    ValueId v2 = addValue(fn, DataType::Int64);

    IRInstr c0;
    c0.op = IROp::Const;
    c0.output = v0;
    c0.constVal = IRConstant{ int64_t(10) };
    IRInstr c1;
    c1.op = IROp::Const;
    c1.output = v1;
    c1.constVal = IRConstant{ int64_t(0) };
    IRInstr dv;
    dv.op = IROp::Div;
    dv.inputs = { v0, v1 };
    dv.output = v2;

    addInstr(fn, 0, c0);
    addInstr(fn, 0, c1);
    addInstr(fn, 0, dv);

    ConstantFolding cf;
    bool changed = cf.run(fn);
    // Div by zero should NOT be folded.
    EXPECT_FALSE(changed);
}

TEST(Optimizer, DCERemovesUnused)
{
    IRFunction fn = makeSimpleFn("dce");

    // v0 = const 1 — never used
    // ret
    ValueId v0 = addValue(fn, DataType::Int64, IRConstant{ int64_t(1) });

    IRInstr c0;
    c0.op = IROp::Const;
    c0.output = v0;
    c0.constVal = IRConstant{ int64_t(1) };
    IRInstr ret;
    ret.op = IROp::Ret;

    addInstr(fn, 0, c0);
    addInstr(fn, 0, ret);

    auto before = fn.blocks[0].instrs.size();
    DeadCodeElim dce;
    bool changed = dce.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_LT(fn.blocks[0].instrs.size(), before);
}

TEST(Optimizer, DCEKeepsUsedValue)
{
    IRFunction fn = makeSimpleFn("dce_keep");

    ValueId v0 = addValue(fn, DataType::Int64, IRConstant{ int64_t(42) });

    IRInstr c0;
    c0.op = IROp::Const;
    c0.output = v0;
    c0.constVal = IRConstant{ int64_t(42) };
    IRInstr ret;
    ret.op = IROp::Ret;
    ret.inputs = { v0 };

    addInstr(fn, 0, c0);
    addInstr(fn, 0, ret);

    DeadCodeElim dce;
    bool changed = dce.run(fn);
    EXPECT_FALSE(changed); // v0 is used by ret, nothing to remove
    EXPECT_EQ(fn.blocks[0].instrs.size(), 2u);
}

TEST(Optimizer, UnreachableBlockRemoved)
{
    IRFunction fn = makeSimpleFn("ube");

    // Block 0 → jmp block 1, block 2 unreachable
    fn.blocks.push_back(IRBlock{ "after", BlockId{ 1 }, {} });
    fn.blocks.push_back(IRBlock{ "dead", BlockId{ 2 }, {} });

    IRInstr jmp;
    jmp.op = IROp::Jmp;
    jmp.trueTarget = BlockId{ 1 };
    IRInstr ret;
    ret.op = IROp::Ret;

    fn.blocks[0].instrs.push_back(jmp);
    fn.blocks[1].instrs.push_back(ret);
    fn.blocks[2].instrs.push_back(ret);

    UnreachableBlockElim ube;
    bool changed = ube.run(fn);
    EXPECT_TRUE(changed);
    EXPECT_EQ(fn.blocks.size(), 2u);
}

TEST(Optimizer, JumpSimplificationFoldsTrue)
{
    IRFunction fn = makeSimpleFn("js");
    fn.blocks.push_back(IRBlock{ "then", BlockId{ 1 }, {} });
    fn.blocks.push_back(IRBlock{ "else_", BlockId{ 2 }, {} });

    // v0 = const true
    ValueId v0 = addValue(fn, DataType::Bool, IRConstant{ bool(true) });
    IRInstr c0;
    c0.op = IROp::Const;
    c0.output = v0;
    c0.constVal = IRConstant{ bool(true) };
    IRInstr br;
    br.op = IROp::BrTrue;
    br.inputs = { v0 };
    br.trueTarget = BlockId{ 1 };
    br.falseTarget = BlockId{ 2 };

    fn.blocks[0].instrs.push_back(c0);
    fn.blocks[0].instrs.push_back(br);
    fn.blocks[1].instrs.push_back(IRInstr{ IROp::Ret, {}, {}, {}, {}, {}, {}, {} });
    fn.blocks[2].instrs.push_back(IRInstr{ IROp::Ret, {}, {}, {}, {}, {}, {}, {} });

    JumpSimplification js;
    bool changed = js.run(fn);
    EXPECT_TRUE(changed);
    // The BrTrue should have become Jmp to block 1 (taken branch).
    auto& br2 = fn.blocks[0].instrs.back();
    EXPECT_EQ(br2.op, IROp::Jmp);
    EXPECT_EQ(br2.trueTarget, BlockId{ 1 });
}

TEST(Optimizer, LevelNoneHasNoPasses)
{
    auto opt = makeOptimizer(OptimizationLevel::None);
    EXPECT_EQ(opt.passCount(), 0u);
}

TEST(Optimizer, LevelBasicHasPasses)
{
    auto opt = makeOptimizer(OptimizationLevel::Basic);
    EXPECT_GT(opt.passCount(), 0u);
}

TEST(Optimizer, LevelAggressiveHasMorePasses)
{
    auto basic = makeOptimizer(OptimizationLevel::Basic);
    auto aggr = makeOptimizer(OptimizationLevel::Aggressive);
    EXPECT_GE(aggr.passCount(), basic.passCount());
}
