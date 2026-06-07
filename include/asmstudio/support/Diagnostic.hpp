#ifndef ASMSTUDIO_SUPPORT_DIAGNOSTIC_HPP
#define ASMSTUDIO_SUPPORT_DIAGNOSTIC_HPP


#include <asmstudio/support/Compat.hpp>
#include <asmstudio/support/SourceLocation.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace asmstudio
{
enum class Severity
{
    Note,
    Warning,
    Error,
    Fatal,
};

struct Diagnostic
{
    Severity severity;
    std::string message;
    SourceRange range;
};

class DiagnosticBag
{
public:
    void emit(Severity severity, std::string message, SourceRange range = {});

    [[nodiscard]] bool hasErrors() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] compat::Span<const Diagnostic> all() const noexcept;

    void print(std::ostream& out) const;
    void clear() noexcept;

private:
    std::vector<Diagnostic> m_diagnostics;
};
} // namespace asmstudio


#endif // ASMSTUDIO_SUPPORT_DIAGNOSTIC_HPP
