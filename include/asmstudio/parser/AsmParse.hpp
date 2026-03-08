#ifndef ASMSTUDIO_PARSER_ASM_PARSE_HPP
#define ASMSTUDIO_PARSER_ASM_PARSE_HPP


#include <asmstudio/core/Compat.hpp>
#include <asmstudio/parser/AsmProgram.hpp>
#include <asmstudio/parser/Parser.hpp>

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace asmstudio
{
// in c++20 and later, we can use a fixed-size string literal wrapper to allow passing string literals as non-type
// template parameters to asm_parse<Src>() for compile-time parsing and validation of embedded assembly code.
#ifdef ASM_CXX20
template <std::size_t N>
struct FixedString
{
    char data[N]{};

    constexpr FixedString(const char (&s)[N]) noexcept
    {
        std::copy(s, s + N, data);
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return { data, N - 1 };
    }
};

// ---------------------------------------------------------------------------
// asm_parse<Src>() — consteval: validates embedded assembly at compile time.
//
// Usage:
//   constexpr auto prog = asm_parse<R"(
//       mov r0, 0
//       add r0, 1
//       ret
//   )">();
//   static_assert(prog.ok());
//   static_assert(prog.instructionCount() == 3);
template <FixedString Src>
[[nodiscard]] consteval parser::ParsedProgram asm_parse() noexcept
{
    return parser::parse(Src.view());
}
#endif

[[nodiscard]] inline parser::ParsedProgram asm_parse_runtime(std::string_view src)
{
    return parser::parse(src);
}
} // namespace asmstudio


#endif // ASMSTUDIO_PARSER_ASM_PARSE_HPP
