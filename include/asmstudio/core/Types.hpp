#pragma once

#include <compare>
#include <cstdint>

namespace asmstudio {

// ---------------------------------------------------------------------------
// Strong ID wrappers — prevent accidental mixing of unrelated integer IDs.
// ---------------------------------------------------------------------------

struct RegisterId
{
    std::uint16_t value{};
    auto operator<=>(const RegisterId&) const noexcept = default;
};

struct BlockId
{
    std::uint32_t value{};
    auto operator<=>(const BlockId&) const noexcept = default;
};

struct ValueId
{
    std::uint32_t value{};
    auto operator<=>(const ValueId&) const noexcept = default;
};

struct InstrId
{
    std::uint32_t value{};
    auto operator<=>(const InstrId&) const noexcept = default;
};

// ---------------------------------------------------------------------------
// Enumerations.
// ---------------------------------------------------------------------------

enum class DataType
{
    Int32,
    Int64,
    Float32,
    Float64,
};

enum class OptimizationLevel
{
    None,
    Basic,
    Aggressive,
    Experimental,
};

} // namespace asmstudio
