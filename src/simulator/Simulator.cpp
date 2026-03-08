#include <asmstudio/simulator/Simulator.hpp>

#include <asmstudio/core/Compat.hpp>

#include <cmath>
#include <iomanip>

namespace asmstudio
{
namespace
{
struct BinaryOpVisitor
{
    IROp opcode;

    template <typename LeftType, typename RightType>
    [[nodiscard]] RegValue operator()(LeftType leftArg, RightType rightArg) const noexcept
    {
        using CommonType = std::common_type_t<LeftType, RightType>;
        const CommonType leftValue{ static_cast<CommonType>(leftArg) };
        const CommonType rightValue{ static_cast<CommonType>(rightArg) };

        switch (opcode)
        {
        case IROp::Add: return static_cast<LeftType>(leftValue + rightValue);
        case IROp::Sub: return static_cast<LeftType>(leftValue - rightValue);
        case IROp::Mul: return static_cast<LeftType>(leftValue * rightValue);
        case IROp::Div: return static_cast<LeftType>(leftValue / rightValue); // caller ensures rightValue != 0
        case IROp::Mod:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue % rightValue);
            }
            else
            {
                return static_cast<LeftType>(std::fmod(static_cast<double>(leftValue), static_cast<double>(rightValue)));
            }
        case IROp::And:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue & rightValue);
            }
            else
            {
                return leftArg;
            }
        case IROp::Or:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue | rightValue);
            }
            else
            {
                return leftArg;
            }
        case IROp::Xor:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue ^ rightValue);
            }
            else
            {
                return leftArg;
            }
        case IROp::Shl:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue << static_cast<int>(rightValue));
            }
            else
            {
                return leftArg;
            }
        case IROp::Shr:
            if constexpr (std::is_integral_v<CommonType>)
            {
                return static_cast<LeftType>(leftValue >> static_cast<int>(rightValue));
            }
            else
            {
                return leftArg;
            }
        default: return leftArg;
        }
    }
};

[[nodiscard]] RegValue applyBinaryOp(IROp opcode, RegValue leftOperand, RegValue rightOperand) noexcept
{
    return std::visit(BinaryOpVisitor{ opcode }, leftOperand, rightOperand);
}

[[nodiscard]] bool isDivisionByZero(IROp opcode, RegValue divisor) noexcept
{
    if (opcode != IROp::Div && opcode != IROp::Mod)
    {
        return false;
    }
    return std::visit(
        compat::Overloaded{
            [](int64_t value) noexcept -> bool { return value == int64_t{ 0 }; },
            [](uint64_t value) noexcept -> bool { return value == uint64_t{ 0 }; },
            [](double) noexcept -> bool { return false; },
            [](bool) noexcept -> bool { return false; },
        },
        divisor);
}

struct CompareVisitor
{
    CmpKind kind;

    template <typename LeftType, typename RightType>
    [[nodiscard]] bool operator()(LeftType leftArg, RightType rightArg) const noexcept
    {
        using CommonType = std::common_type_t<LeftType, RightType>;
        const CommonType leftValue{ static_cast<CommonType>(leftArg) };
        const CommonType rightValue{ static_cast<CommonType>(rightArg) };
        switch (kind)
        {
        case CmpKind::Lt: return leftValue < rightValue;
        case CmpKind::Le: return leftValue <= rightValue;
        case CmpKind::Eq: return leftValue == rightValue;
        case CmpKind::Ne: return leftValue != rightValue;
        case CmpKind::Ge: return leftValue >= rightValue;
        case CmpKind::Gt: return leftValue > rightValue;
        }
        return false;
    }
};

[[nodiscard]] bool applyComparison(CmpKind kind, RegValue leftOperand, RegValue rightOperand) noexcept
{
    return std::visit(CompareVisitor{ kind }, leftOperand, rightOperand);
}

[[nodiscard]] RegValue irConstantToRegValue(const IRConstant& constant) noexcept
{
    return std::visit(
        compat::Overloaded{
            [](int64_t value) noexcept -> RegValue { return value; },
            [](uint64_t value) noexcept -> RegValue { return value; },
            [](double value) noexcept -> RegValue { return value; },
            [](bool value) noexcept -> RegValue { return value; },
        },
        constant);
}

} // namespace

