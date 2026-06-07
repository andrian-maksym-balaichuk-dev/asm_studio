#ifndef ASMSTUDIO_SUPPORT_COMPAT_HPP
#define ASMSTUDIO_SUPPORT_COMPAT_HPP


#include <algorithm>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// ---------------------------------------------------------------------------
// Standard-version detection macros.
// ---------------------------------------------------------------------------
#if __cplusplus >= 202302L
#define ASM_CXX23 1
#endif
#if __cplusplus >= 202002L
#define ASM_CXX20 1
#endif

// ---------------------------------------------------------------------------
// overloaded{} helper — C++17 compatible, used with std::visit.
// ---------------------------------------------------------------------------
namespace asmstudio::compat
{
template <typename... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace asmstudio::compat

// ---------------------------------------------------------------------------
// Span<T> — wraps std::span on C++20+, lightweight fallback on C++17.
// ---------------------------------------------------------------------------
#ifdef ASM_CXX20
#include <span>

namespace asmstudio::compat
{
template <typename T>
using Span = std::span<T>;
} // namespace asmstudio::compat

#else

namespace asmstudio::compat
{
template <typename T>
class Span
{
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using pointer = T*;
    using reference = T&;
    using iterator = T*;

    constexpr Span() noexcept = default;
    constexpr Span(pointer data, size_type size) noexcept : m_data{ data }, m_size{ size } {}

    template <
        typename Container,
        typename = decltype(std::data(std::declval<Container&>())),
        typename = decltype(std::size(std::declval<Container&>())),
        typename = std::enable_if_t<std::is_convertible_v<decltype(std::data(std::declval<Container&>())), pointer> > >
    constexpr Span(Container& container) noexcept : m_data{ std::data(container) }, m_size{ std::size(container) }
    {}

    [[nodiscard]] constexpr iterator begin() const noexcept
    {
        return m_data;
    }
    [[nodiscard]] constexpr iterator end() const noexcept
    {
        return m_data + m_size;
    }
    [[nodiscard]] constexpr pointer data() const noexcept
    {
        return m_data;
    }
    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return m_size;
    }
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }
    [[nodiscard]] constexpr reference operator[](size_type index) const noexcept
    {
        return m_data[index];
    }

private:
    pointer m_data{ nullptr };
    size_type m_size{ 0 };
};
} // namespace asmstudio::compat
#endif

// ---------------------------------------------------------------------------
// Result<T,E> — wraps std::expected (C++23) or a minimal shim (C++17/20).
// ---------------------------------------------------------------------------
#ifdef ASM_CXX23
#include <expected>

namespace asmstudio::compat
{
template <typename T, typename E>
using Result = std::expected<T, E>;

template <typename E>
[[nodiscard]] auto makeUnexpected(E&& e)
{
    return std::unexpected(std::forward<E>(e));
}
} // namespace asmstudio::compat

#else // C++17 / C++20 fallback

#include <stdexcept>
#include <variant>

namespace asmstudio::compat
{

// Sentinel tag so we can distinguish "error" state inside the variant.
template <typename E>
struct Unexpected
{
    E value;
};

// Minimal std::expected-compatible type.
template <typename T, typename E>
class Result
{
public:
    // Construct from value (success).
    Result(T val) : m_storage{ std::move(val) } {} // NOLINT(google-explicit-constructor)

    // Construct from Unexpected<E> (error).
    Result(Unexpected<E> u) : m_storage{ std::move(u) } {} // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool has_value() const noexcept
    {
        return std::holds_alternative<T>(m_storage);
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]] T& value() &
    {
        return std::get<T>(m_storage);
    }
    [[nodiscard]] const T& value() const&
    {
        return std::get<T>(m_storage);
    }
    [[nodiscard]] T& operator*() &
    {
        return value();
    }
    [[nodiscard]] const T& operator*() const&
    {
        return value();
    }
    [[nodiscard]] T* operator->()
    {
        return &value();
    }
    [[nodiscard]] const T* operator->() const
    {
        return &value();
    }

    [[nodiscard]] E& error()
    {
        return std::get<Unexpected<E> >(m_storage).value;
    }
    [[nodiscard]] const E& error() const
    {
        return std::get<Unexpected<E> >(m_storage).value;
    }

private:
    std::variant<T, Unexpected<E> > m_storage;
};

// Specialisation for Result<void, E>.
template <typename E>
class Result<void, E>
{
public:
    Result() : m_error{} {}
    Result(Unexpected<E> u) : m_error{ std::move(u.value) } {} // NOLINT

    [[nodiscard]] bool has_value() const noexcept
    {
        return !m_error.has_value();
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]] E& error()
    {
        return *m_error;
    }
    [[nodiscard]] const E& error() const
    {
        return *m_error;
    }

private:
    std::optional<E> m_error;
};

template <typename E>
[[nodiscard]] Unexpected<E> makeUnexpected(E e)
{
    return Unexpected<E>{ std::move(e) };
}

} // namespace asmstudio::compat
#endif // ASM_CXX23

// ---------------------------------------------------------------------------
// print / println — wraps std::print (C++23) or falls back to std::cout.
// ---------------------------------------------------------------------------
#ifdef ASM_CXX23
#include <print>

namespace asmstudio::compat
{
template <typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args)
{
    std::print(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args)
{
    std::println(fmt, std::forward<Args>(args)...);
}

inline void println()
{
    std::println();
}
} // namespace asmstudio::compat

#else // C++17 / C++20 fallback
#include <iostream>

