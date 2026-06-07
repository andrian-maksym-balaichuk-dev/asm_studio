#ifndef ASMSTUDIO_ANALYSIS_CFGDOT
#define ASMSTUDIO_ANALYSIS_CFGDOT


#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
// Translates the control flow graph of an IR function / module into a Graphviz DOT format string for visualization.
// The resulting DOT string can be rendered using Graphviz tools to visualize the structure of the control flow graph,
// with nodes representing basic blocks and edges representing control flow between them.
[[nodiscard]] std::string toDot(const IRFunction& function);
[[nodiscard]] std::string toDot(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_ANALYSIS_CFGDOT
