#pragma once

#include "routing_common/transit_types.hpp"

#include "geometry/point2d.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/macros.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace routing
{
namespace transit
{
// Note. For the time being double at transit section is used only for saving weight of edges (in seconds).
// Let us assume that it takes less than 10^7 seconds (115 days) to get from one station to a neighboring one.
double constexpr kMinDoubleAtTransit = kInvalidWeight;
double constexpr kMaxDoubleAtTransit = 10000000.0;
uint32_t constexpr kDoubleBits = 31;

template <typename Sink>
class Serializer
{
public:
  Serializer(Sink & sink) : m_sink(sink) {}

  template<typename T>
  typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type
      operator()(T const & t, char const * /* name */ = nullptr)
  {
    WriteToSink(m_sink, t);
  }

  void operator()(double d, char const * name = nullptr)
  {
    CHECK_GREATER_OR_EQUAL(d, kMinDoubleAtTransit, ());
    CHECK_LESS_OR_EQUAL(d, kMaxDoubleAtTransit, ());
    (*this)(DoubleToUint32(d, kMinDoubleAtTransit, kMaxDoubleAtTransit, kDoubleBits), name);
  }

  void operator()(std::string const & s, char const * /* name */ = nullptr)
  {
    rw::Write(m_sink, s);
  }

  void operator()(m2::PointD const & p, char const * /* name */ = nullptr)
  {
    m2::PointU const pointU = PointD2PointU(p, POINT_COORD_BITS);
    WriteVarUint(m_sink, pointU.x);
    WriteVarUint(m_sink, pointU.y);
  }

  template <typename T>
  void operator()(std::vector<T> const & vs, char const * /* name */ = nullptr)
  {
    CHECK_LESS_OR_EQUAL(vs.size(), std::numeric_limits<uint64_t>::max(), ());
    WriteVarUint(m_sink, static_cast<uint64_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

  template<typename T>
  typename std::enable_if<std::is_class<T>::value>::type operator()(T const & t, char const * /* name */ = nullptr)
  {
    t.Visit(*this);
  }

private:
  Sink & m_sink;
};

template <typename Source>
class Deserializer
{
public:
  Deserializer(Source & source) : m_source(source) {}

  template<typename T>
  typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type
      operator()(T & t, char const * name = nullptr)
  {
    ReadPrimitiveFromSource(m_source, t);
  }

  void operator()(double & d, char const * name = nullptr)
  {
    uint32_t ui;
    (*this)(ui, name);
    d = Uint32ToDouble(ui, kMinDoubleAtTransit, kMaxDoubleAtTransit, kDoubleBits);
  }

  void operator()(std::string & s, char const * /* name */ = nullptr)
  {
    rw::Read(m_source, s);
  }

  void operator()(m2::PointD & p, char const * /* name */ = nullptr)
  {
    m2::PointU pointU;
    pointU.x = ReadVarUint<uint32_t, Source>(m_source);
    pointU.y = ReadVarUint<uint32_t, Source>(m_source);
    p = PointU2PointD(pointU, POINT_COORD_BITS);
  }

  template <typename T>
  void operator()(vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const size = ReadVarUint<uint64_t, Source>(m_source);
    vs.resize(size);
    for (auto & v : vs)
      (*this)(v);
  }

  template<typename T>
  typename std::enable_if<std::is_class<T>::value>::type operator()(T & t, char const * /* name */ = nullptr)
  {
    t.Visit(*this);
  }

private:
  Source & m_source;
};
}  // namespace transit
}  // namespace routing
