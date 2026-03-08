#ifndef ASMSTUDIO_SIMULATOR_SIMSTATE_HPP
#define ASMSTUDIO_SIMULATOR_SIMSTATE_HPP


#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace asmstudio
{
using RegValue = std::variant<int64_t, std::uint64_t, double, bool>;

struct SimFrame
{
    std::string functionName;
    std::vector<std::optional<RegValue>> values;
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
