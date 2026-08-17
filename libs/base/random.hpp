#pragma once

#include <concepts>
#include <limits>
#include <random>
#include <type_traits>

namespace base
{
namespace impl
{
template <typename T>
concept StandardFloatingPoint = std::same_as<T, float> || std::same_as<T, double> || std::same_as<T, long double>;

template <typename T>
concept UniformRandomValue = std::same_as<T, std::remove_cvref_t<T>> &&
                             ((std::integral<T> && sizeof(T) <= sizeof(long long)) || StandardFloatingPoint<T>);

template <typename T, typename Signed, typename Unsigned>
using MatchSignedness = std::conditional_t<std::is_signed_v<T>, Signed, Unsigned>;

// uniform_int_distribution takes only short/int/long/long long and their unsigned counterparts, so
// map every other integer (bool, char, int8_t, char16_t, wchar_t, ...) onto one of them. Staying as
// narrow as possible is not cosmetic: libc++ rebuilds its bit engine on every draw and sizes that
// arithmetic after the distribution type, so a 64-bit one costs ~30% more per draw.
template <typename T>
using DistributionIntT = std::conditional_t<sizeof(T) <= sizeof(int), MatchSignedness<T, int, unsigned int>,
                                            MatchSignedness<T, long long, unsigned long long>>;
}  // namespace impl

template <impl::UniformRandomValue T>
class UniformRandom
{
  std::mt19937 m_gen;

  using ValueT = std::conditional_t<std::is_integral_v<T>, impl::DistributionIntT<T>, T>;
  using DistributionT = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<ValueT>,
                                           std::uniform_real_distribution<ValueT>>;
  DistributionT m_distr;

public:
  UniformRandom(T min, T max) : m_gen(std::random_device{}()), m_distr(min, max) {}
  // uniform_real_distribution requires max - min <= numeric_limits<T>::max(), so the whole floating
  // point range is not a valid default and yields infinity instead of a number.
  UniformRandom()
    : UniformRandom(std::is_floating_point_v<T> ? T{0} : std::numeric_limits<T>::lowest(),
                    std::is_floating_point_v<T> ? T{1} : std::numeric_limits<T>::max())
  {}

  void SetLimits(T min, T max) { m_distr = DistributionT(min, max); }

  T operator()() { return static_cast<T>(m_distr(m_gen)); }
};
}  // namespace base
