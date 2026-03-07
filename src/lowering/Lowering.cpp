#include <asmstudio/lowering/Lowering.hpp>

#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/api/Stmt.hpp>
#include <asmstudio/api/Variable.hpp>

#include <functional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace asmstudio
{
namespace
{
struct LoweringContext
{
    IRFunction& fn;
    DiagnosticBag& diags;
    std::uint32_t nextValue{ 0 };
    std::uint32_t nextBlock{ 0 };
    std::uint32_t loopCount{ 0 };
    std::uint32_t ifCount{ 0 };
    std::unordered_map<std::string, ValueId> varValue; // current SSA slot per variable name

    ValueId allocValue(DataType type, std::optional<IRConstant> constant = {})
    {
        ValueId id{ nextValue++ };
        fn.values.push_back(IRValue{ type, constant });
        return id;
    }

    BlockId allocBlock(std::string name)
    {
        BlockId id{ nextBlock++ };
        fn.blocks.push_back(IRBlock{ std::move(name), id, {} });
        return id;
    }

    IRBlock& block(BlockId id)
    {
        return fn.blocks[id.value];
    }

    void emit(BlockId b, IRInstr instr)
    {
        fn.blocks[b.value].instrs.push_back(std::move(instr));
    }
};

ValueId lowerExpr(LoweringContext& ctx, BlockId cur, const ExprNode& exprNode)
{
    return std::visit(
        [&](auto&& node) -> ValueId {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, ConstExpr>)
            {
                DataType dt = std::visit(
                    []<typename V>(V) -> DataType {
                        if constexpr (std::is_same_v<V, int64_t>)
                        {
                            return DataType::Int64;
                        }
                        else if constexpr (std::is_same_v<V, uint64_t>)
                        {
                            return DataType::UInt64;
                        }
                        else if constexpr (std::is_same_v<V, double>)
                        {
                            return DataType::Float64;
                        }
                        else
                        {
                            return DataType::Bool;
                        }
                    },
                    node.value);

                ValueId id = ctx.allocValue(dt, node.value);
                IRInstr instr;
                instr.op = IROp::Const;
                instr.output = id;
                instr.constVal = node.value;
                ctx.emit(cur, std::move(instr));
                return id;
            }
            else if constexpr (std::is_same_v<T, VarExpr>)
            {
                const Variable& v = node.var.get();
                auto it = ctx.varValue.find(std::string(v.name()));
                if (it != ctx.varValue.end())
                {
                    return it->second;
                }

                // Variable hasn't been assigned yet — emit its init as a constant.
                ValueId id = ctx.allocValue(v.type());
                IRInstr instr;
                instr.op = IROp::Const;
                instr.output = id;
                instr.constVal = std::visit([]<typename V>(V val) -> IRConstant { return val; }, v.initVariant());
                ctx.emit(cur, std::move(instr));
                ctx.varValue[std::string(v.name())] = id;
                return id;
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<BinExpr>>)
            {
                ValueId lhs = lowerExpr(ctx, cur, node->lhs);
                ValueId rhs = lowerExpr(ctx, cur, node->rhs);
                ValueId out = ctx.allocValue(ctx.fn.values[lhs.value].type);
                IRInstr instr;

                instr.op = [&]() -> IROp {
                    switch (node->op)
                    {
                    case BinOp::Add: return IROp::Add;
                    case BinOp::Sub: return IROp::Sub;
                    case BinOp::Mul: return IROp::Mul;
                    case BinOp::Div: return IROp::Div;
                    case BinOp::Mod: return IROp::Mod;
                    case BinOp::And: return IROp::And;
                    case BinOp::Or: return IROp::Or;
                    case BinOp::Xor: return IROp::Xor;
                    case BinOp::Shl: return IROp::Shl;
                    case BinOp::Shr: return IROp::Shr;
                    }
                    return IROp::Add;
                }();

                instr.inputs = { lhs, rhs };
                instr.output = out;
                ctx.emit(cur, std::move(instr));
                return out;
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<UnaryExpr>>)
            {
                ValueId operand = lowerExpr(ctx, cur, node->operand);
                ValueId out = ctx.allocValue(ctx.fn.values[operand.value].type);
                IRInstr instr;
                instr.op = (node->op == UnaryOp::Neg) ? IROp::Neg : IROp::Not;
                instr.inputs = { operand };
                instr.output = out;
                ctx.emit(cur, std::move(instr));
                return out;
            }
            else
            {
                // Shouldn't reach here — all ExprNode variants should be handled.
                return lowerExpr(ctx, cur, node->inner);
            }
        },
        exprNode);
}

ValueId lowerOperand(LoweringContext& ctx, BlockId cur, const Operand& op)
{
    return std::visit(
        [&](auto&& o) -> ValueId {
            using T = std::decay_t<decltype(o)>;
            if constexpr (std::is_same_v<T, std::reference_wrapper<const Variable>>)
            {
                return lowerExpr(ctx, cur, VarExpr{ o });
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
                return lowerExpr(ctx, cur, ConstExpr{ { o } });
            }
            else if constexpr (std::is_same_v<T, std::uint64_t>)
            {
                return lowerExpr(ctx, cur, ConstExpr{ { o } });
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return lowerExpr(ctx, cur, ConstExpr{ { o } });
            }
            else // bool
            {
                return lowerExpr(ctx, cur, ConstExpr{ { o } });
            }
        },
        op);
}

// Forward declare for recursive statement lowering.
BlockId lowerStmts(LoweringContext& ctx, BlockId cur, std::span<const Stmt> stmts);

BlockId lowerStmt(LoweringContext& ctx, BlockId cur, const Stmt& stmt)
{
    return std::visit(
        [&](auto&& s) -> BlockId {
            using T = std::decay_t<decltype(s)>;

            if constexpr (std::is_same_v<T, AssignStmt>)
            {
                ValueId val = lowerExpr(ctx, cur, s.value);
                ValueId canonical = ctx.varValue[std::string(s.target.get().name())];
                IRInstr cp;
                cp.op = IROp::Copy;
                cp.inputs = { val };
                cp.output = canonical;
                ctx.emit(cur, std::move(cp));
                return cur;
            }
            else if constexpr (std::is_same_v<T, WhileStmt>)
            {
                std::uint32_t idx = ctx.loopCount++;
                BlockId condBlock = ctx.allocBlock("while_cond_" + std::to_string(idx));
                BlockId bodyBlock = ctx.allocBlock("while_body_" + std::to_string(idx));
                BlockId afterBlock = ctx.allocBlock("while_after_" + std::to_string(idx));

                // Jump from cur → condBlock.
                IRInstr jmpIn;
                jmpIn.op = IROp::Jmp;
                jmpIn.trueTarget = condBlock;
                ctx.emit(cur, std::move(jmpIn));

                // condBlock: evaluate condition, branch.
                ValueId lhsVal = lowerOperand(ctx, condBlock, s.cond.lhs);
                ValueId rhsVal = lowerOperand(ctx, condBlock, s.cond.rhs);
                ValueId condVal = ctx.allocValue(DataType::Bool);
                IRInstr cmpI;
                cmpI.op = IROp::Cmp;
                cmpI.inputs = { lhsVal, rhsVal };
                cmpI.output = condVal;
                cmpI.cmpKind = s.cond.kind;
                ctx.emit(condBlock, std::move(cmpI));

                IRInstr brI;
                brI.op = IROp::BrTrue;
                brI.inputs = { condVal };
                brI.trueTarget = bodyBlock;
                brI.falseTarget = afterBlock;
                ctx.emit(condBlock, std::move(brI));

                // bodyBlock: lower body statements, then jump back to condBlock.
                BlockId bodyEnd = lowerStmts(ctx, bodyBlock, s.body);
                IRInstr backJmp;
                backJmp.op = IROp::Jmp;
                backJmp.trueTarget = condBlock;
                ctx.emit(bodyEnd, std::move(backJmp));

                return afterBlock;
            }
            else if constexpr (std::is_same_v<T, IfStmt>)
            {
                std::uint32_t idx = ctx.ifCount++;
                BlockId thenBlock = ctx.allocBlock("if_then_" + std::to_string(idx));
                BlockId elseBlock = ctx.allocBlock("if_else_" + std::to_string(idx));
                BlockId mergeBlock = ctx.allocBlock("if_merge_" + std::to_string(idx));

                ValueId lhsVal = lowerOperand(ctx, cur, s.cond.lhs);
                ValueId rhsVal = lowerOperand(ctx, cur, s.cond.rhs);
                ValueId condVal = ctx.allocValue(DataType::Bool);
                IRInstr cmpI;
                cmpI.op = IROp::Cmp;
                cmpI.inputs = { lhsVal, rhsVal };
                cmpI.output = condVal;
                cmpI.cmpKind = s.cond.kind;
                ctx.emit(cur, std::move(cmpI));

                IRInstr brI;
                brI.op = IROp::BrTrue;
                brI.inputs = { condVal };
                brI.trueTarget = thenBlock;
                brI.falseTarget = elseBlock;
                ctx.emit(cur, std::move(brI));

                BlockId thenEnd = lowerStmts(ctx, thenBlock, s.thenBody);
                IRInstr thenJmp;
                thenJmp.op = IROp::Jmp;
                thenJmp.trueTarget = mergeBlock;
                ctx.emit(thenEnd, std::move(thenJmp));

                BlockId elseEnd = lowerStmts(ctx, elseBlock, s.elseBody);
                IRInstr elseJmp;
                elseJmp.op = IROp::Jmp;
                elseJmp.trueTarget = mergeBlock;
                ctx.emit(elseEnd, std::move(elseJmp));

                return mergeBlock;
            }
            else if constexpr (std::is_same_v<T, ReturnStmt>)
            {
                IRInstr retI;
                retI.op = IROp::Ret;

                if (s.value)
                {
                    ValueId val = lowerExpr(ctx, cur, *s.value);
                    retI.inputs = { val };
                }

                ctx.emit(cur, std::move(retI));
                return cur;
            }
            else // CallStmt
            {
                std::vector<ValueId> argIds;
                argIds.reserve(s.args.size());

                for (const auto& arg : s.args)
                {
                    argIds.push_back(lowerExpr(ctx, cur, arg));
                }

                IRInstr callI;
                callI.op = IROp::Call;
                callI.callee = s.callee;
                callI.inputs = std::move(argIds);
                ctx.emit(cur, std::move(callI));
                return cur;
            }
        },
        stmt.var());
}

BlockId lowerStmts(LoweringContext& ctx, BlockId cur, std::span<const Stmt> stmts)
{
    for (const auto& stmt : stmts)
    {
        cur = lowerStmt(ctx, cur, stmt);
    }
    return cur;
}

IRFunction lowerFunction(const Function& fn, DiagnosticBag& diags)
{
    IRFunction irFn;
    irFn.name = std::string(fn.name());

    LoweringContext ctx{ irFn, diags };

    // Initialize variable current values to their initial constants.
    // We use TWO values per variable:
    //   1. An immutable Const (for the init literal) — ConstFolding may use this.
    //   2. A mutable canonical slot (Copy of the init) with NO constant field.
    //      The optimizer will NOT constant-fold reads of the canonical slot,
    //      which is correct because the variable is overwritten by assignments.
    BlockId entryBlock = ctx.allocBlock("entry");
    for (const auto& var : fn.variables())
    {
        IRConstant initC = std::visit([]<typename T>(T v) -> IRConstant { return IRConstant{ v }; }, var->initVariant());

        // Immutable init constant.
        ValueId initId = ctx.allocValue(var->type(), initC);
        IRInstr initI;
        initI.op = IROp::Const;
        initI.output = initId;
        initI.constVal = initC;
        ctx.emit(entryBlock, std::move(initI));

        // Mutable canonical slot — no constant so ConstFolding doesn't freeze it.
        ValueId canonical = ctx.allocValue(var->type(), std::nullopt);
        IRInstr copyI;
        copyI.op = IROp::Copy;
        copyI.inputs = { initId };
        copyI.output = canonical;
        ctx.emit(entryBlock, std::move(copyI));

        ctx.varValue[std::string(var->name())] = canonical;
    }

    lowerStmts(ctx, entryBlock, fn.statements());

    return irFn;
}
} // namespace

IRModule lower(std::string_view moduleName, std::span<const std::unique_ptr<Function>> functions, DiagnosticBag& diags)
{
    IRModule module;
    module.name = std::string(moduleName);
    module.functions.reserve(functions.size());

    for (const auto& fn : functions)
    {
        module.functions.push_back(lowerFunction(*fn, diags));
    }

    return module;
}
} // namespace asmstudio