namespace asmstudio::compat
{
namespace detail
{
template <typename T>
[[nodiscard]] std::string toString(T&& value)
{
    using ValueType = std::decay_t<T>;

    if constexpr (std::is_same_v<ValueType, std::string>)
    {
        return value;
    }
    else if constexpr (std::is_same_v<ValueType, std::string_view>)
    {
        return std::string{ value };
    }
    else if constexpr (std::is_same_v<ValueType, const char*> || std::is_same_v<ValueType, char*>)
    {
        return value == nullptr ? std::string{} : std::string{ value };
    }
    else if constexpr (std::is_same_v<ValueType, bool>)
    {
        return value ? "true" : "false";
    }
    else
    {
        std::ostringstream outputStream{};
        outputStream << value;
        return outputStream.str();
    }
}

[[nodiscard]] inline std::string applyFormatSpec(std::string text, std::string_view spec)
{
    if (spec.empty())
    {
        return text;
    }

    char fillCharacter{ ' ' };
    char alignCharacter{ '>' };
    std::size_t index{ 0 };

    if (spec.size() >= 2 && (spec[1] == '<' || spec[1] == '>' || spec[1] == '^'))
    {
        fillCharacter = spec[0];
        alignCharacter = spec[1];
        index = 2;
    }
    else if (spec[0] == '<' || spec[0] == '>' || spec[0] == '^')
    {
        alignCharacter = spec[0];
        index = 1;
    }

    std::size_t width{ 0 };
    while (index < spec.size() && spec[index] >= '0' && spec[index] <= '9')
    {
        width = (width * 10U) + static_cast<std::size_t>(spec[index] - '0');
        ++index;
    }

    if (width <= text.size())
    {
        return text;
    }

    const std::size_t paddingSize{ width - text.size() };
    if (alignCharacter == '<')
    {
        return text + std::string(paddingSize, fillCharacter);
    }
    if (alignCharacter == '^')
    {
        const std::size_t leftPadding{ paddingSize / 2U };
        const std::size_t rightPadding{ paddingSize - leftPadding };
        return std::string(leftPadding, fillCharacter) + text + std::string(rightPadding, fillCharacter);
    }
    return std::string(paddingSize, fillCharacter) + text;
}

template <typename... Args>
[[nodiscard]] std::string format(std::string_view formatString, Args&&... args)
{
    const std::array<std::string, sizeof...(Args)> argumentStrings{ toString(std::forward<Args>(args))... };

    std::string output{};
    output.reserve(formatString.size() + argumentStrings.size() * 8U);

    std::size_t argumentIndex{ 0 };
    for (std::size_t index = 0; index < formatString.size(); ++index)
    {
        const char currentCharacter{ formatString[index] };
        if (currentCharacter == '{')
        {
            if (index + 1U < formatString.size() && formatString[index + 1U] == '{')
            {
                output += '{';
                ++index;
                continue;
            }

            const std::size_t closeIndex{ formatString.find('}', index + 1U) };
            if (closeIndex == std::string_view::npos)
            {
                output += currentCharacter;
                continue;
            }

            std::string_view spec{};
            if (index + 1U < closeIndex && formatString[index + 1U] == ':')
            {
                spec = formatString.substr(index + 2U, closeIndex - index - 2U);
            }

            if (argumentIndex < argumentStrings.size())
            {
                output += applyFormatSpec(argumentStrings[argumentIndex], spec);
                ++argumentIndex;
            }

            index = closeIndex;
            continue;
        }

        if (currentCharacter == '}' && index + 1U < formatString.size() && formatString[index + 1U] == '}')
        {
            output += '}';
            ++index;
            continue;
        }

        output += currentCharacter;
    }

    return output;
}
} // namespace detail

template <typename... Args>
void print(std::string_view fmt, Args&&... args)
{
    std::cout << detail::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void println(std::string_view fmt, Args&&... args)
{
    std::cout << detail::format(fmt, std::forward<Args>(args)...) << '\n';
}

inline void println()
{
    std::cout << '\n';
}
} // namespace asmstudio::compat
#endif // ASM_CXX23

// ---------------------------------------------------------------------------
// format — std::format is C++20; on C++17 we include it anyway since
// Clang/GCC with -std=c++17 may still expose it via <format>.
// If unavailable, callers must guard with #ifdef ASM_CXX20.
// ---------------------------------------------------------------------------
#ifdef ASM_CXX20
#include <format>
namespace asmstudio::compat
{
using std::format;
using std::make_format_args;
using std::vformat;
} // namespace asmstudio::compat
#endif

// ---------------------------------------------------------------------------
// ranges::any_of / erase_if — C++20 ranges vs C++17 iterator algorithms.
// ---------------------------------------------------------------------------
#ifdef ASM_CXX20
#include <algorithm>
#include <ranges>
#include <vector>

namespace asmstudio::compat::ranges
{
using std::ranges::any_of;

// std::erase_if for vector/unordered_map is C++20.
template <typename Container, typename Pred>
auto erase_if(Container& c, Pred&& pred) -> decltype(std::erase_if(c, pred))
{
    return std::erase_if(c, std::forward<Pred>(pred));
}
} // namespace asmstudio::compat::ranges

#else  // C++17 iterator fallbacks

namespace asmstudio::compat::ranges
{
template <typename Range, typename Pred>
[[nodiscard]] bool any_of(Range&& range, Pred&& pred)
{
    return std::any_of(std::begin(range), std::end(range), std::forward<Pred>(pred));
}

template <typename Container, typename Pred>
void erase_if(Container& c, Pred&& pred)
{
    c.erase(std::remove_if(c.begin(), c.end(), std::forward<Pred>(pred)), c.end());
}
} // namespace asmstudio::compat::ranges
#endif // ASM_CXX20


#endif // ASMSTUDIO_SUPPORT_COMPAT_HPP
