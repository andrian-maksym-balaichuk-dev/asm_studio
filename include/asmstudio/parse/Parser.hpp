#ifndef ASMSTUDIO_PARSE_PARSER_HPP
#define ASMSTUDIO_PARSE_PARSER_HPP


#include <asmstudio/parse/AsmProgram.hpp>
#include <asmstudio/parse/Lexer.hpp>
#include <asmstudio/parse/Token.hpp>

#include <string_view>

namespace asmstudio::parser
{
[[nodiscard]] constexpr ParsedProgram parse(std::string_view src) noexcept
{
    ParsedProgram parsedProgram{};
    Lexer lexer{ src };

    auto skipIgnorableTokens = [&](Token& token) {
        while (token.kind == TokenKind::Newline || token.kind == TokenKind::Comment)
        {
            token = lexer.next();
        }
    };

    Token token{ lexer.next() };
    skipIgnorableTokens(token);

    while (token.kind != TokenKind::Eof)
    {
        if (token.kind == TokenKind::Label)
        {
            parsedProgram.addLabel(token.text, static_cast<std::uint32_t>(parsedProgram.instructionCount()));
            token = lexer.next();
            skipIgnorableTokens(token);
            continue;
        }

        if (token.kind == TokenKind::Mnemonic)
        {
            const std::string_view mnemonic{ token.text };
            const std::uint32_t instructionLine{ token.line };
            std::string_view firstOperand{};
            std::string_view secondOperand{};

            token = lexer.next();

            if (token.kind == TokenKind::Register || token.kind == TokenKind::Immediate || token.kind == TokenKind::Unknown)
            {
                firstOperand = token.text;
                token = lexer.next();

                if (token.kind == TokenKind::Comma)
                {
                    token = lexer.next();
                    if (token.kind == TokenKind::Register || token.kind == TokenKind::Immediate ||
                        token.kind == TokenKind::Label || token.kind == TokenKind::Unknown)
                    {
                        secondOperand = token.text;
                        token = lexer.next();
                    }
                    else
                    {
                        parsedProgram.addDiagnostic("expected operand after ','", token.line, token.col, true);
                    }
                }
            }

            if (token.kind == TokenKind::Comment)
            {
                token = lexer.next();
            }

            if (token.kind != TokenKind::Newline && token.kind != TokenKind::Eof)
            {
                parsedProgram.addDiagnostic("expected newline after instruction", token.line, token.col, false);

                while (token.kind != TokenKind::Newline && token.kind != TokenKind::Eof)
                {
                    token = lexer.next();
                }
            }

            parsedProgram.addInstruction(mnemonic, firstOperand, secondOperand, instructionLine);
            skipIgnorableTokens(token);
            continue;
        }

        if (token.kind == TokenKind::Comment || token.kind == TokenKind::Newline)
        {
            skipIgnorableTokens(token);
            continue;
        }

        parsedProgram.addDiagnostic("unexpected token", token.line, token.col, false);
        token = lexer.next();
    }

    return parsedProgram;
}
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSE_PARSER_HPP
