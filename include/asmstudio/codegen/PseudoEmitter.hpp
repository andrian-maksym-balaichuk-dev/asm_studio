#ifndef ASMSTUDIO_CODEGEN_PSEUDOEMITTER_HPP
#define ASMSTUDIO_CODEGEN_PSEUDOEMITTER_HPP


#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
// Translates an IR function / module into a human-readable pseudo-assembly listing as a string.
// This is intended for debugging and educational purposes, not for actual code generation.
[[nodiscard]] std::string emitPseudoAsm(const IRFunction& function);
[[nodiscard]] std::string emitPseudoAsm(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_CODEGEN_PSEUDOEMITTER_HPP