Simulator::Simulator(const IRModule& module, std::size_t maxSteps, std::size_t maxCallDepth)
: m_module{ module }, m_maxSteps{ maxSteps }, m_maxCallDepth{ maxCallDepth }
{
    m_functionCache.reserve(m_module.functions.size());
    for (const auto& function : m_module.functions)
    {
        auto& functionCache{ m_functionCache[function.name] };
        functionCache.function = &function;
        functionCache.blockIndexById.reserve(function.blocks.size());
        for (std::size_t blockIndex{ 0 }; blockIndex < function.blocks.size(); ++blockIndex)
        {
            functionCache.blockIndexById.emplace(function.blocks[blockIndex].id, blockIndex);
        }
    }
}

void Simulator::reset()
{
    m_state = SimState{};
    m_history.clear();
    m_trace.clear();
    m_done = true;
    m_stepCount = 0;
}

compat::Result<std::optional<RegValue>, SimError> Simulator::run(std::string_view functionName)
{
    const FunctionCache* functionCache{ findFunctionCache(functionName) };
    if (!functionCache || !functionCache->function)
    {
        return compat::makeUnexpected(SimError::UnknownFunction);
    }
    const IRFunction& function{ *functionCache->function };

    reset();
    m_done = false;
    m_state.callStack.reserve(m_maxCallDepth);

    SimFrame initialFrame{};
    initialFrame.functionName = std::string{ function.name };
    initialFrame.values.resize(function.values.size());
    initialFrame.block = function.entryBlock() ? function.entryBlock()->id : BlockId{ 0 };
    initialFrame.pc = 0;
    m_state.callStack.push_back(std::move(initialFrame));

    while (!m_done)
    {
        m_history.push_back(m_state);
        auto stepResult{ stepOne() };
        if (!stepResult)
        {
            return compat::makeUnexpected(stepResult.error());
        }

        ++m_stepCount;
        if (m_stepCount > m_maxSteps)
        {
            return compat::makeUnexpected(SimError::MaxStepsExceeded);
        }
    }

    if (m_trace.empty())
    {
        return std::optional<RegValue>{};
    }
    return std::optional<RegValue>{ m_trace.back() };
}

compat::Result<bool, SimError> Simulator::step()
{
    if (m_done)
    {
        return false;
    }
    m_history.push_back(m_state);
    return stepOne();
}

void Simulator::rewind(std::size_t steps)
{
    steps = std::min(steps, m_history.size());
    for (std::size_t stepIndex{ 0 }; stepIndex < steps; ++stepIndex)
    {
        m_history.pop_back();
    }
    if (!m_history.empty())
    {
        m_state = m_history.back();
        m_done = false;
    }
}

void Simulator::printTrace(std::ostream& out) const
{
    out << "=== Simulation Trace ===\n";
    for (std::size_t stepIndex{ 0 }; stepIndex < m_history.size(); ++stepIndex)
    {
        const SimState& state{ m_history[stepIndex] };
        if (state.callStack.empty())
        {
            continue;
        }
        const SimFrame& callFrame{ state.callStack.back() };
        out << "  step " << std::setw(4) << stepIndex << "  fn=" << callFrame.functionName
            << " block=" << callFrame.block.value << " pc=" << callFrame.pc << '\n';
    }
    out << "  total steps: " << m_stepCount << '\n';
    if (!m_trace.empty())
    {
        out << "  return value: ";
        std::visit(
            compat::Overloaded{
                [&out](int64_t value) { out << value; },
                [&out](uint64_t value) { out << value; },
                [&out](double value) { out << value; },
                [&out](bool value) { out << value; },
            },
            m_trace.back());
        out << '\n';
    }
}

const Simulator::FunctionCache* Simulator::findFunctionCache(std::string_view name) const noexcept
{
    const auto it{ m_functionCache.find(name) };
    return (it != m_functionCache.end()) ? &it->second : nullptr;
}

const IRBlock* Simulator::findBlock(const FunctionCache& cache, BlockId id) const noexcept
{
    const auto blockIt{ cache.blockIndexById.find(id) };
    if (blockIt == cache.blockIndexById.end() || blockIt->second >= cache.function->blocks.size())
    {
        return nullptr;
    }
    return &cache.function->blocks[blockIt->second];
}

Simulator::ValueResult Simulator::readValue(const SimFrame& frame, const IRFunction& function, ValueId valueId) const
{
    if (valueId.value < frame.values.size() && frame.values[valueId.value].has_value())
    {
        return *frame.values[valueId.value];
    }

    if (valueId.value < function.values.size())
    {
        const IRValue& irValue{ function.values[valueId.value] };
        if (irValue.constant)
        {
            return irConstantToRegValue(*irValue.constant);
        }
    }
    return compat::makeUnexpected(SimError::TypeMismatch);
}

