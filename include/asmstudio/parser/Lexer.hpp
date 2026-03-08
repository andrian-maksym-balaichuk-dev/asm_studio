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
    for (const auto mnemonic : KnownMnemonics)
    {
        if (mnemonic == word)
        {
            return true;
        }
    }
    return false;
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
    [[nodiscard]] constexpr explicit Lexer(std::string_view source) noexcept
    : m_src{ source }, m_pos{ 0 }, m_line{ 1 }, m_col{ 1 }
    {}

    [[nodiscard]] constexpr Token next() noexcept
    {
        skipWhitespace();

        if (m_pos >= m_src.size())
        {
            return makeToken(TokenKind::Eof, {});
        }

        const char currentCharacter{ m_src[m_pos] };

        if (currentCharacter == '\n')
        {
            Token token{ makeToken(TokenKind::Newline, m_src.substr(m_pos, 1)) };
            ++m_pos;
            ++m_line;
            m_col = 1;
            return token;
        }

        if (currentCharacter == ',')
        {
            return advance(TokenKind::Comma);
        }

        if (currentCharacter == ':')
        {
            return advance(TokenKind::Colon);
        }

        if (currentCharacter == ';')
        {
            const std::size_t startPosition{ m_pos };

            while (m_pos < m_src.size() && m_src[m_pos] != '\n')
            {
                ++m_pos;
            }

            return makeToken(TokenKind::Comment, m_src.substr(startPosition, m_pos - startPosition));
        }

        if (currentCharacter == '-' || (currentCharacter >= '0' && currentCharacter <= '9'))
        {
            return lexNumber();
        }

        if (isAlpha(currentCharacter) || currentCharacter == '_')
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
            const char currentCharacter{ m_src[m_pos] };

            if (currentCharacter == ' ' || currentCharacter == '\t' || currentCharacter == '\r')
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
        Token token{ makeToken(kind, m_src.substr(m_pos, 1)) };
        ++m_pos;
        ++m_col;
        return token;
    }

    [[nodiscard]] constexpr Token lexNumber() noexcept
    {
        const std::size_t startPosition{ m_pos };
        const std::uint32_t startColumn{ m_col };

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

        return Token{ TokenKind::Immediate, m_src.substr(startPosition, m_pos - startPosition), m_line, startColumn };
    }

    [[nodiscard]] constexpr Token lexWord() noexcept
    {
        const std::size_t startPosition{ m_pos };
        const std::uint32_t startColumn{ m_col };

        while (m_pos < m_src.size() && (isAlNum(m_src[m_pos]) || m_src[m_pos] == '_'))
        {
            ++m_pos;
            ++m_col;
        }

        const std::string_view word{ m_src.substr(startPosition, m_pos - startPosition) };

        // Peek: if next non-space char is ':', this is a label.
        std::size_t peek = m_pos;
        while (peek < m_src.size() && (m_src[peek] == ' ' || m_src[peek] == '\t'))
        {
            ++peek;
        }

        if (peek < m_src.size() && m_src[peek] == ':')
        {
            m_pos = peek + 1;
            m_col = startColumn + static_cast<std::uint32_t>(m_pos - startPosition);
            return Token{ TokenKind::Label, word, m_line, startColumn };
        }

        if (isRegister(word))
        {
            return Token{ TokenKind::Register, word, m_line, startColumn };
        }
        if (isMnemonic(word))
        {
            return Token{ TokenKind::Mnemonic, word, m_line, startColumn };
        }

        return Token{ TokenKind::Unknown, word, m_line, startColumn };
    }

    std::string_view m_src;
    std::size_t m_pos;
    std::uint32_t m_line;
    std::uint32_t m_col;
};
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_LEXER_HPP
