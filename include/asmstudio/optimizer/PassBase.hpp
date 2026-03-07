#ifndef ASMSTUDIO_PASSBASE_HPP
#define ASMSTUDIO_PASSBASE_HPP


#include <asmstudio/ir/IRTypes.hpp>

#include <concepts>
#include <string_view>

namespace asmstudio
{
template <typename P>
concept Pass = requires(P p, IRFunction& fn) {
    { p.run(fn) } -> std::same_as<bool>; // true if IR was modified
    { p.name() } -> std::convertible_to<std::string_view>;
};
} // namespace asmstudio


#endif // ASMSTUDIO_PASSBASE_HPP