void Simulator::writeValue(SimFrame& frame, const ValueId valueId, RegValue newValue)
{
    if (valueId.value >= frame.values.size())
    {
        frame.values.resize(valueId.value + 1U);
    }
    frame.values[valueId.value] = std::move(newValue);
}

Simulator::StepResult Simulator::stepOne()
{
    if (m_state.callStack.empty())
    {
        m_done = true;
        return false;
    }
    if (m_state.callStack.size() > m_maxCallDepth)
    {
        return compat::makeUnexpected(SimError::StackOverflow);
    }

    SimFrame& frame{ m_state.callStack.back() };
    const FunctionCache* functionCache{ findFunctionCache(frame.functionName) };
    if (!functionCache || !functionCache->function)
    {
        return compat::makeUnexpected(SimError::UnknownFunction);
    }
    const IRFunction& function{ *functionCache->function };

    const IRBlock* currentBlock{ findBlock(*functionCache, frame.block) };
    if (!currentBlock)
    {
        return compat::makeUnexpected(SimError::UnreachableInstruction);
    }

    // Block ended without a terminator — treat as implicit return.
    if (frame.pc >= currentBlock->instrs.size())
    {
        m_done = (m_state.callStack.size() == 1);
        if (!m_done)
        {
            m_state.callStack.pop_back();
        }
        return !m_done;
    }

    const IRInstr& instruction{ currentBlock->instrs[frame.pc] };
    ++frame.pc;

    switch (instruction.op)
    {
    case IROp::Const: return handleConstOp(frame, instruction);
    case IROp::Copy: return handleCopyOp(frame, instruction, function);
    case IROp::Add:
    case IROp::Sub:
    case IROp::Mul:
    case IROp::Div:
    case IROp::Mod:
    case IROp::And:
    case IROp::Or:
    case IROp::Xor:
    case IROp::Shl:
    case IROp::Shr: return handleBinaryOp(frame, instruction, function);
    case IROp::Neg:
    case IROp::Not: return handleUnaryOp(frame, instruction, function);
    case IROp::Cmp: return handleCmpOp(frame, instruction, function);
    case IROp::Jmp: return handleJumpOp(frame, instruction);
    case IROp::BrTrue: return handleBranchOp(frame, instruction, function);
    case IROp::Call: return handleCallOp(frame, instruction);
    case IROp::Ret: return handleRetOp(frame, instruction, function);
    case IROp::Load:
    case IROp::Store: break; // not implemented in basic simulation tier
    }
    return !m_done;
}

Simulator::StepResult Simulator::handleConstOp(SimFrame& frame, const IRInstr& instruction)
{
    if (instruction.output && instruction.constVal)
    {
        writeValue(frame, *instruction.output, irConstantToRegValue(*instruction.constVal));
    }
    return !m_done;
}

Simulator::StepResult Simulator::handleCopyOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    if (instruction.inputs.empty() || !instruction.output)
    {
        return !m_done;
    }
    auto readResult{ readValue(frame, function, instruction.inputs[0]) };
    if (!readResult)
    {
        return compat::makeUnexpected(readResult.error());
    }
    writeValue(frame, *instruction.output, *readResult);
    return !m_done;
}

Simulator::StepResult Simulator::handleBinaryOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    static constexpr std::size_t kBinaryInputCount{ 2 };
    if (instruction.inputs.size() < kBinaryInputCount || !instruction.output)
    {
        return !m_done;
    }

    auto leftResult{ readValue(frame, function, instruction.inputs[0]) };
    auto rightResult{ readValue(frame, function, instruction.inputs[1]) };
    if (!leftResult)
    {
        return compat::makeUnexpected(leftResult.error());
    }
    if (!rightResult)
    {
        return compat::makeUnexpected(rightResult.error());
    }
    if (isDivisionByZero(instruction.op, *rightResult))
    {
        return compat::makeUnexpected(SimError::DivisionByZero);
    }

    writeValue(frame, *instruction.output, applyBinaryOp(instruction.op, *leftResult, *rightResult));
    return !m_done;
}

