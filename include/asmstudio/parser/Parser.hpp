#ifndef ASMSTUDIO_PARSER_PARSER_HPP
#define ASMSTUDIO_PARSER_PARSER_HPP

#include <asmstudio/parser/AsmProgram.hpp>
#include <asmstudio/parser/Lexer.hpp>
#include <asmstudio/parser/Token.hpp>

#include <string_view>

namespace asmstudio::parser
{
[[nodiscard]] constexpr ParsedProgram parse(std::string_view src) noexcept
{
    ParsedProgram prog;
    Lexer lex{ src };

    auto skipNewlines = [&](Token& tok) {
        while (tok.kind == TokenKind::Newline || tok.kind == TokenKind::Comment)
        {
            tok = lex.next();
        }
    };

    Token tok = lex.next();
    skipNewlines(tok);

    while (tok.kind != TokenKind::Eof)
    {
        if (tok.kind == TokenKind::Label)
        {
            prog.addLabel(tok.text, static_cast<std::uint32_t>(prog.instructionCount()));
            tok = lex.next();
            skipNewlines(tok);
            continue;
        }

        if (tok.kind == TokenKind::Mnemonic)
        {
            std::string_view mnemonic = tok.text;
            std::uint32_t instrLine = tok.line;
            std::string_view op1;
            std::string_view op2;

            tok = lex.next();

            if (tok.kind == TokenKind::Register || tok.kind == TokenKind::Immediate || tok.kind == TokenKind::Unknown)
            {
                op1 = tok.text;
                tok = lex.next();

                if (tok.kind == TokenKind::Comma)
                {
                    tok = lex.next();
                    if (tok.kind == TokenKind::Register || tok.kind == TokenKind::Immediate ||
                        tok.kind == TokenKind::Label || tok.kind == TokenKind::Unknown)
                    {
                        op2 = tok.text;
                        tok = lex.next();
                    }
                    else
                    {
                        prog.addDiagnostic("expected operand after ','", tok.line, tok.col, true);
                    }
                }
            }

            if (tok.kind == TokenKind::Comment)
            {
                tok = lex.next();
            }

            if (tok.kind != TokenKind::Newline && tok.kind != TokenKind::Eof)
            {
                prog.addDiagnostic("expected newline after instruction", tok.line, tok.col, false);

                while (tok.kind != TokenKind::Newline && tok.kind != TokenKind::Eof)
                {
                    tok = lex.next();
                }
            }

            prog.addInstruction(mnemonic, op1, op2, instrLine);
            skipNewlines(tok);
            continue;
        }

        if (tok.kind == TokenKind::Comment || tok.kind == TokenKind::Newline)
        {
            skipNewlines(tok);
            continue;
        }

        prog.addDiagnostic("unexpected token", tok.line, tok.col, false);
        tok = lex.next();
    }

    return prog;
}
} // namespace asmstudio::parser


#endif // ASMSTUDIO_PARSER_PARSER_HPP