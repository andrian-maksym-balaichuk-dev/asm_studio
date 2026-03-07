#ifndef ASMSTUDIO_BACKEND_PSEUDOEMITTER_HPP
#define ASMSTUDIO_BACKEND_PSEUDOEMITTER_HPP


#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
[[nodiscard]] std::string emitPseudoAsm(const IRFunction& fn);
[[nodiscard]] std::string emitPseudoAsm(const IRModule& module);
} // namespace asmstudio


#endif // ASMSTUDIO_BACKEND_PSEUDOEMITTER_HPP
