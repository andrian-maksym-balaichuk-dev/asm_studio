#ifndef ASMSTUDIO_ANALYSIS_EXPLAIN_HPP
#define ASMSTUDIO_ANALYSIS_EXPLAIN_HPP


#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
// Generates a human-readable explanation of the IR function / module as a string.
// This is intended for debugging and educational purposes, not for actual code generation.
[[nodiscard]] std::string explain(const IRFunction& function);
[[nodiscard]] std::string explain(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_ANALYSIS_EXPLAIN_HPP
