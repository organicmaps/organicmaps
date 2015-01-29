// Fixed-sized, zero-padded serialization and de-serialization for unsigned types of two or more bytes.
//
// Ported into Bricks from TailProduce.

#ifndef BRICKS_STRINGS_FIXED_SIZE_SERIALIZER_H
#define BRICKS_STRINGS_FIXED_SIZE_SERIALIZER_H

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

namespace bricks {
namespace strings {

struct FixedSizeSerializerEnabler {};
template <typename T>
struct FixedSizeSerializer
    : std::enable_if<std::is_unsigned<T>::value && std::is_integral<T>::value&&(sizeof(T) > 1),
                     FixedSizeSerializerEnabler>::type {
  static constexpr size_t size_in_bytes = std::numeric_limits<T>::digits10 + 1;
  static std::string PackToString(T x) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(size_in_bytes) << x;
    return os.str();
  }
  static T UnpackFromString(std::string const& s) {
    T x;
    std::istringstream is(s);
    is >> x;
    return x;
  }
};

// To allow implicit type specialization wherever possible.
template <typename T>
inline std::string PackToString(T x) {
  return FixedSizeSerializer<T>::PackToString(x);
}
template <typename T>
inline const T& UnpackFromString(std::string const& s, T& x) {
  x = FixedSizeSerializer<T>::UnpackFromString(s);
  return x;
}

}  // namespace string
}  // namespace bricks

#endif  // BRICKS_STRINGS_FIXED_SIZE_SERIALIZER_H
