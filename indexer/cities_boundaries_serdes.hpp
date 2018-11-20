#pragma once

#include "indexer/city_boundary.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/bounding_box.hpp"
#include "geometry/calipers_box.hpp"
#include "geometry/diamond_box.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/visitor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace indexer
{
template <typename Sink>
class CitiesBoundariesEncoder
{
public:
  struct Visitor
  {
  public:
    Visitor(Sink & sink, serial::GeometryCodingParams const & params)
      : m_sink(sink), m_params(params), m_last(params.GetBasePoint())
    {
    }

    void operator()(m2::PointD const & p) { return (*this)(ToU(p)); }

    void operator()(m2::PointU const & p)
    {
      WriteVarUint(m_sink, coding::EncodePointDeltaAsUint(p, m_last));
      m_last = p;
    }

    void operator()(m2::BoundingBox const & bbox)
    {
      auto const min = ToU(bbox.Min());
      auto const max = ToU(bbox.Max());

      (*this)(min);
      EncodeNonNegativePointDelta(min, max);
    }

    void operator()(m2::CalipersBox const & cbox)
    {
      auto ps = cbox.Points();

      CHECK(!ps.empty(), ());
      CHECK_LESS_OR_EQUAL(ps.size(), 4, ());
      CHECK(ps.size() != 3, ());

      if (ps.size() == 1)
      {
        auto const p0 = ps[0];
        while (ps.size() != 4)
          ps.push_back(p0);
      }
      else if (ps.size() == 2)
      {
        auto const p0 = ps[0];
        auto const p1 = ps[1];

        ps.push_back(p1);
        ps.push_back(p0);
      }

      ASSERT_EQUAL(ps.size(), 4, ());

      auto const us = ToU(ps);

      (*this)(us[0]);
      coding::EncodePointDelta(m_sink, us[0], us[1]);
      coding::EncodePointDelta(m_sink, us[0], us[3]);
    }

    void operator()(m2::DiamondBox const & dbox)
    {
      auto const ps = ToU(dbox.Points());
      auto const base = ps[0];
      auto const next = ps[1];
      auto const prev = ps[3];

      (*this)(base);

      ASSERT_GREATER_OR_EQUAL(next.x, base.x, ());
      ASSERT_GREATER_OR_EQUAL(next.y, base.y, ());
      WriteVarUint(m_sink, next.x >= base.x ? next.x - base.x : 0);

      ASSERT_GREATER_OR_EQUAL(prev.x, base.x, ());
      ASSERT_LESS_OR_EQUAL(prev.y, base.y, ());
      WriteVarUint(m_sink, prev.x >= base.x ? prev.x - base.x : 0);
    }

    template <typename R>
    void operator()(R const & r)
    {
      r.Visit(*this);
    }

  private:
    m2::PointU ToU(m2::PointD const & p) const
    {
      return PointDToPointU(p, m_params.GetCoordBits());
    }

    std::vector<m2::PointU> ToU(std::vector<m2::PointD> const & ps) const
    {
      std::vector<m2::PointU> us(ps.size());
      for (size_t i = 0; i < ps.size(); ++i)
        us[i] = ToU(ps[i]);
      return us;
    }

    // Writes the difference of two 2d vectors to |m_sink|. The vector |next|
    // must have both its coordinates greater than or equal to those
    // of |curr|. While this condition is unlikely when encoding polylines,
    // this method may be useful when encoding rectangles, in particular
    // bounding boxes of shapes.
    void EncodeNonNegativePointDelta(m2::PointU const & curr, m2::PointU const & next)
    {
      ASSERT_GREATER_OR_EQUAL(next.x, curr.x, ());
      ASSERT_GREATER_OR_EQUAL(next.y, curr.y, ());

      // Paranoid checks due to possible floating point artifacts
      // here. In general, next.x >= curr.x and next.y >= curr.y.
      auto const dx = next.x >= curr.x ? next.x - curr.x : 0;
      auto const dy = next.y >= curr.y ? next.y - curr.y : 0;
      WriteVarUint(m_sink, dx);
      WriteVarUint(m_sink, dy);
    }

    Sink & m_sink;
    serial::GeometryCodingParams m_params;
    m2::PointU m_last;
  };

  CitiesBoundariesEncoder(Sink & sink, serial::GeometryCodingParams const & params)
    : m_sink(sink), m_visitor(sink, params)
  {
  }

  void operator()(std::vector<std::vector<CityBoundary>> const & boundaries)
  {
    WriteVarUint(m_sink, boundaries.size());

    {
      BitWriter<Sink> writer(m_sink);
      for (auto const & bs : boundaries)
      {
        CHECK_LESS(bs.size(), std::numeric_limits<uint64_t>::max(), ());
        auto const success =
            coding::GammaCoder::Encode(writer, static_cast<uint64_t>(bs.size()) + 1);
        ASSERT(success, ());
        UNUSED_VALUE(success);
      }
    }

    for (auto const & bs : boundaries)
    {
      for (auto const & b : bs)
        m_visitor(b);
    }
  }

private:
  Sink & m_sink;
  Visitor m_visitor;
};

template <typename Source>
class CitiesBoundariesDecoderV0
{
public:
  struct Visitor
  {
  public:
    Visitor(Source & source, serial::GeometryCodingParams const & params)
      : m_source(source), m_params(params), m_last(params.GetBasePoint())
    {
    }

    void operator()(m2::PointD & p)
    {
      m2::PointU u;
      (*this)(u);
      p = FromU(u);
    }

    void operator()(m2::PointU & p)
    {
      p = coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(m_source), m_last);
      m_last = p;
    }

    void operator()(m2::BoundingBox & bbox)
    {
      m2::PointU min;
      (*this)(min);
      auto const max = DecodeNonNegativePointDelta(min);

      bbox = m2::BoundingBox();
      bbox.Add(FromU(min));
      bbox.Add(FromU(max));
    }

    void operator()(m2::CalipersBox & cbox)
    {
      std::vector<m2::PointU> us(4);
      (*this)(us[0]);
      us[1] = coding::DecodePointDelta(m_source, us[0]);
      us[3] = coding::DecodePointDelta(m_source, us[0]);

      auto ps = FromU(us);
      auto const dp = ps[3] - ps[0];
      ps[2] = ps[1] + dp;

      cbox = m2::CalipersBox(ps);
    }

    void operator()(m2::DiamondBox & dbox)
    {
      m2::PointU base;
      (*this)(base);

      auto const nx = ReadVarUint<uint32_t>(m_source);
      auto const px = ReadVarUint<uint32_t>(m_source);

      dbox = m2::DiamondBox();
      dbox.Add(FromU(base));
      dbox.Add(FromU(base + m2::PointU(nx, nx)));
      dbox.Add(FromU(base + m2::PointU(px, -px)));
      dbox.Add(FromU(base + m2::PointU(nx + px, nx - px)));
    }

    template <typename R>
    void operator()(R & r)
    {
      r.Visit(*this);
    }

  private:
    m2::PointD FromU(m2::PointU const & u) const
    {
      return PointUToPointD(u, m_params.GetCoordBits());
    }

    std::vector<m2::PointD> FromU(std::vector<m2::PointU> const & us) const
    {
      std::vector<m2::PointD> ps(us.size());
      for (size_t i = 0; i < us.size(); ++i)
        ps[i] = FromU(us[i]);
      return ps;
    }

    // Reads the encoded difference from |m_source| and returns the
    // point equal to |base| + difference. It is guaranteed that
    // both coordinates of difference are non-negative.
    m2::PointU DecodeNonNegativePointDelta(m2::PointU const & base)
    {
      auto const dx = ReadVarUint<uint32_t>(m_source);
      auto const dy = ReadVarUint<uint32_t>(m_source);
      return m2::PointU(base.x + dx, base.y + dy);
    }

    Source & m_source;
    serial::GeometryCodingParams const m_params;
    m2::PointU m_last;
  };

  CitiesBoundariesDecoderV0(Source & source, serial::GeometryCodingParams const & params)
    : m_source(source), m_visitor(source, params)
  {
  }

  void operator()(std::vector<std::vector<CityBoundary>> & boundaries)
  {
    auto const size = ReadVarUint<uint64_t>(m_source);
    boundaries.resize(size);

    {
      BitReader<Source> reader(m_source);
      for (auto & bs : boundaries)
      {
        auto const size = coding::GammaCoder::Decode(reader);
        ASSERT_GREATER_OR_EQUAL(size, 1, ());
        bs.resize(size - 1);
      }
    }

    for (auto & bs : boundaries)
    {
      for (auto & b : bs)
        m_visitor(b);
    }
  }

private:
  Source & m_source;
  Visitor m_visitor;
};

