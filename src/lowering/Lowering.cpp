#include <asmstudio/lowering/Lowering.hpp>

#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/api/Stmt.hpp>
#include <asmstudio/api/Variable.hpp>
#include <asmstudio/core/Compat.hpp>

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace asmstudio
{
namespace
{
class LoweringContext
{
public:
    using VariableTable = std::unordered_map<std::string_view, ValueId>;

    explicit LoweringContext(IRFunction& function, std::size_t variableCount, DiagnosticBag& /*diagnostics*/)
    : m_function{ function }
    {
        m_variableTable.reserve(variableCount);
    }

    [[nodiscard]] ValueId allocValue(const DataType type, std::optional<IRConstant> constant = {})
    {
        ValueId id{ m_nextValue++ };
        m_function.values.push_back(IRValue{ type, constant });
        return id;
    }

    [[nodiscard]] BlockId allocBlock(std::string name)
    {
        const BlockId id{ m_nextBlock++ };
        m_function.blocks.push_back(IRBlock{ std::move(name), id, {} });
        return id;
    }

    [[nodiscard]] IRBlock& blockAt(const BlockId id) noexcept
    {
        return m_function.blocks[id.value];
    }

    [[nodiscard]] IRValue& valueAt(const ValueId id) noexcept
    {
        return m_function.values[id.value];
    }

    void emit(const BlockId blockId, IRInstr instruction)
    {
        m_function.blocks[blockId.value].instrs.push_back(std::move(instruction));
    }

    void setVariable(std::string_view name, ValueId id)
    {
        m_variableTable.insert_or_assign(name, id);
    }

    [[nodiscard]] std::optional<ValueId> lookupVariable(std::string_view name) const
    {
        const auto it = m_variableTable.find(name);
        if (it != m_variableTable.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::uint32_t nextLoopIndex() noexcept
    {
        return m_loopCount++;
    }
    [[nodiscard]] std::uint32_t nextIfIndex() noexcept
    {
        return m_ifCount++;
    }

private:
    IRFunction& m_function;
    std::uint32_t m_nextValue{ 0 };
    std::uint32_t m_nextBlock{ 0 };
    std::uint32_t m_loopCount{ 0 };
    std::uint32_t m_ifCount{ 0 };
    VariableTable m_variableTable;
};

[[nodiscard]] IROp binOpToIROp(BinOp operation) noexcept
{
    switch (operation)
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
    return IROp::Add; // unreachable
}

[[nodiscard]] DataType dataTypeFromConstant(const IRConstant& constant) noexcept
{
    return std::visit(
        compat::Overloaded{
            [](int64_t) -> DataType { return DataType::Int64; },
            [](uint64_t) -> DataType { return DataType::UInt64; },
            [](double) -> DataType { return DataType::Float64; },
            [](bool) -> DataType { return DataType::Bool; },
        },
        constant);
}

// Forward declaration for mutual recursion between lowerExpression and lowerStatements.
BlockId lowerStatements(LoweringContext& context, BlockId currentBlock, compat::Span<const Stmt> statements);

[[nodiscard]] ValueId lowerExpression(LoweringContext& context, BlockId currentBlock, const ExprNode& expression);

[[nodiscard]] ValueId lowerConstantExpression(LoweringContext& context, BlockId currentBlock, const ConstExpr& constExpression)
{
    const DataType dataType{ dataTypeFromConstant(constExpression.value) };
    const ValueId outputId{ context.allocValue(dataType, constExpression.value) };

    IRInstr instruction{};
    instruction.op = IROp::Const;
    instruction.output = outputId;
    instruction.constVal = constExpression.value;
    context.emit(currentBlock, std::move(instruction));
    return outputId;
}

[[nodiscard]] ValueId lowerVariableExpression(LoweringContext& context, BlockId currentBlock, const VarExpr& varExpression)
{
    const Variable& variable{ varExpression.variable.get() };
    const auto existingId{ context.lookupVariable(variable.name()) };
    if (existingId)
    {
        return *existingId;
    }

    // Variable accessed before explicit assignment — emit its initialiser as a constant.
    const IRConstant initConstant{ std::visit([](auto value) -> IRConstant { return value; }, variable.initVariant()) };
    const ValueId outputId{ context.allocValue(variable.type()) };

    IRInstr instruction{};
    instruction.op = IROp::Const;
    instruction.output = outputId;
    instruction.constVal = initConstant;
    context.emit(currentBlock, std::move(instruction));
    context.setVariable(variable.name(), outputId);
    return outputId;
}

[[nodiscard]] ValueId lowerBinaryExpression(LoweringContext& context, BlockId currentBlock, const std::shared_ptr<BinExpr>& binaryExpression)
{
    const ValueId leftValue{ lowerExpression(context, currentBlock, binaryExpression->leftOperand) };
    const ValueId rightValue{ lowerExpression(context, currentBlock, binaryExpression->rightOperand) };
    const ValueId outputId{ context.allocValue(context.valueAt(leftValue).type) };

    IRInstr instruction{};
    instruction.op = binOpToIROp(binaryExpression->op);
    instruction.inputs = { leftValue, rightValue };
    instruction.output = outputId;
    context.emit(currentBlock, std::move(instruction));
    return outputId;
}

[[nodiscard]] ValueId lowerUnaryExpression(LoweringContext& context, BlockId currentBlock, const std::shared_ptr<UnaryExpr>& unaryExpression)
{
    const ValueId operandValue{ lowerExpression(context, currentBlock, unaryExpression->operand) };
    const ValueId outputId{ context.allocValue(context.valueAt(operandValue).type) };

    IRInstr instruction{};
    instruction.op = (unaryExpression->op == UnaryOp::Neg) ? IROp::Neg : IROp::Not;
    instruction.inputs = { operandValue };
    instruction.output = outputId;
    context.emit(currentBlock, std::move(instruction));
    return outputId;
}

[[nodiscard]] ValueId lowerExpression(LoweringContext& context, BlockId currentBlock, const ExprNode& expression)
{
    return std::visit(
        [&](auto&& node) -> ValueId {
            using NodeType = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<NodeType, ConstExpr>)
            {
                return lowerConstantExpression(context, currentBlock, node);
            }
            else if constexpr (std::is_same_v<NodeType, VarExpr>)
            {
                return lowerVariableExpression(context, currentBlock, node);
            }
            else if constexpr (std::is_same_v<NodeType, std::shared_ptr<BinExpr>>)
            {
                return lowerBinaryExpression(context, currentBlock, node);
            }
            else
            {
                return lowerUnaryExpression(context, currentBlock, node);
            }
        },
        expression);
}

[[nodiscard]] ValueId lowerOperand(LoweringContext& context, BlockId currentBlock, const Operand& operand)
{
    return std::visit(
        [&](auto&& value) -> ValueId {
            using ValueType = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<ValueType, std::reference_wrapper<const Variable>>)
            {
                return lowerVariableExpression(context, currentBlock, VarExpr{ value });
            }
            else
            {
                return lowerConstantExpression(context, currentBlock, ConstExpr{ { value } });
            }
        },
        operand);
}

[[nodiscard]] BlockId lowerAssignStatement(LoweringContext& context, BlockId currentBlock, const AssignStmt& statement)
{
    const ValueId newValue{ lowerExpression(context, currentBlock, statement.value) };
    const ValueId canonicalSlot{ *context.lookupVariable(statement.target.get().name()) };

    IRInstr copyInstruction{};
    copyInstruction.op = IROp::Copy;
    copyInstruction.inputs = { newValue };
    copyInstruction.output = canonicalSlot;
    context.emit(currentBlock, std::move(copyInstruction));
    return currentBlock;
}

[[nodiscard]] BlockId lowerWhileStatement(LoweringContext& context, BlockId currentBlock, const WhileStmt& statement)
{
    const std::uint32_t loopIndex{ context.nextLoopIndex() };
    const BlockId conditionBlock{ context.allocBlock("while_cond_" + std::to_string(loopIndex)) };
    const BlockId loopBodyBlock{ context.allocBlock("while_body_" + std::to_string(loopIndex)) };
    const BlockId afterLoopBlock{ context.allocBlock("while_after_" + std::to_string(loopIndex)) };

    // Unconditional jump from the current block into the condition check.
    IRInstr jumpToCondition{};
    jumpToCondition.op = IROp::Jmp;
    jumpToCondition.trueTarget = conditionBlock;
    context.emit(currentBlock, std::move(jumpToCondition));

    // Condition block: evaluate the loop guard, then branch.
    const ValueId leftValue{ lowerOperand(context, conditionBlock, statement.condition.leftOperand) };
    const ValueId rightValue{ lowerOperand(context, conditionBlock, statement.condition.rightOperand) };
    const ValueId conditionValue{ context.allocValue(DataType::Bool) };

    IRInstr compareInstruction{};
    compareInstruction.op = IROp::Cmp;
    compareInstruction.inputs = { leftValue, rightValue };
    compareInstruction.output = conditionValue;
    compareInstruction.cmpKind = statement.condition.comparisonKind;
    context.emit(conditionBlock, std::move(compareInstruction));

    IRInstr branchInstruction{};
    branchInstruction.op = IROp::BrTrue;
    branchInstruction.inputs = { conditionValue };
    branchInstruction.trueTarget = loopBodyBlock;
    branchInstruction.falseTarget = afterLoopBlock;
    context.emit(conditionBlock, std::move(branchInstruction));

    // Body block: lower body statements, then loop back to the condition.
    const BlockId loopBodyEnd{ lowerStatements(context, loopBodyBlock, statement.body) };

    IRInstr backEdgeJump{};
    backEdgeJump.op = IROp::Jmp;
    backEdgeJump.trueTarget = conditionBlock;
    context.emit(loopBodyEnd, std::move(backEdgeJump));

    return afterLoopBlock;
}

[[nodiscard]] BlockId lowerIfStatement(LoweringContext& context, BlockId currentBlock, const IfStmt& statement)
{
    const std::uint32_t ifIndex{ context.nextIfIndex() };
    const BlockId thenBlock{ context.allocBlock("if_then_" + std::to_string(ifIndex)) };
    const BlockId elseBlock{ context.allocBlock("if_else_" + std::to_string(ifIndex)) };
    const BlockId mergeBlock{ context.allocBlock("if_merge_" + std::to_string(ifIndex)) };

    const ValueId leftValue{ lowerOperand(context, currentBlock, statement.condition.leftOperand) };
    const ValueId rightValue{ lowerOperand(context, currentBlock, statement.condition.rightOperand) };
    const ValueId conditionValue{ context.allocValue(DataType::Bool) };

    IRInstr compareInstruction{};
    compareInstruction.op = IROp::Cmp;
    compareInstruction.inputs = { leftValue, rightValue };
    compareInstruction.output = conditionValue;
    compareInstruction.cmpKind = statement.condition.comparisonKind;
    context.emit(currentBlock, std::move(compareInstruction));

    IRInstr branchInstruction{};
    branchInstruction.op = IROp::BrTrue;
    branchInstruction.inputs = { conditionValue };
    branchInstruction.trueTarget = thenBlock;
    branchInstruction.falseTarget = elseBlock;
    context.emit(currentBlock, std::move(branchInstruction));

    const BlockId thenBlockEnd{ lowerStatements(context, thenBlock, statement.thenBody) };

    IRInstr thenMergeJump{};
    thenMergeJump.op = IROp::Jmp;
    thenMergeJump.trueTarget = mergeBlock;
    context.emit(thenBlockEnd, std::move(thenMergeJump));

    const BlockId elseBlockEnd{ lowerStatements(context, elseBlock, statement.elseBody) };

    IRInstr elseMergeJump{};
    elseMergeJump.op = IROp::Jmp;
    elseMergeJump.trueTarget = mergeBlock;
    context.emit(elseBlockEnd, std::move(elseMergeJump));

    return mergeBlock;
}

[[nodiscard]] BlockId lowerReturnStatement(LoweringContext& context, BlockId currentBlock, const ReturnStmt& statement)
{
    IRInstr returnInstruction{};
    returnInstruction.op = IROp::Ret;

    if (statement.value)
    {
        const ValueId returnValue{ lowerExpression(context, currentBlock, *statement.value) };
        returnInstruction.inputs = { returnValue };
    }

    context.emit(currentBlock, std::move(returnInstruction));
    return currentBlock;
}

[[nodiscard]] BlockId lowerCallStatement(LoweringContext& context, BlockId currentBlock, const CallStmt& statement)
{
    std::vector<ValueId> argumentIds{};
    argumentIds.reserve(statement.arguments.size());

    for (const auto& argument : statement.arguments)
    {
        argumentIds.push_back(lowerExpression(context, currentBlock, argument));
    }

    IRInstr callInstruction{};
    callInstruction.op = IROp::Call;
    callInstruction.callee = statement.callee;
    callInstruction.inputs = std::move(argumentIds);
    context.emit(currentBlock, std::move(callInstruction));
    return currentBlock;
}

[[nodiscard]] BlockId lowerStatement(LoweringContext& context, BlockId currentBlock, const Stmt& statement)
{
    return std::visit(
        [&](auto&& node) -> BlockId {
            using NodeType = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<NodeType, AssignStmt>)
            {
                return lowerAssignStatement(context, currentBlock, node);
            }
            else if constexpr (std::is_same_v<NodeType, WhileStmt>)
            {
                return lowerWhileStatement(context, currentBlock, node);
            }
            else if constexpr (std::is_same_v<NodeType, IfStmt>)
            {
                return lowerIfStatement(context, currentBlock, node);
            }
            else if constexpr (std::is_same_v<NodeType, ReturnStmt>)
            {
                return lowerReturnStatement(context, currentBlock, node);
            }
            else
            {
                return lowerCallStatement(context, currentBlock, node);
            }
        },
        statement.variant());
}

BlockId lowerStatements(LoweringContext& context, BlockId currentBlock, compat::Span<const Stmt> statements)
{
    for (const auto& statement : statements)
    {
        currentBlock = lowerStatement(context, currentBlock, statement);
    }
    return currentBlock;
}

[[nodiscard]] IRFunction lowerFunction(const Function& function, DiagnosticBag& diagnostics)
{
    IRFunction irFunction{};
    irFunction.name = std::string(function.name());
    irFunction.blocks.reserve(function.statements().size() + 1U);
    irFunction.values.reserve((function.variables().size() * 2U) + function.statements().size());

    LoweringContext context{ irFunction, function.variables().size(), diagnostics };

    // Emit two values per variable:
    //   1. An immutable Const — ConstFolding may propagate this.
    //   2. A mutable canonical slot (Copy of the init) with no constant field.
    //      Optimizer does not constant-fold canonical slots so mutations take effect.
    const BlockId entryBlock{ context.allocBlock("entry") };
    context.blockAt(entryBlock).instrs.reserve((function.variables().size() * 2U) + function.statements().size());

    for (const auto& variable : function.variables())
    {
        const IRConstant initialConstant{ std::visit([](auto value) -> IRConstant { return value; }, variable->initVariant()) };

        const ValueId initConstantId{ context.allocValue(variable->type(), initialConstant) };

        IRInstr constInstruction{};
        constInstruction.op = IROp::Const;
        constInstruction.output = initConstantId;
        constInstruction.constVal = initialConstant;
        context.emit(entryBlock, std::move(constInstruction));

        const ValueId canonicalSlotId{ context.allocValue(variable->type(), std::nullopt) };

        IRInstr copyInstruction{};
        copyInstruction.op = IROp::Copy;
        copyInstruction.inputs = { initConstantId };
        copyInstruction.output = canonicalSlotId;
        context.emit(entryBlock, std::move(copyInstruction));

        context.setVariable(variable->name(), canonicalSlotId);
    }

    lowerStatements(context, entryBlock, function.statements());
    return irFunction;
}
} // namespace

IRModule lower(std::string_view moduleName, compat::Span<const std::unique_ptr<Function>> functions, DiagnosticBag& diagnostics)
{
    IRModule module{};
    module.name = std::string(moduleName);
    module.functions.reserve(functions.size());

    for (const auto& function : functions)
    {
        module.functions.push_back(lowerFunction(*function, diagnostics));
    }

    return module;
}
} // namespace asmstudio
