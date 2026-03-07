#include <asmstudio/backend/PseudoEmitter.hpp>

#include <format>
#include <sstream>

namespace asmstudio
{
namespace
{
std::string fmtValue(ValueId id)
{
    return std::format("v{}", id.value);
}
std::string fmtBlock(BlockId id)
{
    return std::format(".blk_{}", id.value);
}

std::string fmtConstant(const IRConstant& c)
{
    return std::visit(
        []<typename T>(T v) -> std::string {
            if constexpr (std::is_same_v<T, bool>)
            {
                return v ? "true" : "false";
            }
            else
            {
                return std::format("{}", v);
            }
        },
        c);
}

std::string fmtOpcode(const IROp op)
{
    switch (op)
    {
    case IROp::Const: return "const";
    case IROp::Copy: return "copy";
    case IROp::Add: return "add";
    case IROp::Sub: return "sub";
    case IROp::Mul: return "mul";
    case IROp::Div: return "div";
    case IROp::Mod: return "mod";
    case IROp::And: return "and";
    case IROp::Or: return "or";
    case IROp::Xor: return "xor";
    case IROp::Shl: return "shl";
    case IROp::Shr: return "shr";
    case IROp::Neg: return "neg";
    case IROp::Not: return "not";
    case IROp::Cmp: return "cmp";
    case IROp::Jmp: return "jmp";
    case IROp::BrTrue: return "brtrue";
    case IROp::Call: return "call";
    case IROp::Ret: return "ret";
    case IROp::Load: return "load";
    case IROp::Store: return "store";
    }
    return "?";
}

void emitInstr(std::ostringstream& out, const IRInstr& instr, const IRFunction& fn)
{
    out << "    ";
    if (instr.output)
    {
        out << fmtValue(*instr.output) << " = ";
    }

    out << fmtOpcode(instr.op);

    if (instr.cmpKind)
    {
        out << '.' << cmpKindName(*instr.cmpKind);
    }

    if (instr.constVal)
    {
        out << ' ' << fmtConstant(*instr.constVal);
    }

    if (instr.callee)
    {
        out << ' ' << *instr.callee;
    }

    for (const auto& inp : instr.inputs)
    {
        out << ' ' << fmtValue(inp);
    }

    if (instr.trueTarget)
    {
        out << ' ' << fmtBlock(*instr.trueTarget);
    }
    if (instr.falseTarget)
    {
        out << " else " << fmtBlock(*instr.falseTarget);
    }

    // Type annotation.
    if (instr.output && instr.output->value < fn.values.size())
    {
        out << "   ; " << dataTypeName(fn.values[instr.output->value].type);
    }

    out << '\n';
}
} // namespace

std::string emitPseudoAsm(const IRFunction& fn)
{
    std::ostringstream out;
    out << fn.name << ":\n";

    for (const auto& [name, id, instrs] : fn.blocks)
    {
        out << fmtBlock(id) << ": ; " << name << '\n';
        for (const auto& instr : instrs)
        {
            emitInstr(out, instr, fn);
        }
    }
    return out.str();
}

std::string emitPseudoAsm(const IRModule& module)
{
    std::ostringstream out;
    out << "; Module: " << module.name << "\n\n";

    for (const auto& fn : module.functions)
    {
        out << emitPseudoAsm(fn) << '\n';
    }

    return out.str();
}

} // namespace asmstudio
