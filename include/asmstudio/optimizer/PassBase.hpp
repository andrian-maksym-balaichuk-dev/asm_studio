#ifndef ASMSTUDIO_PASSBASE_HPP
#define ASMSTUDIO_PASSBASE_HPP


#include <asmstudio/core/Compat.hpp>
#include <asmstudio/ir/IRTypes.hpp>

#include <string_view>
#include <type_traits>

namespace asmstudio
{
#ifdef ASM_CXX20
#include <concepts>

template <typename P>
concept Pass = requires(P pass, IRFunction& function) {
    { pass.run(function) } -> std::same_as<bool>;
    { pass.name() } -> std::convertible_to<std::string_view>;
};

#else // C++17 type-trait fallback
template <typename P, typename = void>
struct IsPass : std::false_type
{};

template <typename P>
struct IsPass<
    P,
    std::void_t<
        std::enable_if_t<std::is_same_v<decltype(std::declval<P>().run(std::declval<IRFunction&>())), bool> >,
        std::enable_if_t<std::is_convertible_v<decltype(std::declval<P>().name()), std::string_view> > > > : std::true_type
{};

template <typename P>
inline constexpr bool IsPassV{ IsPass<P>::value };

#endif // ASM_CXX20
} // namespace asmstudio


#endif // ASMSTUDIO_PASSBASE_HPP
