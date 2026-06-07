#ifndef ASMSTUDIO_IR_LOWERING_HPP
#define ASMSTUDIO_IR_LOWERING_HPP


#include <asmstudio/support/Compat.hpp>
#include <asmstudio/support/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <memory>
#include <string_view>

namespace asmstudio
{
class Function;

// Translates a collection of API-level Function objects into an IRModule, performing lowering and emitting diagnostics as needed.
// This is the main entry point for the lowering phase, called by Program::build() after the user has finished defining their functions.
[[nodiscard]] IRModule lower(std::string_view moduleName, compat::Span<const std::unique_ptr<Function>> functions, DiagnosticBag& diagnostics);
} // namespace asmstudio


#endif // ASMSTUDIO_IR_LOWERING_HPP
