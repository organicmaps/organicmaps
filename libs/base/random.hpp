#pragma once

#include <limits>
#include <random>

namespace base
{
template <class T>
class UniformRandom
{
  static_assert(std::is_arithmetic_v<T>);

  std::mt19937 m_gen;

  using ValueT = std::conditional_t<sizeof(T) != 1, T, std::conditional_t<std::is_signed_v<T>, short, unsigned short>>;
  using DistributionT = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<ValueT>,
                                           std::uniform_real_distribution<ValueT>>;
  DistributionT m_distr;

public:
  UniformRandom(T min, T max) : m_gen(std::random_device{}()), m_distr(min, max) {}
  UniformRandom() : UniformRandom(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max()) {}

  void SetLimits(T min, T max) { m_distr = DistributionT(min, max); }

  T operator()() { return static_cast<T>(m_distr(m_gen)); }
};
}  // namespace base
