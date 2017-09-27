#pragma once

#include "indexer/coding_params.hpp"
#include "indexer/geometry_coding.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/point_to_integer.hpp"
#include "coding/varint.hpp"

#include "geometry/bounding_box.hpp"
#include "geometry/calipers_box.hpp"
#include "geometry/diamond_box.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace indexer
{
template <typename Sink>
class BoundaryBoxesEncoder
{
public:
  struct Visitor
  {
  public:
    Visitor(Sink & sink, serial::CodingParams const & params)
      : m_sink(sink), m_params(params), m_last(params.GetBasePoint())
    {
    }

    void operator()(m2::PointD const & p) { return (*this)(ToU(p)); }

    void operator()(m2::PointU const & p)
    {
      WriteVarUint(m_sink, EncodeDelta(p, m_last));
      m_last = p;
    }

    void operator()(m2::BoundingBox const & bbox)
    {
      auto const min = ToU(bbox.Min());
      auto const max = ToU(bbox.Max());

      (*this)(min);
      EncodePositiveDelta(min, max);
    }

    void operator()(m2::CalipersBox const & cbox)
    {
      double const kEps = 1e-5;

      auto ps = cbox.Points();
      auto us = ToU(ps);

      CHECK(!ps.empty(), ());
      CHECK_LESS_OR_EQUAL(ps.size(), 4, ());
      CHECK(ps.size() != 3, ());

      while (ps.size() != 4)
      {
        auto const lp = ps.back();
        ps.push_back(lp);

        auto const lu = us.back();
        us.push_back(lu);
      }

      ASSERT_EQUAL(ps.size(), 4, ());
      ASSERT_EQUAL(us.size(), 4, ());

      size_t pivot = ps.size();
      for (size_t curr = 0; curr < ps.size(); ++curr)
      {
        size_t const next = (curr + 1) % ps.size();
        if (ps[next].x >= ps[curr].x - kEps && ps[next].y >= ps[curr].y - kEps)
        {
          pivot = curr;
          break;
        }
      }

      CHECK(pivot != ps.size(), ());
      std::rotate(us.begin(), us.begin() + pivot, us.end());

      (*this)(us[0]);
      EncodePositiveDelta(us[0], us[1]);

      uint64_t const width = us[3].Length(us[0]);
      WriteVarUint(m_sink, width);
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
    m2::PointU ToU(m2::PointD const & p) const { return PointD2PointU(p, m_params.GetCoordBits()); }

    std::vector<m2::PointU> ToU(std::vector<m2::PointD> const & ps) const
    {
      std::vector<m2::PointU> us(ps.size());
      for (size_t i = 0; i < ps.size(); ++i)
        us[i] = ToU(ps[i]);
      return us;
    }

    void EncodePositiveDelta(m2::PointU const & curr, m2::PointU const & next)
    {
      ASSERT_GREATER_OR_EQUAL(next.x, curr.x, ());
      ASSERT_GREATER_OR_EQUAL(next.y, curr.y, ());

      // Paranoid checks due to possible floating point artifacts
      // here.  In general, next.x >= curr.x and next.y >= curr.y.
      auto const dx = next.x >= curr.x ? next.x - curr.x : 0;
      auto const dy = next.y >= curr.y ? next.y - curr.y : 0;
      WriteVarUint(m_sink, dx);
      WriteVarUint(m_sink, dy);
    }

    Sink & m_sink;
    serial::CodingParams m_params;
    m2::PointU m_last;
  };

  BoundaryBoxesEncoder(Sink & sink, serial::CodingParams const & params)
    : m_sink(sink), m_visitor(sink, params)
  {
  }

  void operator()(std::vector<std::vector<BoundaryBoxes>> const & boxes)
  {
    WriteVarUint(m_sink, boxes.size());

    {
      BitWriter<Sink> writer(m_sink);
      for (auto const & bs : boxes)
      {
        CHECK(!bs.empty(), ());
        coding::GammaCoder::Encode(writer, bs.size());
      }
    }

    for (auto const & bs : boxes)
    {
      for (auto const & b : bs)
        m_visitor(b);
    }
  }

private:
  Sink & m_sink;
  serial::CodingParams m_params;
  Visitor m_visitor;
};

template <typename Source>
class BoundaryBoxesDecoder
{
public:
  struct Visitor
  {
  public:
    Visitor(Source & source, serial::CodingParams const & params)
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
      p = DecodeDelta(ReadVarUint<uint64_t>(m_source), m_last);
      m_last = p;
    }

    void operator()(m2::BoundingBox & bbox)
    {
      m2::PointU min;
      (*this)(min);
      auto const max = min + DecodePositiveDelta();

      bbox = m2::BoundingBox();
      bbox.Add(FromU(min));
      bbox.Add(FromU(max));
    }

    void operator()(m2::CalipersBox & cbox)
    {
      m2::PointU pivot;
      (*this)(pivot);

      std::vector<m2::PointU> points(4);
      points[0] = pivot;
      points[1] = pivot + DecodePositiveDelta();

      auto const width = ReadVarUint<uint64_t>(m_source);

      auto r01 = m2::PointD(points[1] - points[0]);
      if (!r01.IsAlmostZero())
        r01 = r01.Normalize();
      auto const r21 = r01.Ort() * width;

      points[2] = points[1] + r21;
      points[3] = points[0] + r21;

      cbox = m2::CalipersBox(FromU(points));
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
      return PointU2PointD(u, m_params.GetCoordBits());
    }

    std::vector<m2::PointD> FromU(std::vector<m2::PointU> const & us) const
    {
      std::vector<m2::PointD> ps(us.size());
      for (size_t i = 0; i < us.size(); ++i)
        ps[i] = FromU(us[i]);
      return ps;
    }

    m2::PointU DecodePositiveDelta()
    {
      auto const dx = ReadVarUint<uint32_t>(m_source);
      auto const dy = ReadVarUint<uint32_t>(m_source);
      return m2::PointU(static_cast<uint32_t>(dx), static_cast<uint32_t>(dy));
    }

    Source & m_source;
    serial::CodingParams const m_params;
    m2::PointU m_last;
  };

  BoundaryBoxesDecoder(Source & source, serial::CodingParams const & params)
    : m_source(source), m_visitor(source, params)
  {
  }

  void operator()(std::vector<std::vector<BoundaryBoxes>> & boxes)
  {
    auto const size = ReadVarUint<uint64_t>(m_source);
    boxes.resize(size);

    {
      BitReader<Source> reader(m_source);
      for (auto & bs : boxes)
      {
        auto const size = coding::GammaCoder::Decode(reader);
        bs.resize(size);
      }
    }

    for (auto & bs : boxes)
    {
      for (auto & b : bs)
        m_visitor(b);
    }
  }

private:
  Source & m_source;
  Visitor m_visitor;
};
}  // namespace indexer