struct CitiesBoundariesSerDes
{
  template <typename Sink>
  struct WriteToSinkVisitor
  {
    WriteToSinkVisitor(Sink & sink) : m_sink(sink) {}

    template <typename T>
    std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, void> operator()(
        T const & t, char const * /* name */ = nullptr)
    {
      WriteToSink(m_sink, t);
    }

    template <typename T>
    std::enable_if_t<!std::is_integral<T>::value && !std::is_enum<T>::value, void> operator()(
        T const & t, char const * /* name */ = nullptr)
    {
      t.Visit(*this);
    }

    Sink & m_sink;
  };

  template <typename Source>
  struct ReadFromSourceVisitor
  {
    ReadFromSourceVisitor(Source & source) : m_source(source) {}

    template <typename T>
    std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, void> operator()(
        T & t, char const * /* name */ = nullptr)
    {
      t = ReadPrimitiveFromSource<T>(m_source);
    }

    template <typename T>
    std::enable_if_t<!std::is_integral<T>::value && !std::is_enum<T>::value, void> operator()(
        T & t, char const * /* name */ = nullptr)
    {
      t.Visit(*this);
    }

    Source & m_source;
  };

  static uint8_t constexpr kLatestVersion = 0;

  struct HeaderV0
  {
    static uint8_t const kDefaultCoordBits = 19;

