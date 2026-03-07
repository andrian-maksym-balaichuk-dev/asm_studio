#ifndef ASMSTUDIO_PARSER_ASMPROGRAM_HPP
#define ASMSTUDIO_PARSER_ASMPROGRAM_HPP


#include <array>
#include <cstddef>
#include <string_view>

namespace asmstudio::parser
{
inline constexpr std::size_t MaxInstructions = 256;
inline constexpr std::size_t MaxLabels = 64;
inline constexpr std::size_t MaxDiagnostics = 16;

struct ParsedInstruction
{
    std::string_view mnemonic;
    std::string_view op1;
    std::string_view op2;
    std::uint32_t line{};
};

struct ParsedLabel
{
    std::string_view name;
    std::uint32_t instrIndex{}; // index of first instr after the label
};

struct ParseDiagnostic
{
    std::string_view message;
    std::uint32_t line{};
    std::uint32_t col{};
    bool isError{ false };
};

struct ParsedProgram
{
    std::array<ParsedInstruction, MaxInstructions> instructions{};
    std::array<ParsedLabel, MaxLabels> labels{};
    std::array<ParseDiagnostic, MaxDiagnostics> diagnostics{};
    std::size_t instrCount_{};
    std::size_t labelCount_{};
    std::size_t diagCount_{};

    [[nodiscard]] constexpr bool ok() const noexcept
    {
        for (std::size_t i = 0; i < diagCount_; ++i)
        {
            if (diagnostics[i].isError)
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr std::size_t instructionCount() const noexcept
    {
        return instrCount_;
    }
    [[nodiscard]] constexpr std::size_t labelCount() const noexcept
    {
        return labelCount_;
    }
    [[nodiscard]] constexpr std::size_t diagCount() const noexcept
    {
        return diagCount_;
    }

    constexpr void addInstruction(std::string_view mnemonic, std::string_view op1, std::string_view op2, const std::uint32_t line)
    {
        if (instrCount_ < MaxInstructions)
        {
            instructions[instrCount_++] = { mnemonic, op1, op2, line };
        }
    }

    constexpr void addLabel(std::string_view name, const std::uint32_t instrIdx)
    {
        if (labelCount_ < MaxLabels)
        {
            labels[labelCount_++] = { name, instrIdx };
        }
    }

    constexpr void addDiagnostic(std::string_view msg, std::uint32_t line, const std::uint32_t col, const bool isError)
    {
        if (diagCount_ < MaxDiagnostics)
        {
            diagnostics[diagCount_++] = { msg, line, col, isError };
        }
    }
};
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_ASMPROGRAM_HPP
