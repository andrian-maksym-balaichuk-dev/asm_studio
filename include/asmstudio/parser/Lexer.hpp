#ifndef ASMSTUDIO_PARSER_LEXER_HPP
#define ASMSTUDIO_PARSER_LEXER_HPP


#include <asmstudio/parser/Token.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string_view>

namespace asmstudio::parser
{
inline constexpr std::array<std::string_view, 19> KnownMnemonics = { "mov",  "add", "sub",  "mul",  "div",
                                                                     "cmp",  "jmp", "je",   "jne",  "jl",
                                                                     "jg",   "jle", "jge",  "push", "pop",
                                                                     "call", "ret", "load", "store" };

[[nodiscard]] constexpr bool isMnemonic(std::string_view word) noexcept
{
    return std::ranges::any_of(KnownMnemonics, [word](std::string_view m) { return m == word; });
}

[[nodiscard]] constexpr bool isRegister(std::string_view word) noexcept
{
    if (word.size() < 2 || word[0] != 'r')
    {
        return false;
    }
    for (std::size_t i = 1; i < word.size(); ++i)
    {
        if (word[i] < '0' || word[i] > '9')
        {
            return false;
        }
    }


    std::size_t num = 0;
    for (std::size_t i = 1; i < word.size(); ++i)
    {
        num = num * 10 + static_cast<std::size_t>(word[i] - '0');
    }

    return num <= 15;
}


class Lexer
{
public:
    [[nodiscard]] constexpr explicit Lexer(std::string_view src) noexcept : m_src(src), m_pos(0), m_line(1), m_col(1) {}

    [[nodiscard]] constexpr Token next() noexcept
    {
        skipWhitespace();

        if (m_pos >= m_src.size())
        {
            return makeToken(TokenKind::Eof, {});
        }

        char c = m_src[m_pos];

        if (c == '\n')
        {
            Token tok = makeToken(TokenKind::Newline, m_src.substr(m_pos, 1));
            ++m_pos;
            ++m_line;
            m_col = 1;
            return tok;
        }

        if (c == ',')
        {
            return advance(TokenKind::Comma);
        }

        if (c == ':')
        {
            return advance(TokenKind::Colon);
        }

        if (c == ';')
        {
            std::size_t start = m_pos;

            while (m_pos < m_src.size() && m_src[m_pos] != '\n')
            {
                ++m_pos;
            }

            return makeToken(TokenKind::Comment, m_src.substr(start, m_pos - start));
        }

        if (c == '-' || (c >= '0' && c <= '9'))
        {
            return lexNumber();
        }

        if (isAlpha(c) || c == '_')
        {
            return lexWord();
        }

        return advance(TokenKind::Error);
    }

    [[nodiscard]] constexpr std::uint32_t line() const noexcept
    {
        return m_line;
    }
    [[nodiscard]] constexpr std::uint32_t col() const noexcept
    {
        return m_col;
    }

private:
    [[nodiscard]] static constexpr bool isAlpha(char c) noexcept
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    [[nodiscard]] static constexpr bool isAlNum(char c) noexcept
    {
        return isAlpha(c) || (c >= '0' && c <= '9');
    }

    constexpr void skipWhitespace() noexcept
    {
        while (m_pos < m_src.size())
        {
            char c = m_src[m_pos];

            if (c == ' ' || c == '\t' || c == '\r')
            {
                ++m_pos;
                ++m_col;
            }
            else
            {
                break;
            }
        }
    }

    [[nodiscard]] constexpr Token makeToken(const TokenKind kind, std::string_view text) const noexcept
    {
        return Token{ kind, text, m_line, m_col };
    }

    [[nodiscard]] constexpr Token advance(TokenKind kind) noexcept
    {
        Token t = makeToken(kind, m_src.substr(m_pos, 1));
        ++m_pos;
        ++m_col;
        return t;
    }

    [[nodiscard]] constexpr Token lexNumber() noexcept
    {
        std::size_t start = m_pos;
        std::uint32_t startCol = m_col;

        if (m_src[m_pos] == '-')
        {
            ++m_pos;
            ++m_col;
        }

        while (m_pos < m_src.size() && m_src[m_pos] >= '0' && m_src[m_pos] <= '9')
        {
            ++m_pos;
            ++m_col;
        }

        return Token{ TokenKind::Immediate, m_src.substr(start, m_pos - start), m_line, startCol };
    }

    [[nodiscard]] constexpr Token lexWord() noexcept
    {
        std::size_t start = m_pos;
        std::uint32_t startCol = m_col;

        while (m_pos < m_src.size() && (isAlNum(m_src[m_pos]) || m_src[m_pos] == '_'))
        {
            ++m_pos;
            ++m_col;
        }

        std::string_view word = m_src.substr(start, m_pos - start);

        // Peek: if next non-space char is ':', this is a label.
        std::size_t peek = m_pos;
        while (peek < m_src.size() && (m_src[peek] == ' ' || m_src[peek] == '\t'))
        {
            ++peek;
        }

        if (peek < m_src.size() && m_src[peek] == ':')
        {
            m_pos = peek + 1;
            m_col = startCol + static_cast<std::uint32_t>(m_pos - start);
            return Token{ TokenKind::Label, word, m_line, startCol };
        }

        if (isRegister(word))
        {
            return Token{ TokenKind::Register, word, m_line, startCol };
        }
        if (isMnemonic(word))
        {
            return Token{ TokenKind::Mnemonic, word, m_line, startCol };
        }

        return Token{ TokenKind::Unknown, word, m_line, startCol };
    }

    std::string_view m_src;
    std::size_t m_pos;
    std::uint32_t m_line;
    std::uint32_t m_col;
};
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_LEXER_HPP
