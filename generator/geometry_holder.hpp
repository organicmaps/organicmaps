#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_helpers.hpp"
#include "generator/tesselator.hpp"

#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"
#include "geometry/polygon.hpp"
#include "geometry/simplification.hpp"

#include "indexer/data_header.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <vector>

namespace feature
{
class GeometryHolder
{
public:
  using FileGetter = std::function<FileWriter &(int i)>;
  using Points = std::vector<m2::PointD>;
  using Polygons = std::list<Points>;

  // For FeatureType serialization maxNumTriangles should be less than numeric_limits<uint8_t>::max
  // because FeatureType format uses uint8_t to encode the number of triangles.
  GeometryHolder(FileGetter geoFileGetter, FileGetter trgFileGetter, FeatureBuilder2 & fb,
                 feature::DataHeader const & header, size_t maxNumTriangles = 14)
    : m_geoFileGetter(geoFileGetter)
    , m_trgFileGetter(trgFileGetter)
    , m_fb(fb)
    , m_ptsInner(true)
    , m_trgInner(true)
    , m_header(header)
    , m_maxNumTriangles(maxNumTriangles)
  {
  }

  GeometryHolder(FeatureBuilder2 & fb, feature::DataHeader const & header,
                 size_t maxNumTriangles = 14)
    : m_fb(fb)
    , m_ptsInner(true)
    , m_trgInner(true)
    , m_header(header)
    , m_maxNumTriangles(maxNumTriangles)
  {
  }

  void SetInner() { m_trgInner = true; }

  FeatureBuilder2::SupportingData & GetBuffer() { return m_buffer; }

  Points const & GetSourcePoints()
  {
    return !m_current.empty() ? m_current : m_fb.GetOuterGeometry();
  }

  void AddPoints(Points const & points, int scaleIndex)
  {
    if (m_ptsInner && points.size() <= m_maxNumTriangles)
    {
      if (m_buffer.m_innerPts.empty())
        m_buffer.m_innerPts = points;
      else
        FillInnerPointsMask(points, scaleIndex);
      m_current = points;
    }
    else
    {
      m_ptsInner = false;
      WriteOuterPoints(points, scaleIndex);
    }
  }

  bool NeedProcessTriangles() const { return !m_trgInner || m_buffer.m_innerTrg.empty(); }

  bool TryToMakeStrip(Points & points)
  {
    size_t const count = points.size();
    if (!m_trgInner || (count >= 2 && count - 2 > m_maxNumTriangles))
    {
      // too many points for strip
      m_trgInner = false;
      return false;
    }

    if (m2::robust::CheckPolygonSelfIntersections(points.begin(), points.end()))
    {
      // polygon has self-intersectios
      m_trgInner = false;
      return false;
    }

    CHECK(m_buffer.m_innerTrg.empty(), ());

    // make CCW orientation for polygon
    if (!IsPolygonCCW(points.begin(), points.end()))
    {
      reverse(points.begin(), points.end());

      // Actually this check doesn't work for some degenerate polygons.
      // See IsPolygonCCW_DataSet tests for more details.
      // ASSERT(IsPolygonCCW(points.begin(), points.end()), (points));
      if (!IsPolygonCCW(points.begin(), points.end()))
        return false;
    }

    size_t const index = FindSingleStrip(
        count, IsDiagonalVisibleFunctor<Points::const_iterator>(points.begin(), points.end()));

    if (index == count)
    {
      // can't find strip
      m_trgInner = false;
      return false;
    }

    MakeSingleStripFromIndex(index, count, StripEmitter(points, m_buffer.m_innerTrg));

    CHECK_EQUAL(count, m_buffer.m_innerTrg.size(), ());
    return true;
  }

  void AddTriangles(Polygons const & polys, int scaleIndex)
  {
    CHECK(m_buffer.m_innerTrg.empty(), ());
    m_trgInner = false;

    WriteOuterTriangles(polys, scaleIndex);
  }

private:
  class StripEmitter
  {
  public:
    StripEmitter(Points const & src, Points & dest) : m_src(src), m_dest(dest)
    {
      m_dest.reserve(m_src.size());
    }
    void operator()(size_t i) { m_dest.push_back(m_src[i]); }

