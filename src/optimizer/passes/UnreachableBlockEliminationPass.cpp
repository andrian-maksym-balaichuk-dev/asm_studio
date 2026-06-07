#include <asmstudio/optimizer/Optimizer.hpp>

#include <asmstudio/core/Compat.hpp>

#include <unordered_set>
#include <vector>

namespace asmstudio
{
namespace
{
[[nodiscard]] std::unordered_set<std::uint32_t> findReachableBlockIds(const IRFunction& function)
{
    std::unordered_set<std::uint32_t> reachableBlockIds{};
    std::vector<BlockId> worklist{};

    worklist.push_back(function.blocks[0].id);
    reachableBlockIds.insert(function.blocks[0].id.value);

    while (!worklist.empty())
    {
        const BlockId currentBlockId{ worklist.back() };
        worklist.pop_back();

        const IRBlock* currentBlock{ function.findBlock(currentBlockId) };
        if (!currentBlock) { continue; }

        auto enqueueBlock = [&](BlockId targetBlockId) {
            if (reachableBlockIds.insert(targetBlockId.value).second)
            {
                worklist.push_back(targetBlockId);
            }
        };

        for (const auto& instruction : currentBlock->instrs)
        {
            if (instruction.trueTarget)  { enqueueBlock(*instruction.trueTarget); }
            if (instruction.falseTarget) { enqueueBlock(*instruction.falseTarget); }
        }
    }

    return reachableBlockIds;
}

} // namespace

[[nodiscard]] bool UnreachableBlockElim::run(IRFunction& function) const
{
    if (function.blocks.empty()) { return false; }

    const auto reachableBlockIds{ findReachableBlockIds(function) };
    const auto previousBlockCount{ function.blocks.size() };

    asmstudio::compat::ranges::erase_if(
        function.blocks,
        [&](const IRBlock& block) {
            return reachableBlockIds.find(block.id.value) == reachableBlockIds.end();
        });

    const bool changed{ function.blocks.size() != previousBlockCount };
    if (changed)
    {
        function.rebuildBlockIndex();
    }
    return changed;
}
} // namespace asmstudio
