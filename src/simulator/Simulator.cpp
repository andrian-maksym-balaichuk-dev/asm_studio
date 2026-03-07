#include <asmstudio/simulator/Simulator.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <print>
#include <ranges>
#include <stdexcept>

namespace asmstudio
{
namespace
{
RegValue applyBinOp(IROp op, RegValue lhs, RegValue rhs)
{
    return std::visit(
        [op]<typename L, typename R>(L l, R r) -> RegValue {
            using C = std::common_type_t<L, R>;
            C lc = static_cast<C>(l);
            C rc = static_cast<C>(r);
            switch (op)
            {
            case IROp::Add: return static_cast<L>(lc + rc);
            case IROp::Sub: return static_cast<L>(lc - rc);
            case IROp::Mul: return static_cast<L>(lc * rc);
            case IROp::Div:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc / rc); // caller checks zero
                }
                else
                {
                    return static_cast<L>(lc / rc);
                }
            case IROp::Mod:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc % rc);
                }
                else
                {
                    return static_cast<L>(std::fmod(static_cast<double>(lc), static_cast<double>(rc)));
                }
            case IROp::And:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc & rc);
                }
                else
                {
                    return l;
                }
            case IROp::Or:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc | rc);
                }
                else
                {
                    return l;
                }
            case IROp::Xor:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc ^ rc);
                }
                else
                {
                    return l;
                }
            case IROp::Shl:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc << static_cast<int>(rc));
                }
                else
                {
                    return l;
                }
            case IROp::Shr:
                if constexpr (std::is_integral_v<C>)
                {
                    return static_cast<L>(lc >> static_cast<int>(rc));
                }
                else
                {
                    return l;
                }
            default: return l;
            }
        },
        lhs, rhs);
}

bool applyBinOpIsDivZero(IROp op, RegValue rhs)
{
    if (op != IROp::Div && op != IROp::Mod)
    {
        return false;
    }

    return std::visit(
        []<typename T>(T v) -> bool {
            if constexpr (std::is_integral_v<T>)
            {
                return v == T{ 0 };
            }
            return false;
        },
        rhs);
}

bool applyCmp(CmpKind kind, RegValue lhs, RegValue rhs)
{
    return std::visit(
        [kind]<typename L, typename R>(L l, R r) -> bool {
            using C = std::common_type_t<L, R>;
            C lc = static_cast<C>(l);
            C rc = static_cast<C>(r);
            switch (kind)
            {
            case CmpKind::Lt: return lc < rc;
            case CmpKind::Le: return lc <= rc;
            case CmpKind::Eq: return lc == rc;
            case CmpKind::Ne: return lc != rc;
            case CmpKind::Ge: return lc >= rc;
            case CmpKind::Gt: return lc > rc;
            }
            return false;
        },
        lhs, rhs);
}

RegValue irConstantToRegValue(const IRConstant& c)
{
    return std::visit([]<typename T>(T v) -> RegValue { return v; }, c);
}

} // namespace

// ---------------------------------------------------------------------------
// Simulator implementation.
// ---------------------------------------------------------------------------

Simulator::Simulator(const IRModule& module, std::size_t maxSteps, std::size_t maxCallDepth)
: m_module(module), m_maxSteps(maxSteps), m_maxCallDepth(maxCallDepth)
{}

const IRFunction* Simulator::findFunction(std::string_view name) const noexcept
{
    for (const auto& fn : m_module.functions)
    {
        if (fn.name == name)
        {
            return &fn;
        }
    }
    return nullptr;
}

void Simulator::reset()
{
    m_state = SimState{};
    m_history.clear();
    m_trace.clear();
    m_done = true;
    m_stepCount = 0;
}

