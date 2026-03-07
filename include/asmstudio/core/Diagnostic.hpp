#ifndef ASMSTUDIO_CORE_DIAGNOSTIC_HPP
#define ASMSTUDIO_CORE_DIAGNOSTIC_HPP

#include <asmstudio/core/SourceLocation.hpp>

#include <ostream>
#include <span>
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
    [[nodiscard]] std::span<const Diagnostic> all() const noexcept;

    void print(std::ostream& out) const;
    void clear() noexcept;

private:
    std::vector<Diagnostic> m_diagnostic;
};
} // namespace asmstudio


#endif // ASMSTUDIO_CORE_DIAGNOSTIC_HPP
