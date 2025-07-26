#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_helpers.hpp"
#include "generator/tesselator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polygon.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"

#include <functional>
#include <list>
#include <vector>

namespace feature
{
// 4 bits are used to encode number of inner points/triangles
// (and the "0" value is reserved to indicate use of outer geometry).
// But actually 16 inner triangles are stored or just 14 inner points,
// because a 3-byte points simplification mask doesn't allow more.
size_t constexpr kMaxInnerGeometryElements = 14;

// Minimum difference and times to a more detailed geometry to store a simplified geom scale.
// A geom level is discarded as too similar to a more detailed one
// when its' number of elements is <kGeomMinDiff less or <kGeomMinFactor times less.
size_t constexpr kGeomMinDiff = 2;
double constexpr kGeomMinFactor = 1.5f;

class GeometryHolder
{
public:
  using FileGetter = std::function<FileWriter &(int i)>;
  using Points = std::vector<m2::PointD>;
  using Polygons = std::list<Points>;

  GeometryHolder(FileGetter geoFileGetter, FileGetter trgFileGetter, FeatureBuilder & fb, DataHeader const & header)
    : m_geoFileGetter(geoFileGetter)
    , m_trgFileGetter(trgFileGetter)
    , m_fb(fb)
    , m_ptsInner(true)
    , m_trgInner(true)
    , m_header(header)
  {}

  GeometryHolder(FeatureBuilder & fb, DataHeader const & header)
    : m_fb(fb)
    , m_ptsInner(true)
    , m_trgInner(true)
    , m_header(header)
  {}

  FeatureBuilder::SupportingData & GetBuffer() { return m_buffer; }

  Points const & GetSourcePoints() const
  {
    // For inner geometry lines keep simplifying the previous version to ensure points visibility is consistent.
    return !m_current.empty() ? m_current : m_fb.GetOuterGeometry();
  }

  // Its important AddPoints is called sequentially from upper scales to lower.
  void AddPoints(Points const & points, int scaleIndex)
  {
    if (m_ptsInner && points.size() <= kMaxInnerGeometryElements)
    {
      // Inner geometry: store inside feature's header and keep
      // a simplification mask for scale-specific visibility of each point.
      FillInnerPoints(points, scaleIndex);
      m_current = points;
    }
    else
    {
      m_ptsInner = false;
      if (m_ptsPrevCount == 0 ||
          (points.size() + kGeomMinDiff <= m_ptsPrevCount && points.size() * kGeomMinFactor <= m_ptsPrevCount))
      {
        WriteOuterPoints(points, scaleIndex);
        m_ptsPrevCount = points.size();
      }
      else
      {
        CHECK(m_buffer.m_ptsMask != 0, ("Some valid geometry should be present already"));
        m_buffer.m_ptsMask |= (1 << scaleIndex);
        m_buffer.m_ptsOffset.push_back(feature::kGeomOffsetFallback);
      }
    }
  }

  bool NeedProcessTriangles() const { return !m_trgInner || m_buffer.m_innerTrg.empty(); }

  bool TryToMakeStrip(Points & points)
  {
    size_t const count = points.size();
    if (!m_trgInner || (count >= 2 && count - 2 > kMaxInnerGeometryElements))
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
      {
        // Usually, it is a bad polygon in OSM.
        LOG(LWARNING, ("GeometryHolder: Degenerated polygon", m_fb.GetMostGenericOsmId()));
        return false;
      }
    }

    size_t const index = FindSingleStrip(count, IsDiagonalVisibleFunctor(points.begin(), points.end()));
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

    size_t trgPointsCount = 0;
    for (auto const & points : polys)
      trgPointsCount += points.size();

    if (m_trgPrevCount == 0 ||
        (trgPointsCount + kGeomMinDiff <= m_trgPrevCount && trgPointsCount * kGeomMinFactor <= m_trgPrevCount))
    {
      if (WriteOuterTriangles(polys, scaleIndex))
      {
        // Assign only if geometry is valid (correctly tesselated and saved).
        m_trgPrevCount = trgPointsCount;
      }
    }
    else
    {
      CHECK(m_buffer.m_trgMask != 0, ("Some valid geometry should be present already"));
      m_buffer.m_trgMask |= (1 << scaleIndex);
      m_buffer.m_trgOffset.push_back(feature::kGeomOffsetFallback);
    }
  }

private:
  class StripEmitter
  {
  public:
    StripEmitter(Points const & src, Points & dest) : m_src(src), m_dest(dest) { m_dest.reserve(m_src.size()); }
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

