#include <asmstudio/visualization/CfgDot.hpp>

#include <format>
#include <sstream>

namespace asmstudio
{

namespace
{

std::string nodeId(std::string_view fnName, BlockId id)
{
    return std::format("{}_blk_{}", fnName, id.value);
}

std::string escapeDot(std::string_view s)
{
    std::string out;
    for (char c : s)
    {
        if (c == '"')
        {
            out += "\\\"";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

std::string blockLabel(const IRBlock& blk)
{
    std::ostringstream oss;
    oss << blk.name << "\\l";
    for (const auto& instr : blk.instrs)
    {
        if (instr.output)
        {
            oss << "v" << instr.output->value << " = ";
        }
        switch (instr.op)
        {
        case IROp::Const: oss << "const"; break;
        case IROp::Copy: oss << "copy"; break;
        case IROp::Add: oss << "add"; break;
        case IROp::Sub: oss << "sub"; break;
        case IROp::Mul: oss << "mul"; break;
        case IROp::Div: oss << "div"; break;
        case IROp::Cmp: oss << "cmp"; break;
        case IROp::Jmp: oss << "jmp"; break;
        case IROp::BrTrue: oss << "brtrue"; break;
        case IROp::Ret: oss << "ret"; break;
        case IROp::Call: oss << "call"; break;
        default: oss << "..."; break;
        }
        oss << "\\l";
    }
    return escapeDot(oss.str());
}

void emitFunctionDot(std::ostringstream& out, const IRFunction& fn)
{
    out << "  subgraph cluster_" << fn.name << " {\n";
    out << "    label=\"" << fn.name << "\";\n";
    out << "    style=filled; fillcolor=lightgrey;\n\n";

    for (const auto& blk : fn.blocks)
    {
        std::string nid = nodeId(fn.name, blk.id);
        out << "    \"" << nid << "\" [shape=box, style=filled, fillcolor=white, "
            << "label=\"" << blockLabel(blk) << "\", align=left];\n";
    }
    out << "\n";

    for (const auto& blk : fn.blocks)
    {
        std::string src = nodeId(fn.name, blk.id);
        for (const auto& instr : blk.instrs)
        {
            if (instr.trueTarget)
            {
                std::string dst = nodeId(fn.name, *instr.trueTarget);
                std::string lbl = (instr.op == IROp::BrTrue) ? "T" : "";
                out << "    \"" << src << "\" -> \"" << dst << "\"";
                if (!lbl.empty())
                    out << " [label=\"" << lbl << "\"]";
                out << ";\n";
            }
            if (instr.falseTarget)
            {
                std::string dst = nodeId(fn.name, *instr.falseTarget);
                out << "    \"" << src << "\" -> \"" << dst << "\" [label=\"F\"];\n";
            }
        }
    }
    out << "  }\n";
}
} // namespace

std::string toDot(const IRFunction& fn)
{
    std::ostringstream out;
    out << "digraph " << fn.name << " {\n";
    out << "  rankdir=TB;\n";
    emitFunctionDot(out, fn);
    out << "}\n";
    return out.str();
}

std::string toDot(const IRModule& module)
{
    std::ostringstream out;
    out << "digraph " << module.name << " {\n";
    out << "  rankdir=TB;\n";
    for (const auto& fn : module.functions)
    {
        emitFunctionDot(out, fn);
    }
    out << "}\n";
    return out.str();
}
} // namespace asmstudio