  private:
    Points const & m_src;
    Points & m_dest;
  };

  void WriteOuterPoints(Points const & points, int i)
  {
    CHECK(m_geoFileGetter, ("m_geoFileGetter must be set to write outer points."));

    // outer path can have 2 points in small scale levels
    ASSERT_GREATER(points.size(), 1, ());

    auto cp = m_header.GetGeometryCodingParams(i);

    // Optimization: Store first point once in header for outer linear features.
    cp.SetBasePoint(points[0]);
    // Can optimize here, but ... Make copy of vector.
    Points toSave(points.begin() + 1, points.end());

    m_buffer.m_ptsMask |= (1 << i);
    auto const pos = feature::CheckedFilePosCast(m_geoFileGetter(i));
    m_buffer.m_ptsOffset.push_back(pos);

    serial::SaveOuterPath(toSave, cp, m_geoFileGetter(i));
  }

  void WriteOuterTriangles(Polygons const & polys, int i)
  {
    CHECK(m_trgFileGetter, ("m_trgFileGetter must be set to write outer triangles."));

    // tesselation
    tesselator::TrianglesInfo info;
    if (0 == tesselator::TesselateInterior(polys, info))
    {
      LOG(LINFO, ("NO TRIANGLES in", polys));
      return;
    }

    auto const cp = m_header.GetGeometryCodingParams(i);

    serial::TrianglesChainSaver saver(cp);

    // points conversion
    tesselator::PointsInfo points;
    m2::PointU (*D2U)(m2::PointD const &, uint8_t) = &PointDToPointU;
    info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(),
                       std::bind(D2U, std::placeholders::_1, cp.GetCoordBits()), points);

    // triangles processing (should be optimal)
    info.ProcessPortions(points, saver, true);

    // check triangles processing (to compare with optimal)
    // serial::TrianglesChainSaver checkSaver(cp);
    // info.ProcessPortions(points, checkSaver, false);

    // CHECK_LESS_OR_EQUAL(saver.GetBufferSize(), checkSaver.GetBufferSize(), ());

    // saving to file
    m_buffer.m_trgMask |= (1 << i);
    auto const pos = feature::CheckedFilePosCast(m_trgFileGetter(i));
    m_buffer.m_trgOffset.push_back(pos);
    saver.Save(m_trgFileGetter(i));
  }

  void FillInnerPointsMask(Points const & points, uint32_t scaleIndex)
  {
    auto const & src = m_buffer.m_innerPts;
    CHECK(!src.empty(), ());

    CHECK(feature::ArePointsEqual(src.front(), points.front()), ());
    CHECK(feature::ArePointsEqual(src.back(), points.back()), ());

    size_t j = 1;
    for (size_t i = 1; i < points.size() - 1; ++i)
    {
      for (; j < src.size() - 1; ++j)
      {
        if (feature::ArePointsEqual(src[j], points[i]))
        {
          // set corresponding 2 bits for source point [j] to scaleIndex
          uint32_t mask = 0x3;
          m_buffer.m_ptsSimpMask &= ~(mask << (2 * (j - 1)));
          m_buffer.m_ptsSimpMask |= (scaleIndex << (2 * (j - 1)));
          break;
        }
      }

      CHECK_LESS(j, src.size() - 1, ("Simplified point is not found in the source point array."));
    }
  }

  FileGetter m_geoFileGetter = {};
  FileGetter m_trgFileGetter = {};

  FeatureBuilder2 & m_fb;

  FeatureBuilder2::SupportingData m_buffer;

  Points m_current;
  bool m_ptsInner, m_trgInner;

  feature::DataHeader const & m_header;
  // max triangles number to store in innerTriangles
  size_t m_maxNumTriangles;
};
}  //  namespace feature