    HeaderV0() {}

    DECLARE_VISITOR(visitor(m_coordBits, "coordBits"))

    uint8_t m_coordBits = kDefaultCoordBits;
  };

  template <typename Sink>
  static void Serialize(Sink & sink, std::vector<std::vector<CityBoundary>> const & boundaries)
  {
    uint8_t const version = kLatestVersion;

    WriteToSinkVisitor<Sink> visitor(sink);
    visitor(version);

    HeaderV0 const header;
    visitor(header);

    serial::GeometryCodingParams const params(
        header.m_coordBits, m2::PointD(MercatorBounds::kMinX, MercatorBounds::kMinY));
    CitiesBoundariesEncoder<Sink> encoder(sink, params);
    encoder(boundaries);
  }

  template <typename Source>
  static void Deserialize(Source & source, std::vector<std::vector<CityBoundary>> & boundaries,
                          double & precision)
  {
    ReadFromSourceVisitor<Source> visitor(source);

    uint8_t version;
    visitor(version);

    CHECK_EQUAL(version, 0, ());

    HeaderV0 header;
    visitor(header);

    auto const wx = MercatorBounds::kRangeX;
    auto const wy = MercatorBounds::kRangeY;
    precision = std::max(wx, wy) / pow(2, header.m_coordBits);

    serial::GeometryCodingParams const params(
        header.m_coordBits, m2::PointD(MercatorBounds::kMinX, MercatorBounds::kMinY));
    CitiesBoundariesDecoderV0<Source> decoder(source, params);
    decoder(boundaries);
  }
};
}  // namespace indexer
