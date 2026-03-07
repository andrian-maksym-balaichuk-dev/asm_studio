#ifndef ASMSTUDIO_IR_IRTYPES_HPP
#define ASMSTUDIO_IR_IRTYPES_HPP

#include <asmstudio/core/Types.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
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
};

struct IRBlock
{
    std::string name;
    BlockId id;
    std::vector<IRInstr> instrs;
};

struct IRFunction
{
    std::string name;
    std::vector<IRBlock> blocks;
    std::vector<IRValue> values; // indexed by ValueId::value
};

struct IRModule
{
    std::string name;
    std::vector<IRFunction> functions;
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
