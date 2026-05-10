#pragma once

#include "base/float_to_string.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <ctime>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

inline std::string DebugPrint(std::string s)
{
  return s;
}
inline std::string DebugPrint(std::string_view s)
{
  return std::string(s);
}

inline std::string DebugPrint(char const * t);  // Forward decl: called from DebugPrint(char *) below.
inline std::string DebugPrint(char * t)
{
  return DebugPrint(static_cast<char const *>(t));
}

// We are going step-by-step to C++20. Use UTF8 string literals instead.
std::string DebugPrint(char16_t const * t) = delete;
std::string DebugPrint(char16_t * t) = delete;
std::string DebugPrint(char32_t const * t) = delete;
std::string DebugPrint(char32_t * t) = delete;

// Forward declarations: pair / optional / DebugPrintSequence are defined before the STL container
// overloads but call DebugPrint on element types — these decls make the container overloads
// candidates at parse time so two-phase name lookup picks them at instantiation (ADL on std::
// types alone wouldn't find ::DebugPrint).
template <typename U, typename V>
inline std::string DebugPrint(std::pair<U, V> const & p);
template <typename T>
inline std::string DebugPrint(std::list<T> const & v);
template <typename T>
inline std::string DebugPrint(std::vector<T> const & v);
template <typename T, typename C = std::less<T>>
inline std::string DebugPrint(std::set<T, C> const & v);
template <typename T, typename C = std::less<T>>
inline std::string DebugPrint(std::multiset<T, C> const & v);
template <typename U, typename V, typename C = std::less<U>>
inline std::string DebugPrint(std::map<U, V, C> const & v);
template <typename T>
inline std::string DebugPrint(std::initializer_list<T> const & v);
template <typename T>
inline std::string DebugPrint(std::unique_ptr<T> const & v);
template <class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>>
inline std::string DebugPrint(std::unordered_set<Key, Hash, Pred> const & v);
template <class Key, class T, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>>
inline std::string DebugPrint(std::unordered_map<Key, T, Hash, Pred> const & v);

namespace internal
{
std::string ToUtf8(std::u16string_view utf16);
std::string ToUtf8(std::u32string_view utf32);

// Subset of integers that std::to_chars formats as a decimal number — bool and the character
// types are excluded so they keep operator<<'s character-printing semantics. Floating-point types
// route to base::FloatToString instead because std::to_chars for float/double is not available
// on libc++ before iOS 16.3 / macOS 13.3 (the project supports iOS 15+).
template <typename T>
concept Integer =
    std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<std::remove_cv_t<T>, char> &&
    !std::is_same_v<std::remove_cv_t<T>, signed char> && !std::is_same_v<std::remove_cv_t<T>, unsigned char> &&
    !std::is_same_v<std::remove_cv_t<T>, wchar_t> && !std::is_same_v<std::remove_cv_t<T>, char8_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char16_t> && !std::is_same_v<std::remove_cv_t<T>, char32_t>;
}  // namespace internal

// Generic fallback. Floating-point types use base::FloatToString (locale-free, no std::to_chars
// dependency for floats — required for iOS 15 compatibility). Integers go through std::to_chars
// directly — locale-free and allocation-free into a stack buffer. Everything else falls back to
// std::ostringstream for compatibility with legacy types streamable via operator<< —
// ostringstream is heavyweight (constructs a full std::locale per call), so types that care about
// cost should provide an explicit DebugPrint overload.
template <typename T>
inline std::string DebugPrint(T const & t)
{
  if constexpr (std::is_floating_point_v<T>)
  {
    return base::FloatToString(t);
  }
  else if constexpr (internal::Integer<T>)
  {
    char buf[32];
    auto [end, _] = std::to_chars(buf, buf + sizeof(buf), t);
    return std::string{buf, end};
  }
  else
  {
    std::ostringstream out;
    out << t;
    return out.str();
  }
}

inline std::string DebugPrint(char const * t)
{
  if (t)
    return {t};
  return {"NULL string pointer"};
}

inline std::string DebugPrint(char t)
{
  // return {1, t} wrongly constructs "\0x1t" string.
  return std::string(1, t);
}

inline std::string DebugPrint(std::u16string const & utf16)
{
  return internal::ToUtf8(utf16);
}

inline std::string DebugPrint(std::u16string_view utf16)
{
  return internal::ToUtf8(utf16);
}

inline std::string DebugPrint(std::u32string const & utf32)
{
  return internal::ToUtf8(utf32);
}

inline std::string DebugPrint(std::u32string_view utf32)
{
  return internal::ToUtf8(utf32);
}

