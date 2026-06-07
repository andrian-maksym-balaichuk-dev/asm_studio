#ifndef ASMSTUDIO_CODEGEN_ARM64EMITTER_HPP
#define ASMSTUDIO_CODEGEN_ARM64EMITTER_HPP


#include <asmstudio/support/Compat.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <string>

namespace asmstudio
{
enum class Arm64Platform
{
    macOS,   // Mach-O ABI: symbol prefix "_",  svc #0x80
    Linux,   // ELF ABI:    no symbol prefix,    svc #0
    Windows, // PE/COFF:    no symbol prefix (MinGW/Clang; MSVC has no inline asm on ARM64)
};

[[nodiscard]] constexpr Arm64Platform hostArm64Platform() noexcept
{
#if defined(__APPLE__)
    return Arm64Platform::macOS;
#elif defined(_WIN32)
    return Arm64Platform::Windows;
#else
    return Arm64Platform::Linux;
#endif
}

// Translates an IR function / module into a real ARM64 `asm volatile(...)` block
// as a C++ source string ready to embed in generated code or print for inspection.
[[nodiscard]] std::string emitArm64AsmVolatile(const IRFunction& function, Arm64Platform platform = hostArm64Platform());
[[nodiscard]] std::string emitArm64AsmVolatile(const IRModule& module, Arm64Platform platform = hostArm64Platform());

} // namespace asmstudio


#endif // ASMSTUDIO_CODEGEN_ARM64EMITTER_HPP
