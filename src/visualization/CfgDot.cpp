#include <asmstudio/visualization/CfgDot.hpp>

#include <sstream>
#include <string>

namespace asmstudio
{

namespace
{

[[nodiscard]] std::string blockNodeId(std::string_view functionName, BlockId blockId)
{
    return std::string(functionName) + "_blk_" + std::to_string(blockId.value);
}

[[nodiscard]] std::string escapeDotLabel(std::string_view text)
{
    std::string escaped{};
    for (const char character : text)
    {
        if (character == '"')
        {
            escaped += "\\\"";
        }
        else
        {
            escaped += character;
        }
    }
    return escaped;
}

[[nodiscard]] std::string buildBlockLabel(const IRBlock& block)
{
    std::ostringstream labelStream{};
    labelStream << block.name << "\\l";
    for (const auto& instruction : block.instrs)
    {
        if (instruction.output)
        {
            labelStream << "v" << instruction.output->value << " = ";
        }
        labelStream << instruction.opName();
        labelStream << "\\l";
    }
    return escapeDotLabel(labelStream.str());
}

void emitFunctionSubgraph(std::ostringstream& outputStream, const IRFunction& function)
{
    outputStream << "  subgraph cluster_" << function.name << " {\n";
    outputStream << "    label=\"" << function.name << "\";\n";
    outputStream << "    style=filled; fillcolor=lightgrey;\n\n";

    for (const auto& block : function.blocks)
    {
        const std::string nodeIdentifier{ blockNodeId(function.name, block.id) };
        outputStream << "    \"" << nodeIdentifier << "\" [shape=box, style=filled, fillcolor=white, "
                     << "label=\"" << buildBlockLabel(block) << "\", align=left];\n";
    }
    outputStream << "\n";

    for (const auto& block : function.blocks)
    {
        const std::string sourceNodeId{ blockNodeId(function.name, block.id) };
        for (const auto& instruction : block.instrs)
        {
            if (instruction.trueTarget)
            {
                const std::string destNodeId{ blockNodeId(function.name, *instruction.trueTarget) };
                const std::string edgeLabel{ (instruction.op == IROp::BrTrue) ? "T" : "" };
                outputStream << "    \"" << sourceNodeId << "\" -> \"" << destNodeId << "\"";
                if (!edgeLabel.empty())
                {
                    outputStream << " [label=\"" << edgeLabel << "\"]";
                }
                outputStream << ";\n";
            }
            if (instruction.falseTarget)
            {
                const std::string destNodeId{ blockNodeId(function.name, *instruction.falseTarget) };
                outputStream << "    \"" << sourceNodeId << "\" -> \"" << destNodeId << "\" [label=\"F\"];\n";
            }
        }
    }
    outputStream << "  }\n";
}
} // namespace

std::string toDot(const IRFunction& function)
{
    std::ostringstream outputStream{};
    outputStream << "digraph " << function.name << " {\n";
    outputStream << "  rankdir=TB;\n";
    emitFunctionSubgraph(outputStream, function);
    outputStream << "}\n";
    return outputStream.str();
}

std::string toDot(const IRModule& module)
{
    std::ostringstream outputStream{};
    outputStream << "digraph " << module.name << " {\n";
    outputStream << "  rankdir=TB;\n";
    for (const auto& function : module.functions)
    {
        emitFunctionSubgraph(outputStream, function);
    }
    outputStream << "}\n";
    return outputStream.str();
}
} // namespace asmstudio