    // Optimization: store the first point once in the header instead of duplicating it for each geom scale.
    cp.SetBasePoint(points[0]);
    // Can optimize here, but ... Make copy of vector.
    Points toSave(points.begin() + 1, points.end());

    m_buffer.m_ptsMask |= (1 << i);
    auto const pos = feature::CheckedFilePosCast(m_geoFileGetter(i));
    CHECK(pos != feature::kGeomOffsetFallback, ());
    m_buffer.m_ptsOffset.push_back(pos);

    serial::SaveOuterPath(toSave, cp, m_geoFileGetter(i));
  }

  bool WriteOuterTriangles(Polygons const & polys, int i)
  {
    CHECK(m_trgFileGetter, ("m_trgFileGetter must be set to write outer triangles."));

    // tesselation
    tesselator::TrianglesInfo info;
    if (0 == tesselator::TesselateInterior(polys, info))
    {
      /// @todo Some examples here: https://github.com/organicmaps/organicmaps/issues/5607
      LOG(LWARNING, ("GeometryHolder: No triangles for scale index", i, "in", m_fb.GetMostGenericOsmId()));
      return false;
    }

    auto const cp = m_header.GetGeometryCodingParams(i);

    serial::TrianglesChainSaver saver(cp);

    // points conversion
    tesselator::PointsInfo points;
    info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(),
                       [bits = cp.GetCoordBits()](m2::PointD const & p) { return PointDToPointU(p, bits); }, points);

    size_t const ptsCount = points.m_points.size();
    if (ptsCount > 10000)
    {
      static auto const buildingType = classif().GetTypeByPath({"building"});
      if (m_fb.HasType(buildingType))
      {
        // Big feature is critical for building type in drape (3D drawing).
        LOG(LERROR, ("GeometryHolder: Large building feature", m_fb.GetMostGenericOsmId(), "with", ptsCount, "points"));
      }
      else
        LOG(LWARNING, ("GeometryHolder: Large feature", m_fb.GetMostGenericOsmId(), "with", ptsCount, "points"));
    }

    // triangles processing (should be optimal)
    info.ProcessPortions(points, saver, true);

    // check triangles processing (to compare with optimal)
    // serial::TrianglesChainSaver checkSaver(cp);
    // info.ProcessPortions(points, checkSaver, false);

    // CHECK_LESS_OR_EQUAL(saver.GetBufferSize(), checkSaver.GetBufferSize(), ());

    // saving to file
    m_buffer.m_trgMask |= (1 << i);
    auto const pos = feature::CheckedFilePosCast(m_trgFileGetter(i));
    CHECK(pos != feature::kGeomOffsetFallback, ());
    m_buffer.m_trgOffset.push_back(pos);
    saver.Save(m_trgFileGetter(i));

    return true;
  }

  void FillInnerPoints(Points const & points, uint32_t scaleIndex)
  {
    auto const & src = m_buffer.m_innerPts;

    if (src.empty())
    {
      m_buffer.m_innerPts = points;
      // Init the simplification mask to the scaleIndex.
      // First and last points are present always, hence not stored in the mask.
      for (size_t i = 1; i < points.size() - 1; ++i)
        m_buffer.m_ptsSimpMask |= (scaleIndex << (2 * (i - 1)));
      return;
    }

    CHECK(feature::ArePointsEqual(src.front(), points.front()), ());
    CHECK(feature::ArePointsEqual(src.back(), points.back()), ());

    uint32_t constexpr mask = 0x3;
    size_t j = 1;
    for (size_t i = 1; i < points.size() - 1; ++i)
    {
      for (; j < src.size() - 1; ++j)
      {
        if (feature::ArePointsEqual(src[j], points[i]))
        {
          // Update corresponding 2 bits for the source point [j] to the scaleIndex.
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

  FeatureBuilder & m_fb;

  FeatureBuilder::SupportingData m_buffer;

  Points m_current;
  bool m_ptsInner, m_trgInner;
  size_t m_ptsPrevCount = 0, m_trgPrevCount = 0;

  feature::DataHeader const & m_header;
};
}  //  namespace feature
