#ifndef ASMSTUDIO_VISUALIZATION_CFGDOT
#define ASMSTUDIO_VISUALIZATION_CFGDOT


#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
[[nodiscard]] std::string toDot(const IRFunction& fn);
[[nodiscard]] std::string toDot(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_VISUALIZATION_CFGDOT
