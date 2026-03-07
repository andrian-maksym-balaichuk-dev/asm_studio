#ifndef ASMSTUDIO_EXPLAIN_EXPLAIN_HPP
#define ASMSTUDIO_EXPLAIN_EXPLAIN_HPP

#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
[[nodiscard]] std::string explain(const IRFunction& fn);
[[nodiscard]] std::string explain(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_EXPLAIN_EXPLAIN_HPP