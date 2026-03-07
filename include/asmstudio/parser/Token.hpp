#ifndef ASMSTUDIO_PARSER_TOKEN_HPP
#define ASMSTUDIO_PARSER_TOKEN_HPP

#include <cstdint>
#include <string_view>

namespace asmstudio::parser
{

enum class TokenKind : std::uint8_t
{
    Label,     // identifier followed by ':'
    Mnemonic,  // known instruction word (mov, add, …)
    Register,  // r0–r15
    Immediate, // integer literal
    Comma,     // ','
    Colon,     // ':'
    Newline,   // '\n'
    Comment,   // '; ...' to end of line
    Unknown,   // any other identifier / token
    Eof,       // end of input
    Error,     // lexer error
};

struct Token
{
    TokenKind kind{ TokenKind::Eof };
    std::string_view text;
    std::uint32_t line{};
    std::uint32_t col{};
};
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_TOKEN_HPP
