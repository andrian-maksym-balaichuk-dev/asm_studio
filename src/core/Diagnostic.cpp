#include <asmstudio/core/Diagnostic.hpp>

#include <algorithm>
#include <string_view>
#include <utility>

namespace asmstudio
{
namespace
{
constexpr std::string_view severityLabel(Severity s) noexcept
{
    switch (s)
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
    m_diagnostic.push_back({ severity, std::move(message), range });
}

bool DiagnosticBag::hasErrors() const noexcept
{
    return std::ranges::any_of(m_diagnostic, [](const Diagnostic& d) {
        return d.severity == Severity::Error || d.severity == Severity::Fatal;
    });
}

bool DiagnosticBag::empty() const noexcept
{
    return m_diagnostic.empty();
}

std::span<const Diagnostic> DiagnosticBag::all() const noexcept
{
    return m_diagnostic;
}

void DiagnosticBag::print(std::ostream& out) const
{
    for (const auto& [severity, message, range] : m_diagnostic)
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
    m_diagnostic.clear();
}
} // namespace asmstudio