inline std::string DebugPrint(char32_t t)
{
  return internal::ToUtf8(std::u32string_view{&t, 1});
}

inline std::string DebugPrint(std::chrono::time_point<std::chrono::system_clock> const & ts)
{
  auto t = std::chrono::system_clock::to_time_t(ts);
  std::string str = std::ctime(&t);
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  return str;
}

template <typename U, typename V>
std::string DebugPrint(std::pair<U, V> const & p)
{
  std::string out = "(";
  out += DebugPrint(p.first);
  out += ", ";
  out += DebugPrint(p.second);
  out += ')';
  return out;
}

template <typename IterT>
std::string DebugPrintSequence(IterT beg, IterT end)
{
  std::string out = "[";
  out += std::to_string(std::distance(beg, end));
  out += ':';
  for (; beg != end; ++beg)
  {
    out += ' ';
    out += DebugPrint(*beg);
  }
  out += " ]";
  return out;
}

template <typename T>
std::string DebugPrint(std::optional<T> const & p)
{
  if (!p)
    return "nullopt";
  std::string out = "optional(";
  out += DebugPrint(*p);
  out += ')';
  return out;
}

std::string inline DebugPrint(std::nullopt_t const & p)
{
  return "nullopt";
}

// Avoid calling it for string literals.
template <typename T, size_t N,
          typename = std::enable_if_t<!std::is_same<typename std::remove_cv<T>::type, char>::value &&
                                      !std::is_same<typename std::remove_cv<T>::type, char16_t>::value &&
                                      !std::is_same<typename std::remove_cv<T>::type, char32_t>::value>>
inline std::string DebugPrint(T (&arr)[N])
{
  return DebugPrintSequence(arr, arr + N);
}

template <typename T, size_t N>
inline std::string DebugPrint(std::array<T, N> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T>
inline std::string DebugPrint(std::vector<T> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T>
inline std::string DebugPrint(std::deque<T> const & d)
{
  return DebugPrintSequence(d.begin(), d.end());
}

template <typename T>
inline std::string DebugPrint(std::list<T> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T, typename C>
inline std::string DebugPrint(std::set<T, C> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T, typename C>
inline std::string DebugPrint(std::multiset<T, C> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename U, typename V, typename C>
inline std::string DebugPrint(std::map<U, V, C> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T>
inline std::string DebugPrint(std::initializer_list<T> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <class Key, class Hash, class Pred>
inline std::string DebugPrint(std::unordered_set<Key, Hash, Pred> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <class Key, class T, class Hash, class Pred>
inline std::string DebugPrint(std::unordered_map<Key, T, Hash, Pred> const & v)
{
  return DebugPrintSequence(v.begin(), v.end());
}

template <typename T>
inline std::string DebugPrint(std::unique_ptr<T> const & v)
{
  if (v)
    return std::string(DebugPrint(*v));
  return "null";
}

namespace base
{
namespace internal
{
// Append one Message argument to `out` without an intermediate std::string for the cheap cases:
//   - std::string / std::string_view — direct view append (zero alloc, embedded nulls preserved)
//   - char* / char const* / char[N]  — null-safe view append (zero alloc)
//   - everything else                 — call ::DebugPrint(v), append its result through a
//                                       std::string_view; the temporary is read once and dies
//
// Combined with std::to_chars in the generic ::DebugPrint(T const &) fallback above, a typical
// N-argument LOG/CHECK message allocates once (for the output buffer's reserve) when each
// DebugPrint result fits SSO, regardless of N.
template <typename T>
inline void StreamPart(std::string & out, T const & v)
{
  using D = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<D, std::string> || std::is_same_v<D, std::string_view>)
  {
    out += v;
  }
  else if constexpr (std::is_convertible_v<T const &, char const *>)
  {
    char const * p = v;
    out += p ? std::string_view{p} : std::string_view{"NULL"};
  }
  else
  {
    using ::DebugPrint;
    out += std::string_view{DebugPrint(v)};
  }
}
}  // namespace internal

[[nodiscard]] inline std::string Message()
{
  return {};
}

template <typename First, typename... Rest>
[[nodiscard]] inline std::string Message(First const & first, Rest const &... rest)
{
  std::string out;
  // Reserve enough for the common case (short labels and a few values). std::string's geometric
  // growth handles longer messages with O(log N) realloc cost.
  out.reserve(64);
  internal::StreamPart(out, first);
  ((out += ' ', internal::StreamPart(out, rest)), ...);
  return out;
}
}  // namespace base
