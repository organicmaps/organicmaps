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
#include "base/macros.hpp"

#include <algorithm>
#include <cmath>
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
      : m_sink(sink)
      , m_params(params)
      , m_last(params.GetBasePoint())
    {}

    void Save(m2::PointU const & p) { WriteVarUint(m_sink, coding::EncodePointDeltaAsUint(p, m_last)); }
    void SaveWithLast(m2::PointU const & p)
    {
      Save(p);
      m_last = p;
    }

    void operator()(m2::BoundingBox const & bbox)
    {
      auto const min = ToU(bbox.Min());
      auto const max = ToU(bbox.Max());

      SaveWithLast(min);
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

      SaveWithLast(us[0]);
      Save(us[1]);
      Save(us[3]);
    }

    void operator()(m2::DiamondBox const & dbox)
    {
      auto const ps = ToU(dbox.Points());
      auto const base = ps[0];
      auto const next = ps[1];
      auto const prev = ps[3];

      SaveWithLast(base);

      ASSERT_GREATER_OR_EQUAL(next.x, base.x, ());
      ASSERT_GREATER_OR_EQUAL(next.y, base.y, ());
      auto const nx = next.x >= base.x ? next.x - base.x : 0;

      ASSERT_GREATER_OR_EQUAL(prev.x, base.x, ());
      ASSERT_LESS_OR_EQUAL(prev.y, base.y, ());
      auto const px = prev.x >= base.x ? prev.x - base.x : 0;

      WriteVarUint(m_sink, bits::BitwiseMerge(nx, px));
    }

    template <typename R>
    void operator()(R const & r)
    {
      r.Visit(*this);
    }

  private:
    m2::PointU ToU(m2::PointD const & p) const { return PointDToPointU(p, m_params.GetCoordBits()); }

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
      WriteVarUint(m_sink, bits::BitwiseMerge(dx, dy));
    }

    Sink & m_sink;
    serial::GeometryCodingParams m_params;
    m2::PointU m_last;
  };

  CitiesBoundariesEncoder(Sink & sink, serial::GeometryCodingParams const & params)
    : m_sink(sink)
    , m_visitor(sink, params)
  {}

  void operator()(std::vector<std::vector<CityBoundary>> const & boundaries)
  {
    WriteVarUint(m_sink, boundaries.size());

    {
      BitWriter<Sink> writer(m_sink);
      for (auto const & bs : boundaries)
      {
        CHECK_LESS(bs.size(), std::numeric_limits<uint64_t>::max(), ());
        auto const success = coding::GammaCoder::Encode(writer, static_cast<uint64_t>(bs.size()) + 1);
        ASSERT(success, ());
        UNUSED_VALUE(success);
      }
    }

    for (auto const & bs : boundaries)
      for (auto const & b : bs)
        m_visitor(b);
  }

private:
  Sink & m_sink;
  Visitor m_visitor;
};

template <typename Source>
class CitiesBoundariesDecoder
{
public:
  struct DecoderV0
  {
    static m2::PointU DecodeNonNegativeDelta(Source & src)
    {
      auto const dx = ReadVarUint<uint32_t>(src);
      auto const dy = ReadVarUint<uint32_t>(src);
      return {dx, dy};
    }

    static m2::PointU DecodePoint(Source & src, m2::PointU const & base) { return coding::DecodePointDelta(src, base); }
  };

  struct DecoderV1
  {
    static m2::PointU DecodeNonNegativeDelta(Source & src)
    {
      uint32_t dx, dy;
      bits::BitwiseSplit(ReadVarUint<uint64_t>(src), dx, dy);
      return {dx, dy};
    }

