#include <asmstudio/codegen/Arm64Emitter.hpp>

#include <asmstudio/support/Compat.hpp>

#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

namespace asmstudio
{
namespace
{
// Register allocator — maps ValueId → ARM64 scratch register (x9-x15).
struct RegAlloc
{
    static constexpr std::array kScratchRegisters{ "x9", "x10", "x11", "x12", "x13", "x14", "x15" };

    std::unordered_map<uint32_t, std::string> registerMap{};
    std::set<std::string> usedRegisters{};
    std::size_t nextRegisterIndex{ 0 };

    std::string alloc(ValueId valueId)
    {
        auto [it, inserted]{ registerMap.try_emplace(valueId.value, std::string{}) };
        if (inserted)
        {
            it->second = (nextRegisterIndex < kScratchRegisters.size()) ? kScratchRegisters[nextRegisterIndex++] :
                                                                          "x" + std::to_string(16 + nextRegisterIndex++);
        }
        usedRegisters.insert(it->second);
        return it->second;
    }

    [[nodiscard]] std::string get(ValueId valueId) const
    {
        const auto it{ registerMap.find(valueId.value) };
        return (it != registerMap.end()) ? it->second : "x9";
    }
};

[[nodiscard]] std::string fmtImmediate(const IRConstant& constant)
{
    return std::visit(
        compat::Overloaded{
            [](const bool value) -> std::string { return value ? "#1" : "#0"; },
            [](const int64_t value) -> std::string { return "#" + std::to_string(value); },
            [](const uint64_t value) -> std::string { return "#" + std::to_string(value); },
            [](const double value) -> std::string { return "#" + std::to_string(value); },
        },
        constant);
}

[[nodiscard]] std::string_view cmpSuffix(CmpKind kind) noexcept
{
    switch (kind)
    {
    case CmpKind::Lt: return "lt";
    case CmpKind::Le: return "le";
    case CmpKind::Eq: return "eq";
    case CmpKind::Ne: return "ne";
    case CmpKind::Ge: return "ge";
    case CmpKind::Gt: return "gt";
    }
    return "ne";
}

[[nodiscard]] std::string_view arm64BinaryOpMnemonic(IROp opcode) noexcept
{
    switch (opcode)
    {
    case IROp::Add: return "add";
    case IROp::Sub: return "sub";
    case IROp::And: return "and";
    case IROp::Or: return "orr";
    case IROp::Xor: return "eor";
    case IROp::Shl: return "lsl";
    case IROp::Shr: return "asr";
    default: return "";
    }
}

// Build block-id → sequential-index map (used for GAS local label numbering).
[[nodiscard]] std::unordered_map<uint32_t, std::size_t> buildBlockIndexMap(const IRFunction& function)
{
    std::unordered_map<uint32_t, std::size_t> blockIndexMap{};
    for (std::size_t index{ 0 }; index < function.blocks.size(); ++index)
    {
        blockIndexMap[function.blocks[index].id.value] = index;
    }
    return blockIndexMap;
}

// Return "Nb" (backward) or "Nf" (forward) for a branch from currentBlockIndex → targetBlockIndex.
[[nodiscard]] std::string gasLabel(std::size_t targetBlockIndex, std::size_t currentBlockIndex)
{
    return (targetBlockIndex <= currentBlockIndex) ? std::to_string(targetBlockIndex) + "b" :
                                                     std::to_string(targetBlockIndex) + "f";
}

void emitInstruction(
    std::ostringstream& outputStream,
    const IRInstr& instruction,
    const std::unordered_map<uint32_t, std::size_t>& blockIndexMap,
    std::size_t currentBlockIndex,
    RegAlloc& registerAllocator,
    Arm64Platform platform)
{
    switch (instruction.op)
    {
    case IROp::Const: {
        if (!instruction.output || !instruction.constVal)
        {
            break;
        }
        outputStream << "    \"    mov " << registerAllocator.alloc(*instruction.output) << ", "
                     << fmtImmediate(*instruction.constVal) << "\\n\"\n";
        break;
    }
    case IROp::Copy: {
        if (!instruction.output || instruction.inputs.empty())
        {
            break;
        }
        outputStream << "    \"    mov " << registerAllocator.alloc(*instruction.output) << ", "
                     << registerAllocator.get(instruction.inputs[0]) << "\\n\"\n";
        break;
    }
    case IROp::Add:
    case IROp::Sub:
    case IROp::And:
    case IROp::Or:
    case IROp::Xor:
    case IROp::Shl:
    case IROp::Shr: {
        if (!instruction.output || instruction.inputs.size() < 2)
        {
            break;
        }
        outputStream << "    \"    " << arm64BinaryOpMnemonic(instruction.op) << " "
                     << registerAllocator.alloc(*instruction.output) << ", " << registerAllocator.get(instruction.inputs[0])
                     << ", " << registerAllocator.get(instruction.inputs[1]) << "\\n\"\n";
        break;
    }
    case IROp::Mul: {
        if (!instruction.output || instruction.inputs.size() < 2)
        {
            break;
        }
        outputStream << "    \"    mul " << registerAllocator.alloc(*instruction.output) << ", "
                     << registerAllocator.get(instruction.inputs[0]) << ", "
                     << registerAllocator.get(instruction.inputs[1]) << "\\n\"\n";
        break;
    }
    case IROp::Div: {
        if (!instruction.output || instruction.inputs.size() < 2)
        {
            break;
        }
        outputStream << "    \"    sdiv " << registerAllocator.alloc(*instruction.output) << ", "
                     << registerAllocator.get(instruction.inputs[0]) << ", "
                     << registerAllocator.get(instruction.inputs[1]) << "\\n\"\n";
        break;
    }
    case IROp::Mod: {
        // sdiv dest, lhs, rhs  then  msub dest, dest, rhs, lhs  ≡  lhs - (lhs/rhs)*rhs
        if (!instruction.output || instruction.inputs.size() < 2)
        {
            break;
        }
        const std::string destRegister{ registerAllocator.alloc(*instruction.output) };
        const std::string leftRegister{ registerAllocator.get(instruction.inputs[0]) };
        const std::string rightRegister{ registerAllocator.get(instruction.inputs[1]) };
        outputStream << "    \"    sdiv " << destRegister << ", " << leftRegister << ", " << rightRegister << "\\n\"\n";
        outputStream << "    \"    msub " << destRegister << ", " << destRegister << ", " << rightRegister << ", "
                     << leftRegister << "\\n\"\n";
        break;
    }
    case IROp::Neg: {
        if (!instruction.output || instruction.inputs.empty())
        {
            break;
        }
        outputStream << "    \"    neg " << registerAllocator.alloc(*instruction.output) << ", "
                     << registerAllocator.get(instruction.inputs[0]) << "\\n\"\n";
        break;
    }
    case IROp::Not: {
        if (!instruction.output || instruction.inputs.empty())
        {
            break;
        }
        outputStream << "    \"    mvn " << registerAllocator.alloc(*instruction.output) << ", "
                     << registerAllocator.get(instruction.inputs[0]) << "\\n\"\n";
        break;
    }
    case IROp::Cmp: {
        if (!instruction.output || instruction.inputs.size() < 2 || !instruction.cmpKind)
        {
            break;
        }
        const std::string destRegister{ registerAllocator.alloc(*instruction.output) };
        outputStream << "    \"    cmp " << registerAllocator.get(instruction.inputs[0]) << ", "
                     << registerAllocator.get(instruction.inputs[1]) << "\\n\"\n";
        outputStream << "    \"    cset " << destRegister << ", " << cmpSuffix(*instruction.cmpKind) << "\\n\"\n";
        break;
    }
    case IROp::Jmp: {
        if (!instruction.trueTarget)
        {
            break;
        }
        outputStream << "    \"    b " << gasLabel(blockIndexMap.at(instruction.trueTarget->value), currentBlockIndex) << "\\n\"\n";
        break;
    }
    case IROp::BrTrue: {
        if (instruction.inputs.empty())
        {
            break;
        }
        const std::string conditionRegister{ registerAllocator.get(instruction.inputs[0]) };
        if (instruction.trueTarget)
        {
            outputStream << "    \"    cbnz " << conditionRegister << ", "
                         << gasLabel(blockIndexMap.at(instruction.trueTarget->value), currentBlockIndex) << "\\n\"\n";
        }
        if (instruction.falseTarget)
        {
            outputStream << "    \"    b "
                         << gasLabel(blockIndexMap.at(instruction.falseTarget->value), currentBlockIndex) << "\\n\"\n";
        }
        break;
    }
    case IROp::Call: {
        if (!instruction.callee)
        {
            break;
        }
        const std::string_view calleePrefix{ (platform == Arm64Platform::macOS) ? "_" : "" };
        outputStream << "    \"    bl " << calleePrefix << *instruction.callee << "\\n\"\n";
        if (instruction.output)
        {
            const std::string destRegister{ registerAllocator.alloc(*instruction.output) };
            if (destRegister != "x0")
            {
                outputStream << "    \"    mov " << destRegister << ", x0\\n\"\n";
            }
        }
        break;
    }
    case IROp::Ret: {
        if (!instruction.inputs.empty())
        {
            const std::string returnRegister{ registerAllocator.get(instruction.inputs[0]) };
            if (returnRegister != "x0")
            {
                outputStream << "    \"    mov x0, " << returnRegister << "\\n\"\n";
            }
        }
        outputStream << "    \"    ret\\n\"\n";
        break;
    }
    case IROp::Load:
    case IROp::Store: break; // not implemented in this tier
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

std::string emitArm64AsmVolatile(const IRFunction& function, Arm64Platform platform)
{
    RegAlloc registerAllocator{};
    auto blockIndexMap{ buildBlockIndexMap(function) };
    std::ostringstream outputStream{};

    const std::string_view platformName{ (platform == Arm64Platform::macOS)     ? "macOS" :
                                             (platform == Arm64Platform::Linux) ? "Linux" :
                                                                                  "Windows" };

    outputStream << "// ARM64 (" << platformName << ") — fn: " << function.name << "\n";
    outputStream << "asm volatile(\n";
    outputStream << "    \"// fn: " << function.name << " [" << platformName << "]\\n\"\n";

    for (std::size_t blockIndex{ 0 }; blockIndex < function.blocks.size(); ++blockIndex)
    {
        const IRBlock& block{ function.blocks[blockIndex] };
        outputStream << "    \"" << blockIndex << ":  // " << block.name << "\\n\"\n";
        for (const auto& instruction : block.instrs)
        {
            emitInstruction(outputStream, instruction, blockIndexMap, blockIndex, registerAllocator, platform);
        }
    }

    // Clobber list: all allocated registers + x0, cc, memory.
    std::set<std::string> clobbers(registerAllocator.usedRegisters.begin(), registerAllocator.usedRegisters.end());
    clobbers.insert("x0");
    clobbers.insert("cc");
    clobbers.insert("memory");

    outputStream << "    :\n    :\n    : ";
    bool isFirstClobber{ true };
    for (const auto& clobber : clobbers)
    {
        if (!isFirstClobber)
        {
            outputStream << ", ";
        }
        outputStream << "\"" << clobber << "\"";
        isFirstClobber = false;
    }
    outputStream << "\n);\n";

    return outputStream.str();
}

std::string emitArm64AsmVolatile(const IRModule& module, Arm64Platform platform)
{
    std::ostringstream outputStream{};
    outputStream << "// ARM64 asm volatile — module: " << module.name << "\n\n";
    for (const auto& function : module.functions)
    {
        outputStream << emitArm64AsmVolatile(function, platform) << '\n';
    }
    return outputStream.str();
}

} // namespace asmstudio
