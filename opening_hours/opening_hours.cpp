
#include "opening_hours.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
template <typename T, typename SeparatorExtractor>
void PrintVector(std::ostream & ost, std::vector<T> const & v, SeparatorExtractor && sepFunc)
{
  auto it = begin(v);
  if (it == end(v))
    return;

  auto sep = sepFunc(*it);
  ost << *it++;
  while (it != end(v))
  {
    ost << sep << *it;
    sep = sepFunc(*it);
    ++it;
  }
}

template <typename T>
void PrintVector(std::ostream & ost, std::vector<T> const & v, char const * const sep = ", ")
{
  PrintVector(ost, v, [&sep](T const &) { return sep; });
}

class StreamFlagsKeeper
{
public:
  explicit StreamFlagsKeeper(std::ostream & ost)
    : m_ost(ost), m_flags(m_ost.flags())
  {
  }

  ~StreamFlagsKeeper()
  {
    m_ost.flags(m_flags);
  }

private:
  std::ostream & m_ost;
  std::ios_base::fmtflags m_flags;
};

template <typename TNumber>
void PrintPaddedNumber(std::ostream & ost, TNumber const number, uint32_t const padding = 1)
{
  static constexpr bool isChar = std::is_same_v<signed char, TNumber> ||
                                 std::is_same_v<unsigned char, TNumber> ||
                                 std::is_same_v<char, TNumber>;

  if constexpr (isChar)
  {
    PrintPaddedNumber(ost, static_cast<int32_t>(number), padding);
  }
  else
  {
    static_assert(std::is_integral<TNumber>::value, "number should be of integral type.");
    StreamFlagsKeeper keeper(ost);
    ost << std::setw(padding) << std::setfill('0') << number;
  }
}

}  // namespace

namespace om::opening_hours
{

bool HourMinutes::IsExtended() const
{
  return GetDuration() > 24_h;
}

void HourMinutes::SetHours(THours const hours)
{
  m_empty = false;
  m_hours = hours;
}

void HourMinutes::SetMinutes(TMinutes const minutes)
{
  m_empty = false;
  m_minutes = minutes;
}

void HourMinutes::SetDuration(TMinutes const duration)
{
  SetHours(std::chrono::duration_cast<THours>(duration));
  SetMinutes(duration - GetHours());
}

void PrintHoursMinutes(std::ostream & ost, std::chrono::hours::rep hours, std::chrono::minutes::rep minutes)
{
  PrintPaddedNumber(ost, hours, 2);
  ost << ':';
  PrintPaddedNumber(ost, minutes, 2);
}

HourMinutes operator-(HourMinutes const & hm)
{
  HourMinutes result;
  result.SetHours(-hm.GetHours());
  result.SetMinutes(-hm.GetMinutes());
  return result;
}

std::ostream & operator<<(std::ostream & ost, HourMinutes const & hm)
{
  if (hm.IsEmpty())
    ost << "hh:mm";
  else
    PrintHoursMinutes(ost, std::abs(hm.GetHoursCount()), std::abs(hm.GetMinutesCount()));
  return ost;
}

}  // namespace om::opening_hours
