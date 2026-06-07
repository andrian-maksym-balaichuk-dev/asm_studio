#include <asmstudio/support/Diagnostic.hpp>

#include <asmstudio/support/Compat.hpp>

#include <string_view>
#include <utility>

namespace asmstudio
{
namespace
{
constexpr std::string_view severityLabel(Severity severity) noexcept
{
    switch (severity)
    {
    case Severity::Note: return "note";
    case Severity::Warning: return "warning";
    case Severity::Error: return "error";
    case Severity::Fatal: return "fatal";
    }
    return "unknown";
}
} // namespace

void DiagnosticBag::emit(Severity severity, std::string message, SourceRange range)
{
    m_diagnostics.push_back({ severity, std::move(message), range });
}

bool DiagnosticBag::hasErrors() const noexcept
{
    return compat::ranges::any_of(m_diagnostics, [](const Diagnostic& diagnostic) {
        return diagnostic.severity == Severity::Error || diagnostic.severity == Severity::Fatal;
    });
}

bool DiagnosticBag::empty() const noexcept
{
    return m_diagnostics.empty();
}

compat::Span<const Diagnostic> DiagnosticBag::all() const noexcept
{
    return m_diagnostics;
}

void DiagnosticBag::print(std::ostream& out) const
{
    for (const auto& [severity, message, range] : m_diagnostics)
    {
        out << severityLabel(severity);

        if (range.line != 0)
        {
            out << ':' << range.line << ':' << range.col;
        }

        out << ": " << message << '\n';
    }
}

void DiagnosticBag::clear() noexcept
{
    m_diagnostics.clear();
}
} // namespace asmstudio
