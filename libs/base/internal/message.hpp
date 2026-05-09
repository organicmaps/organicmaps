#pragma once

#include <algorithm>
#include <array>
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

/// @name Declarations.
//@{
template <typename T>
inline std::string DebugPrint(T const & t);

inline std::string DebugPrint(std::string s)
{
  return s;
}
inline std::string DebugPrint(std::string_view s)
{
  return std::string(s);
}
inline std::string DebugPrint(char const * t);
inline std::string DebugPrint(char * t)
{
  return DebugPrint(static_cast<char const *>(t));
}
inline std::string DebugPrint(char t);
inline std::string DebugPrint(char32_t t);

/// @name We are going step-by-step to C++20. Use UTF8 string literals instead.
/// @{
std::string DebugPrint(char16_t const * t) = delete;
std::string DebugPrint(char16_t * t) = delete;
std::string DebugPrint(char32_t const * t) = delete;
std::string DebugPrint(char32_t * t) = delete;
/// @}

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
//@}

template <typename T>
inline std::string DebugPrint(T const & t)
{
  std::ostringstream out;
  out << t;
  return out.str();
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

namespace internal
{
std::string ToUtf8(std::u16string_view utf16);
std::string ToUtf8(std::u32string_view utf32);
}  // namespace internal

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
template <typename T>
std::string DebugPrintToString(T && t)
{
  using ::DebugPrint;
  auto result = DebugPrint(std::forward<T>(t));
  if constexpr (std::is_same_v<decltype(result), std::string>)
    return result;
  else
    return std::string(result);
}
}  // namespace internal

template <typename... Args>
std::string Message(Args &&... args)
{
  if constexpr (sizeof...(args) == 0)
    return {};
  else if constexpr (sizeof...(args) == 1)
    return internal::DebugPrintToString(std::forward<Args>(args)...);
  else
  {
    std::array parts{internal::DebugPrintToString(std::forward<Args>(args))...};
    size_t total = parts.size() - 1;  // single-space separators
    for (auto const & p : parts)
      total += p.size();
    std::string out;
    out.reserve(total);
    out += parts[0];
    for (size_t i = 1; i < parts.size(); ++i)
    {
      out += ' ';
      out += parts[i];
    }
    return out;
  }
}
}  // namespace base