Simulator::StepResult Simulator::handleUnaryOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    if (instruction.inputs.empty() || !instruction.output)
    {
        return !m_done;
    }

    auto operandResult{ readValue(frame, function, instruction.inputs[0]) };
    if (!operandResult)
    {
        return compat::makeUnexpected(operandResult.error());
    }

    RegValue unaryResult{};
    if (instruction.op == IROp::Neg)
    {
        unaryResult = std::visit(
            compat::Overloaded{
                [](int64_t value) noexcept -> RegValue { return static_cast<int64_t>(-value); },
                [](uint64_t value) noexcept -> RegValue { return static_cast<uint64_t>(-static_cast<int64_t>(value)); },
                [](double value) noexcept -> RegValue { return -value; },
                [](bool value) noexcept -> RegValue { return !value; },
            },
            *operandResult);
    }
    else // IROp::Not
    {
        unaryResult = std::visit(
            compat::Overloaded{
                [](int64_t value) noexcept -> RegValue { return ~value; },
                [](uint64_t value) noexcept -> RegValue { return ~value; },
                [](double) noexcept -> RegValue { return double{ 0 }; },
                [](bool value) noexcept -> RegValue { return !value; },
            },
            *operandResult);
    }
    writeValue(frame, *instruction.output, unaryResult);
    return !m_done;
}

Simulator::StepResult Simulator::handleCmpOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    static constexpr std::size_t kBinaryInputCount{ 2 };
    if (instruction.inputs.size() < kBinaryInputCount || !instruction.output || !instruction.cmpKind)
    {
        return !m_done;
    }

    auto leftResult{ readValue(frame, function, instruction.inputs[0]) };
    auto rightResult{ readValue(frame, function, instruction.inputs[1]) };
    if (!leftResult)
    {
        return compat::makeUnexpected(leftResult.error());
    }
    if (!rightResult)
    {
        return compat::makeUnexpected(rightResult.error());
    }

    writeValue(frame, *instruction.output, applyComparison(*instruction.cmpKind, *leftResult, *rightResult));
    return !m_done;
}

Simulator::StepResult Simulator::handleJumpOp(SimFrame& frame, const IRInstr& instruction)
{
    if (!instruction.trueTarget)
    {
        return !m_done;
    }
    frame.block = *instruction.trueTarget;
    frame.pc = 0;
    return !m_done;
}

Simulator::StepResult Simulator::handleBranchOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    if (instruction.inputs.empty())
    {
        return !m_done;
    }

    auto conditionResult{ readValue(frame, function, instruction.inputs[0]) };
    if (!conditionResult)
    {
        return compat::makeUnexpected(conditionResult.error());
    }

    const bool taken{ std::visit(
        compat::Overloaded{
            [](int64_t value) noexcept -> bool { return value != int64_t{ 0 }; },
            [](uint64_t value) noexcept -> bool { return value != uint64_t{ 0 }; },
            [](double value) noexcept -> bool { return value != 0.0; },
            [](bool value) noexcept -> bool { return value; },
        },
        *conditionResult) };

    if (taken && instruction.trueTarget)
    {
        frame.block = *instruction.trueTarget;
        frame.pc = 0;
    }
    else if (!taken && instruction.falseTarget)
    {
        frame.block = *instruction.falseTarget;
        frame.pc = 0;
    }
    return !m_done;
}

Simulator::StepResult Simulator::handleCallOp(SimFrame& /*frame*/, const IRInstr& instruction)
{
    if (!instruction.callee)
    {
        return !m_done;
    }

    const FunctionCache* calleeCache{ findFunctionCache(*instruction.callee) };
    if (!calleeCache || !calleeCache->function)
    {
        return compat::makeUnexpected(SimError::UnknownFunction);
    }
    const IRFunction& calleeFunction{ *calleeCache->function };

    SimFrame calleeFrame{};
    calleeFrame.functionName = calleeFunction.name;
    calleeFrame.values.resize(calleeFunction.values.size());
    calleeFrame.block = calleeFunction.entryBlock() ? calleeFunction.entryBlock()->id : BlockId{ 0 };
    calleeFrame.pc = 0;
    m_state.callStack.push_back(std::move(calleeFrame));
    return !m_done;
}

Simulator::StepResult Simulator::handleRetOp(SimFrame& frame, const IRInstr& instruction, const IRFunction& function)
{
    if (!instruction.inputs.empty())
    {
        auto returnResult{ readValue(frame, function, instruction.inputs[0]) };
        if (returnResult)
        {
            m_trace.push_back(*returnResult);
        }
    }
    m_state.callStack.pop_back();
    if (m_state.callStack.empty())
    {
        m_done = true;
    }
    return !m_done;
}

} // namespace asmstudio
