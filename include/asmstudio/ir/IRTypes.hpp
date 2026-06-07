#ifndef ASMSTUDIO_IR_IRTYPES_HPP
#define ASMSTUDIO_IR_IRTYPES_HPP


#include <asmstudio/support/Types.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace asmstudio
{
using IRConstant = std::variant<int64_t, std::uint64_t, double, bool>;

enum class IROp
{
    Const, // output = constant
    Copy,  // output = input[0]
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Shl,
    Shr,
    Neg,
    Not,
    Cmp,    // output = bool; cmpKind set; inputs = [lhs, rhs]
    Jmp,    // unconditional; trueTarget set
    BrTrue, // inputs = [cond]; trueTarget = then, falseTarget = else
    Call,   // callee set; inputs = args; output = return value (optional)
    Ret,    // inputs = [value] (optional)
    Load,   // inputs = [addr]; output = value
    Store,  // inputs = [addr, value]
};

[[nodiscard]] constexpr std::string_view irOpName(IROp opcode) noexcept
{
    switch (opcode)
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

[[nodiscard]] constexpr bool isArithmeticOp(IROp opcode) noexcept
{
    switch (opcode)
    {
    case IROp::Add:
    case IROp::Sub:
    case IROp::Mul:
    case IROp::Div:
    case IROp::Mod:
    case IROp::Neg:
    case IROp::And:
    case IROp::Or:
    case IROp::Xor:
    case IROp::Shl:
    case IROp::Shr: return true;
    default: return false;
    }
}

struct IRValue
{
    DataType type{ DataType::Void };
    std::optional<IRConstant> constant; // set only for Const instructions
};

struct IRInstr
{
    IROp op{ IROp::Const };
    std::vector<ValueId> inputs;
    std::optional<ValueId> output;
    std::optional<BlockId> trueTarget;
    std::optional<BlockId> falseTarget;
    std::optional<std::string> callee;  // for Call
    std::optional<CmpKind> cmpKind;     // for Cmp
    std::optional<IRConstant> constVal; // for Const — mirrors IRValue::constant

    [[nodiscard]] constexpr std::string_view opName() const noexcept
    {
        return irOpName(op);
    }

    [[nodiscard]] constexpr bool isArithmetic() const noexcept
    {
        return isArithmeticOp(op);
    }

    [[nodiscard]] constexpr bool isTerminator() const noexcept
    {
        return op == IROp::Jmp || op == IROp::BrTrue || op == IROp::Ret;
    }
};

struct IRBlock
{
    std::string name;
    BlockId id;
    std::vector<IRInstr> instrs;

    [[nodiscard]] bool empty() const noexcept
    {
        return instrs.empty();
    }
};

struct IRFunction
{
    std::string name;
    std::vector<IRBlock> blocks;
    std::vector<IRValue> values; // indexed by ValueId::value
    std::unordered_map<std::uint32_t, std::size_t> blockIndex; // blockId.value → index in blocks

    [[nodiscard]] const IRBlock* entryBlock() const noexcept
    {
        return blocks.empty() ? nullptr : &blocks.front();
    }

    [[nodiscard]] IRBlock* entryBlock() noexcept
    {
        return blocks.empty() ? nullptr : &blocks.front();
    }

    [[nodiscard]] const IRBlock* findBlock(BlockId blockId) const noexcept
    {
        if (!blockIndex.empty())
        {
            const auto it{ blockIndex.find(blockId.value) };
            if (it == blockIndex.end() || it->second >= blocks.size())
            {
                return nullptr;
            }
            return &blocks[it->second];
        }
        for (const auto& block : blocks)
        {
            if (block.id == blockId)
            {
                return &block;
            }
        }
        return nullptr;
    }

    [[nodiscard]] IRBlock* findBlock(BlockId blockId) noexcept
    {
        if (!blockIndex.empty())
        {
            const auto it{ blockIndex.find(blockId.value) };
            if (it == blockIndex.end() || it->second >= blocks.size())
            {
                return nullptr;
            }
            return &blocks[it->second];
        }
        for (auto& block : blocks)
        {
            if (block.id == blockId)
            {
                return &block;
            }
        }
        return nullptr;
    }

    void rebuildBlockIndex()
    {
        blockIndex.clear();
        blockIndex.reserve(blocks.size());
        for (std::size_t i{ 0 }; i < blocks.size(); ++i)
        {
            blockIndex.emplace(blocks[i].id.value, i);
        }
    }
};

struct IRModule
{
    std::string name;
    std::vector<IRFunction> functions;

    [[nodiscard]] const IRFunction* findFunction(std::string_view functionName) const noexcept
    {
        for (const auto& function : functions)
        {
            if (function.name == functionName)
            {
                return &function;
            }
        }
        return nullptr;
    }

    [[nodiscard]] IRFunction* findFunction(std::string_view functionName) noexcept
    {
        for (auto& function : functions)
        {
            if (function.name == functionName)
            {
                return &function;
            }
        }
        return nullptr;
    }
};

struct ValueIdHash
{
    [[nodiscard]] std::size_t operator()(const ValueId id) const noexcept
    {
        return std::hash<std::uint32_t>{}(id.value);
    }
};

struct BlockIdHash
{
    [[nodiscard]] std::size_t operator()(const BlockId id) const noexcept
    {
        return std::hash<std::uint32_t>{}(id.value);
    }
};
} // namespace asmstudio


#endif // ASMSTUDIO_IR_IRTYPES_HPP
