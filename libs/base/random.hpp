#pragma once

#include <limits>
#include <random>

namespace base
{
template <class T>
class UniformRandom
{
  static_assert(std::is_integral<T>::value);

  std::random_device m_rd;
  std::mt19937 m_gen;

  using distribution_int_type =
      std::conditional_t<sizeof(T) != 1, T, std::conditional_t<std::is_signed_v<T>, short, unsigned short>>;
  std::uniform_int_distribution<distribution_int_type> m_distr;

public:
  UniformRandom(T min, T max) : m_gen(m_rd()), m_distr(min, max) {}
  UniformRandom() : UniformRandom(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) {}

  T operator()() { return static_cast<T>(m_distr(m_gen)); }
};
}  // namespace base
