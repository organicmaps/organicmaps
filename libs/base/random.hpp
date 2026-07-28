#pragma once

#include <limits>
#include <random>
#include <type_traits>

#include "base/macros.hpp"

namespace base
{
namespace impl
{
template <class T>
using DistributionIntT = std::conditional_t<
    std::is_signed_v<T>,
    std::conditional_t<sizeof(T) <= sizeof(short), short,
                       std::conditional_t<sizeof(T) <= sizeof(int), int,
                                          std::conditional_t<sizeof(T) <= sizeof(long), long, long long>>>,
    std::conditional_t<
        sizeof(T) <= sizeof(unsigned short), unsigned short,
        std::conditional_t<sizeof(T) <= sizeof(unsigned int), unsigned int,
                           std::conditional_t<sizeof(T) <= sizeof(unsigned long), unsigned long, unsigned long long>>>>;
}  // namespace impl

template <class T>
class UniformRandom
{
  static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "T must be cv-unqualified");
  static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
  static_assert(std::is_integral_v<T>
                    ? sizeof(T) <= sizeof(long long)
                    : std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, long double>,
                "128-bit integers and extended floating point types are not supported");

  DISALLOW_COPY(UniformRandom);

  std::mt19937 m_gen;

  using ValueT = std::conditional_t<std::is_integral_v<T>, impl::DistributionIntT<T>, T>;
  using DistributionT = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<ValueT>,
                                           std::uniform_real_distribution<ValueT>>;
  DistributionT m_distr;

public:
  UniformRandom(T min, T max) : m_gen(std::random_device{}()), m_distr(min, max) {}
  UniformRandom()
    : UniformRandom(std::is_floating_point_v<T> ? T{0} : std::numeric_limits<T>::lowest(),
                    std::is_floating_point_v<T> ? T{1} : std::numeric_limits<T>::max())
  {}

  void SetLimits(T min, T max) { m_distr = DistributionT(min, max); }

  T operator()() { return static_cast<T>(m_distr(m_gen)); }
};
}  // namespace base
