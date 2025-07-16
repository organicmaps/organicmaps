#pragma once

#include "coding/endianness.hpp"

#include <cstdint>
#include <type_traits>

namespace traits
{
namespace impl
{
template <typename T>
auto has_arrow_operator_checker(int) -> decltype(std::declval<T &>().operator->(), std::true_type{});

template <typename T>
std::false_type has_arrow_operator_checker(...);
}  // namespace impl

template <typename T>
using has_arrow_operator = decltype(impl::has_arrow_operator_checker<T>(0));
}  // namespace traits

template <class Sink, typename T>
std::enable_if_t<!traits::has_arrow_operator<Sink>::value && (std::is_integral<T>::value || std::is_enum<T>::value),
                 void>
WriteToSink(Sink & sink, T const & v)
{
  T const t = SwapIfBigEndianMacroBased(v);
  sink.Write(&t, sizeof(T));
}

template <class Sink, typename T>
std::enable_if_t<traits::has_arrow_operator<Sink>::value && (std::is_integral<T>::value || std::is_enum<T>::value),
                 void>
WriteToSink(Sink & sink, T const & v)
{
  T const t = SwapIfBigEndianMacroBased(v);
  sink->Write(&t, sizeof(T));
}

template <class Sink>
void WriteZeroesToSink(Sink & sink, uint64_t size)
{
  uint8_t const zeroes[256] = {0};
  for (uint64_t i = 0; i < (size >> 8); ++i)
    sink.Write(zeroes, 256);
  sink.Write(zeroes, size & 255);
}

template <typename Sink>
class WriterFunctor
{
public:
  explicit WriterFunctor(Sink & sink) : m_sink(sink) {}

  template <typename T>
  void operator()(T const & t) const
  {
    m_sink.Write(&t, sizeof(T));
  }

private:
  Sink & m_sink;
};
