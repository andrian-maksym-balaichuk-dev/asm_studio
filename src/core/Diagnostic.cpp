#include <asmstudio/core/Diagnostic.hpp>

#include <algorithm>
#include <string_view>
#include <utility>

namespace asmstudio {

namespace {

constexpr std::string_view severityLabel(Severity s) noexcept
{
    switch(s)
    {
        case Severity::Note:    return "note";
        case Severity::Warning: return "warning";
        case Severity::Error:   return "error";
        case Severity::Fatal:   return "fatal";
    }
    return "unknown";
}

} // namespace

void DiagnosticBag::emit(Severity severity, std::string message, SourceRange range)
{
    diags_.push_back({severity, std::move(message), range});
}

bool DiagnosticBag::hasErrors() const noexcept
{
    return std::ranges::any_of(diags_, [](const Diagnostic& d) {
        return d.severity == Severity::Error || d.severity == Severity::Fatal;
    });
}

bool DiagnosticBag::empty() const noexcept { return diags_.empty(); }

std::span<const Diagnostic> DiagnosticBag::all() const noexcept { return diags_; }

void DiagnosticBag::print(std::ostream& out) const
{
    for(const auto& d : diags_)
    {
        out << severityLabel(d.severity);
        if(d.range.line != 0)
        {
            out << ':' << d.range.line << ':' << d.range.col;
        }
        out << ": " << d.message << '\n';
    }
}

void DiagnosticBag::clear() noexcept { diags_.clear(); }

} // namespace asmstudio
