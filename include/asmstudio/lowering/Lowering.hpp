#ifndef ASMSTUDIO_LOWERING_LOWERING_HPP
#define ASMSTUDIO_LOWERING_LOWERING_HPP

#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <memory>
#include <span>
#include <string_view>

namespace asmstudio
{
class Function;

[[nodiscard]] IRModule lower(std::string_view moduleName, std::span<const std::unique_ptr<Function>> functions, DiagnosticBag& diags);
} // namespace asmstudio

#endif // ASMSTUDIO_LOWERING_LOWERING_HPP
