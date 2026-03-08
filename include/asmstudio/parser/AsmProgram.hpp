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
    std::string_view firstOperand;
    std::string_view secondOperand;
    std::uint32_t line{};
};

struct ParsedLabel
{
    std::string_view name;
    std::uint32_t instructionIndex{}; // index of first instruction after the label
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
    std::size_t instructionCountValue{};
    std::size_t labelCountValue{};
    std::size_t diagnosticCountValue{};

    [[nodiscard]] constexpr bool ok() const noexcept
    {
        for (std::size_t diagnosticIndex{ 0 }; diagnosticIndex < diagnosticCountValue; ++diagnosticIndex)
        {
            if (diagnostics[diagnosticIndex].isError)
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr std::size_t instructionCount() const noexcept
    {
        return instructionCountValue;
    }
    [[nodiscard]] constexpr std::size_t labelCount() const noexcept
    {
        return labelCountValue;
    }
    [[nodiscard]] constexpr std::size_t diagCount() const noexcept
    {
        return diagnosticCountValue;
    }

    constexpr void
    addInstruction(std::string_view mnemonic, std::string_view firstOperand, std::string_view secondOperand, const std::uint32_t line)
    {
        if (instructionCountValue < MaxInstructions)
        {
            instructions[instructionCountValue++] = { mnemonic, firstOperand, secondOperand, line };
        }
    }

    constexpr void addLabel(std::string_view name, const std::uint32_t instructionIndex)
    {
        if (labelCountValue < MaxLabels)
        {
            labels[labelCountValue++] = { name, instructionIndex };
        }
    }

    constexpr void addDiagnostic(std::string_view message, std::uint32_t line, const std::uint32_t col, const bool isError)
    {
        if (diagnosticCountValue < MaxDiagnostics)
        {
            diagnostics[diagnosticCountValue++] = { message, line, col, isError };
        }
    }
};
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_ASMPROGRAM_HPP
