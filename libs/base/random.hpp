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
  std::uniform_int_distribution<T> m_distr;

public:
  UniformRandom(T min, T max) : m_gen(m_rd()), m_distr(min, max) {}
  UniformRandom() : UniformRandom(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) {}

  T operator()() { return m_distr(m_gen); }
};
}  // namespace base
