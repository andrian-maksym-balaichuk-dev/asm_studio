#pragma once

#include <cstdint>
#include <string_view>

namespace asmstudio {

struct SourceLocation
{
    std::uint32_t line{};
    std::uint32_t col{};
    std::string_view file{};
};

struct SourceRange
{
    std::uint32_t line{};
    std::uint32_t col{};
    std::uint32_t length{};
};

} // namespace asmstudio
