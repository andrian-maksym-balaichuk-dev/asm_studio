#include <asmstudio/codegen/PseudoEmitter.hpp>

#include <asmstudio/support/Compat.hpp>

#include <sstream>
#include <string>

namespace asmstudio
{
namespace
{
[[nodiscard]] std::string fmtValue(const ValueId valueId)
{
    return "v" + std::to_string(valueId.value);
}

[[nodiscard]] std::string fmtBlock(const BlockId blockId)
{
    return ".blk_" + std::to_string(blockId.value);
}

[[nodiscard]] std::string fmtConstant(const IRConstant& constant)
{
    return std::visit(
        compat::Overloaded{
            [](const bool value) -> std::string { return value ? "true" : "false"; },
            [](const int64_t value) -> std::string { return std::to_string(value); },
            [](const uint64_t value) -> std::string { return std::to_string(value); },
            [](const double value) -> std::string { return std::to_string(value); },
        },
        constant);
}

void emitInstruction(std::ostringstream& outputStream, const IRInstr& instruction, const IRFunction& function)
{
    outputStream << "    ";
    if (instruction.output)
    {
        outputStream << fmtValue(*instruction.output) << " = ";
    }

    outputStream << instruction.opName();

    if (instruction.cmpKind)
    {
        outputStream << '.' << cmpKindName(*instruction.cmpKind);
    }

    if (instruction.constVal)
    {
        outputStream << ' ' << fmtConstant(*instruction.constVal);
    }

    if (instruction.callee)
    {
        outputStream << ' ' << *instruction.callee;
    }

    for (const auto inputValueId : instruction.inputs)
    {
        outputStream << ' ' << fmtValue(inputValueId);
    }

    if (instruction.trueTarget)
    {
        outputStream << ' ' << fmtBlock(*instruction.trueTarget);
    }
    if (instruction.falseTarget)
    {
        outputStream << " else " << fmtBlock(*instruction.falseTarget);
    }

    if (instruction.output && instruction.output->value < function.values.size())
    {
        outputStream << "   ; " << dataTypeName(function.values[instruction.output->value].type);
    }

    outputStream << '\n';
}
} // namespace

std::string emitPseudoAsm(const IRFunction& function)
{
    std::ostringstream outputStream{};
    outputStream << function.name << ":\n";

    for (const auto& [name, id, instrs] : function.blocks)
    {
        outputStream << fmtBlock(id) << ": ; " << name << '\n';
        for (const auto& instruction : instrs)
        {
            emitInstruction(outputStream, instruction, function);
        }
    }
    return outputStream.str();
}

std::string emitPseudoAsm(const IRModule& module)
{
    std::ostringstream outputStream{};
    outputStream << "; Module: " << module.name << "\n\n";

    for (const auto& function : module.functions)
    {
        outputStream << emitPseudoAsm(function) << '\n';
    }

    return outputStream.str();
}

} // namespace asmstudio
