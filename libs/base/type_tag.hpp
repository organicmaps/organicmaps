#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

namespace base
{
namespace impl
{
/**
 * @brief Computes the 64-bit FNV-1a hash of a given string at compile time.
 *
 * This function implements the Fowler–Noll–Vo (FNV-1a) hash algorithm for
 * 64-bit values. It is marked `consteval`, ensuring that the hash is computed
 * entirely at compile time when given a compile-time constant string.
 *
 * @param str The input string to hash.
 * @return A 64-bit unsigned integer containing the FNV-1a hash of the input.
 *
 * @note FNV-1a is a non-cryptographic hash function designed for speed and
 *       good distribution over small inputs. It is not suitable for
 *       security-sensitive purposes.
 */
consteval std::uint64_t fnv1a_hash(std::string_view const str) noexcept
{
  std::uint64_t hash = 14695981039346656037ull;
  for (char const c : str)
  {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}
}  // namespace impl

/**
 * @brief Provides a unique, compile-time constant identifier for a given type.
 *
 * This template generates a type-specific identifier by hashing the
 * compiler-generated function signature that contains the type's name.
 * Since the function signature is unique for each type `T`, the resulting
 * identifier is guaranteed to be unique within a given compiler and set of build options.
 *
 * @tparam T The type for which a unique identifier is generated.
 *
 * @note This method uses a compile-time FNV-1a hash of `__PRETTY_FUNCTION__`
 *       (GCC/Clang) or `__FUNCSIG__` (MSVC). The ID is stable within the same
 *       compiler and build configuration but may change across compilers or
 *       compiler versions.
 *
 * @warning The generated ID is not meant for persistence or cross-build
 *          communication. Its uniqueness is guaranteed only at compile time
 *          for the given translation unit and build settings.
 *
 * Example usage:
 * @code
 * consteval std::uint64_t circleId = base::TypeTag<CircleVertex>::id();
 * consteval std::uint64_t squareId = base::TypeTag<SquareVertex>::id();
 * // circleId != squareId, evaluated at compile time
 * @endcode
 */
template <typename T>
struct TypeTag
{
  /// Generates a unique ID by hashing the compiler-generated function signature
  template <std::integral Ret = std::uint64_t>
  static consteval Ret id()
  {
#if defined(__clang__) || defined(__GNUC__)
    return static_cast<Ret>(impl::fnv1a_hash(__PRETTY_FUNCTION__));
#elif defined(_MSC_VER)
    return static_cast<Ret>(impl::fnv1a_hash(__FUNCSIG__));
#else
#error Unsupported compiler
#endif
  }
};
}  // namespace base