std::expected<std::optional<RegValue>, SimError> Simulator::run(std::string_view functionName)
{
    const IRFunction* fn = findFunction(functionName);
    if (!fn)
    {
        return std::unexpected(SimError::UnknownFunction);
    }

    reset();
    m_done = false;

    SimFrame frame;
    frame.functionName = std::string(functionName);
    frame.block = fn->blocks.empty() ? BlockId{ 0 } : fn->blocks[0].id;
    frame.pc = 0;
    m_state.callStack.push_back(std::move(frame));

    while (!m_done)
    {
        m_history.push_back(m_state);
        auto result = stepOne();
        if (!result)
        {
            return std::unexpected(result.error());
        }

        ++m_stepCount;
        if (m_stepCount > m_maxSteps)
        {
            return std::unexpected(SimError::MaxStepsExceeded);
        }
    }

    if (m_trace.empty())
    {
        return std::optional<RegValue>{};
    }

    return std::optional<RegValue>{ m_trace.back() };
}

std::expected<bool, SimError> Simulator::step()
{
    if (m_done)
    {
        return false;
    }

    m_history.push_back(m_state);
    return stepOne();
}

std::expected<bool, SimError> Simulator::stepOne()
{
    if (m_state.callStack.empty())
    {
        m_done = true;
        return false;
    }
    if (m_state.callStack.size() > m_maxCallDepth)
    {
        return std::unexpected(SimError::StackOverflow);
    }

    SimFrame& frame = m_state.callStack.back();
    const IRFunction* fn = findFunction(frame.functionName);
    if (!fn)
    {
        return std::unexpected(SimError::UnknownFunction);
    }

    // Find current block.
    const IRBlock* blk = nullptr;
    for (const auto& b : fn->blocks)
        if (b.id == frame.block)
        {
            blk = &b;
            break;
        }
    if (!blk)
    {
        return std::unexpected(SimError::UnreachableInstruction);
    }

    if (frame.pc >= blk->instrs.size())
    {
        // Block ended without a terminator — treat as implicit Ret.
        m_done = m_state.callStack.size() == 1;
        if (!m_done)
        {
            m_state.callStack.pop_back();
        }
        return !m_done;
    }

    const IRInstr& instr = blk->instrs[frame.pc];
    ++frame.pc;

    auto readValue = [&](ValueId id) -> std::expected<RegValue, SimError> {
        auto it = frame.values.find(id);
        if (it != frame.values.end())
        {
            return it->second;
        }

        // Check function-level constants.
        if (id.value < fn->values.size())
        {
            const IRValue& val = fn->values[id.value];
            if (val.constant)
            {
                return irConstantToRegValue(*val.constant);
            }
        }
        return std::unexpected(SimError::TypeMismatch);
    };

    auto writeValue = [&](ValueId id, RegValue val) { frame.values[id] = val; };

    switch (instr.op)
    {
    case IROp::Const: {
        if (instr.output && instr.constVal)
        {
            writeValue(*instr.output, irConstantToRegValue(*instr.constVal));
        }
        break;
    }
    case IROp::Copy: {
        if (instr.inputs.empty() || !instr.output)
        {
            break;
        }

        auto v = readValue(instr.inputs[0]);
        if (!v)
        {
            return std::unexpected(v.error());
        }
        writeValue(*instr.output, *v);
        break;
    }
    case IROp::Add:
    case IROp::Sub:
    case IROp::Mul:
    case IROp::Div:
    case IROp::Mod:
    case IROp::And:
    case IROp::Or:
    case IROp::Xor:
    case IROp::Shl:
    case IROp::Shr: {
        if (instr.inputs.size() < 2 || !instr.output)
        {
            break;
        }
        auto lhs = readValue(instr.inputs[0]);
        auto rhs = readValue(instr.inputs[1]);
        if (!lhs)
        {
            return std::unexpected(lhs.error());
        }
        if (!rhs)
        {
            return std::unexpected(rhs.error());
        }
        if (applyBinOpIsDivZero(instr.op, *rhs))
        {
            return std::unexpected(SimError::DivisionByZero);
        }
        writeValue(*instr.output, applyBinOp(instr.op, *lhs, *rhs));
        break;
    }
    case IROp::Neg: {
        if (instr.inputs.empty() || !instr.output)
        {
            break;
        }
        auto v = readValue(instr.inputs[0]);
        if (!v)
        {
            return std::unexpected(v.error());
        }
        RegValue neg = std::visit([]<typename T>(T x) -> RegValue { return static_cast<T>(-x); }, *v);
        writeValue(*instr.output, neg);
        break;
    }
    case IROp::Not: {
        if (instr.inputs.empty() || !instr.output)
        {
            break;
        }

        auto v = readValue(instr.inputs[0]);
        if (!v)
        {
            return std::unexpected(v.error());
        }
        RegValue notVal = std::visit(
            []<typename T>(T x) -> RegValue {
                if constexpr (std::is_same_v<T, bool>)
                {
                    return !x;
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    return static_cast<T>(0); // bitwise NOT undefined for float
                }
                else
                {
                    return static_cast<T>(~x);
                }
            },
            *v);
        writeValue(*instr.output, notVal);
        break;
    }
    case IROp::Cmp: {
        if (instr.inputs.size() < 2 || !instr.output || !instr.cmpKind)
        {
            break;
        }
        auto lhs = readValue(instr.inputs[0]);
        auto rhs = readValue(instr.inputs[1]);
        if (!lhs)
        {
            return std::unexpected(lhs.error());
        }
        if (!rhs)
        {
            return std::unexpected(rhs.error());
        }
        writeValue(*instr.output, applyCmp(*instr.cmpKind, *lhs, *rhs));
        break;
    }
    case IROp::Jmp: {
        if (!instr.trueTarget)
        {
            break;
        }
        frame.block = *instr.trueTarget;
        frame.pc = 0;
        break;
    }
    case IROp::BrTrue: {
        if (instr.inputs.empty())
        {
            break;
        }
        auto cond = readValue(instr.inputs[0]);
        if (!cond)
        {
            return std::unexpected(cond.error());
        }
        bool taken = std::visit([]<typename T>(T v) -> bool { return static_cast<bool>(v); }, *cond);
        if (taken && instr.trueTarget)
        {
            frame.block = *instr.trueTarget;
            frame.pc = 0;
        }
        else if (!taken && instr.falseTarget)
        {
            frame.block = *instr.falseTarget;
            frame.pc = 0;
        }
        break;
    }
    case IROp::Call: {
        if (!instr.callee)
        {
            break;
        }
        const IRFunction* callee = findFunction(*instr.callee);
        if (!callee)
        {
            return std::unexpected(SimError::UnknownFunction);
        }
        SimFrame newFrame;
        newFrame.functionName = *instr.callee;
        newFrame.block = callee->blocks.empty() ? BlockId{ 0 } : callee->blocks[0].id;
        newFrame.pc = 0;
        m_state.callStack.push_back(std::move(newFrame));
        break;
    }
    case IROp::Ret: {
        if (!instr.inputs.empty())
        {
            auto v = readValue(instr.inputs[0]);
            if (v)
            {
                m_trace.push_back(*v);
            }
        }
        m_state.callStack.pop_back();
        if (m_state.callStack.empty())
        {
            m_done = true;
        }
        break;
    }
    case IROp::Load:
    case IROp::Store:
        // Not implemented in this simulation tier.
        break;
    }

    return !m_done;
}

void Simulator::rewind(std::size_t steps)
{
    steps = std::min(steps, m_history.size());
    for (std::size_t i = 0; i < steps; ++i)
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
    for (std::size_t i = 0; i < m_history.size(); ++i)
    {
        const SimState& s = m_history[i];
        if (s.callStack.empty())
        {
            continue;
        }
        const SimFrame& f = s.callStack.back();
        out << std::format("  step {:4d}  fn={} block={} pc={}\n", i, f.functionName, f.block.value, f.pc);
    }
    out << "  total steps: " << m_stepCount << "\n";
    if (!m_trace.empty())
    {
        out << "  return value: ";
        std::visit([&out]<typename T>(T v) { out << v; }, m_trace.back());
        out << "\n";
    }
}
} // namespace asmstudio
