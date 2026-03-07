#ifndef ASMSTUDIO_SIMULATOR_SIMSTATE_HPP
#define ASMSTUDIO_SIMULATOR_SIMSTATE_HPP


#include <asmstudio/ir/IRTypes.hpp>

#include <cstddef>
#include <unordered_map>
#include <variant>
#include <vector>

namespace asmstudio
{
using RegValue = std::variant<int64_t, std::uint64_t, double, bool>;

struct SimFrame
{
    std::string functionName;
    std::unordered_map<ValueId, RegValue, ValueIdHash> values;
    std::size_t pc{}; // instruction index inside the current block
    BlockId block{};  // current block ID
};

struct SimState
{
    std::vector<SimFrame> callStack;
    std::vector<std::byte> memory; // heap simulation (unused in basic mode)
};
} // namespace asmstudio


#endif // ASMSTUDIO_SIMULATOR_SIMSTATE_HPP
