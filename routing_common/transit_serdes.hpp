#pragma once

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
#include <vector>

namespace routing
{
namespace transit
{
template <typename Sink>
class Serializer
{
public:
  Serializer(Sink & sink) : m_sink(sink) {}

  void operator()(uint8_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(uint16_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(uint32_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(uint64_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(double const d, char const * /* name */ = nullptr)
  {
    NOTIMPLEMENTED();
  }

  void operator()(std::string const & s, char const * /* name */ = nullptr)
  {
    rw::Write(m_sink, s);
  }

  void operator()(m2::PointD const & p, char const * /* name */ = nullptr)
  {
    m2::PointU const pointU = PointD2PointU(p, POINT_COORD_BITS);
    (*this)(pointU.x);
    (*this)(pointU.y);
  }

  template <typename R>
  void operator()(R const & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  template <typename T>
  void operator()(std::vector<T> const & vs, char const * /* name */ = nullptr)
  {
    CHECK_LESS(vs.size(), std::numeric_limits<uint32_t>::max(), ());
    WriteVarUint(m_sink, static_cast<uint32_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

private:
  Sink & m_sink;
};

template <typename Source>
class Deserializer
{
public:
  Deserializer(Source & source) : m_source(source) {}

  void operator()(uint8_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }

  void operator()(uint16_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }

  void operator()(uint32_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }

  void operator()(uint64_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }

  void operator()(double & d, char const * /* name */ = nullptr)
  {
    NOTIMPLEMENTED();
  }

  void operator()(std::string & s, char const * /* name */ = nullptr)
  {
    rw::Read(m_source, s);
  }

  void operator()(m2::PointD & p, char const * /* name */ = nullptr)
  {
    m2::PointU pointU;
    (*this)(pointU.x);
    (*this)(pointU.y);
    p = PointU2PointD(pointU, POINT_COORD_BITS);
  }


  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  template <typename T>
  void operator()(vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const size = ReadVarUint<uint32_t, Source>(m_source);
    vs.resize(size);
    for (auto & v : vs)
      (*this)(v);
  }

private:
  Source & m_source;
};
}  // namespace transit
}  // namespace routing