    static m2::PointU DecodePoint(Source & src, m2::PointU const & base)
    {
      return coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(src), base);
    }
  };

  template <class TDecoder>
  struct Visitor
  {
  public:
    Visitor(Source & source, serial::GeometryCodingParams const & params)
      : m_source(source)
      , m_params(params)
      , m_last(params.GetBasePoint())
    {}

    m2::PointU LoadPoint() { return coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(m_source), m_last); }

    m2::PointU LoadPointWithLast()
    {
      m_last = LoadPoint();
      return m_last;
    }

    void operator()(m2::BoundingBox & bbox)
    {
      auto const min = LoadPointWithLast();
      m2::PointU const delta = TDecoder::DecodeNonNegativeDelta(m_source);

      bbox = m2::BoundingBox();
      bbox.Add(FromU(min));
      bbox.Add(FromU(min + delta));
    }

    void operator()(m2::CalipersBox & cbox)
    {
      std::vector<m2::PointU> us(4);
      us[0] = LoadPointWithLast();
      us[1] = TDecoder::DecodePoint(m_source, m_last);
      us[3] = TDecoder::DecodePoint(m_source, m_last);

      auto ps = FromU(us);
      auto const dp = ps[3] - ps[0];
      ps[2] = ps[1] + dp;

      cbox.Deserialize(std::move(ps));
    }

    void operator()(m2::DiamondBox & dbox)
    {
      auto const base = LoadPointWithLast();

      // {nx, px}
      auto const delta = TDecoder::DecodeNonNegativeDelta(m_source);

      dbox = m2::DiamondBox();
      dbox.Add(FromU(base));
      dbox.Add(FromU(base + m2::PointU(delta.x, delta.x)));
      dbox.Add(FromU(base + m2::PointU(delta.y, -delta.y)));
      dbox.Add(FromU(base + m2::PointU(delta.x + delta.y, delta.x - delta.y)));
    }

    template <typename R>
    void operator()(R & r)
    {
      r.Visit(*this);
    }

  private:
    m2::PointD FromU(m2::PointU const & u) const { return PointUToPointD(u, m_params.GetCoordBits()); }

    std::vector<m2::PointD> FromU(std::vector<m2::PointU> const & us) const
    {
      std::vector<m2::PointD> ps(us.size());
      for (size_t i = 0; i < us.size(); ++i)
        ps[i] = FromU(us[i]);
      return ps;
    }

    Source & m_source;
    serial::GeometryCodingParams const m_params;
    m2::PointU m_last;
  };

  using BoundariesT = std::vector<std::vector<CityBoundary>>;

  CitiesBoundariesDecoder(Source & source, serial::GeometryCodingParams const & params, BoundariesT & res)
    : m_source(source)
    , m_params(params)
    , m_boundaries(res)
  {}

  void LoadSizes()
  {
    {
      auto const size = static_cast<size_t>(ReadVarUint<uint64_t>(m_source));
      m_boundaries.resize(size);

      BitReader<Source> reader(m_source);
      for (auto & bs : m_boundaries)
      {
        auto const size = static_cast<size_t>(coding::GammaCoder::Decode(reader));
        ASSERT_GREATER_OR_EQUAL(size, 1, ());
        bs.resize(size - 1);
      }
    }
  }

  template <class TDecoder>
  void LoadBoundaries()
  {
    Visitor<TDecoder> visitor(m_source, m_params);
    for (auto & bs : m_boundaries)
      for (auto & b : bs)
        visitor(b);
  }

private:
  Source & m_source;
  serial::GeometryCodingParams const m_params;
  BoundariesT & m_boundaries;
};

struct CitiesBoundariesSerDes
{
  struct Header
  {
    // 0 - initial version
    // 1 - optimized delta-points encoding
    static uint8_t constexpr kLatestVersion = 1;
    static uint8_t constexpr kDefaultCoordBits = 19;

    uint8_t m_coordBits = kDefaultCoordBits;
    uint8_t m_version = kLatestVersion;

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_coordBits);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_version = ReadPrimitiveFromSource<uint8_t>(src);
      m_coordBits = ReadPrimitiveFromSource<uint8_t>(src);
    }
  };

  using BoundariesT = std::vector<std::vector<CityBoundary>>;

  template <typename Sink>
  static void Serialize(Sink & sink, BoundariesT const & boundaries)
  {
    Header header;
    header.Serialize(sink);

    using mercator::Bounds;
    serial::GeometryCodingParams const params(header.m_coordBits, {Bounds::kMinX, Bounds::kMinY});
    CitiesBoundariesEncoder<Sink> encoder(sink, params);
    encoder(boundaries);
  }

  template <typename Source>
  static void Deserialize(Source & source, BoundariesT & boundaries, double & precision)
  {
    Header header;
    header.Deserialize(source);

    using mercator::Bounds;
    precision = std::max(Bounds::kRangeX, Bounds::kRangeY) / double(1 << header.m_coordBits);

    serial::GeometryCodingParams const params(header.m_coordBits, {Bounds::kMinX, Bounds::kMinY});
    using DecoderT = CitiesBoundariesDecoder<Source>;
    DecoderT decoder(source, params, boundaries);

    decoder.LoadSizes();
    if (header.m_version == 0)
      decoder.template LoadBoundaries<typename DecoderT::DecoderV0>();
    else
      decoder.template LoadBoundaries<typename DecoderT::DecoderV1>();
  }
};

}  // namespace indexer
