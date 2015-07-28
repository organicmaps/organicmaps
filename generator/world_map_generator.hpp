#pragma once

#include "generator/feature_merger.hpp"
#include "generator/generate_info.hpp"

#include "geometry/tree4d.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

namespace
{
class WaterBoundaryChecker
{
  uint32_t m_boundaryType;
  deque<m2::RegionD> m_waterRegions;

  m4::Tree<size_t> m_tree;
  size_t m_totalFeatures = 0;
  size_t m_totalBorders = 0;
  size_t m_skippedBorders = 0;
  size_t m_selectedPolygons = 0;

public:
  WaterBoundaryChecker(feature::GenerateInfo const & info)
  {
    m_boundaryType = classif().GetTypeByPath({"boundary", "administrative"});
    LoadWaterGeometry(
        info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION));
  }

  ~WaterBoundaryChecker()
  {
    LOG(LINFO, ("Features checked:", m_totalFeatures, "borders checked:", m_totalBorders,
                "borders skipped:", m_skippedBorders, "selected polygons:", m_selectedPolygons));
  }

  void LoadWaterGeometry(string const & rawGeometryFileName)
  {
    LOG(LINFO, ("Loading water geometry:", rawGeometryFileName));
    FileReader reader(rawGeometryFileName);
    ReaderSource<FileReader> file(reader);

    size_t total = 0;
    while (true)
    {
      uint64_t numGeometries = 0;
      file.Read(&numGeometries, sizeof(numGeometries));

      if (numGeometries == 0)
        break;

      ++total;

      vector<m2::PointD> points;
      for (size_t i = 0; i < numGeometries; ++i)
      {
        uint64_t numPoints = 0;
        file.Read(&numPoints, sizeof(numPoints));
        points.resize(numPoints);
        file.Read(points.data(), sizeof(m2::PointD) * numPoints);
        m_waterRegions.push_back(m2::RegionD());
        m_waterRegions.back().Assign(points.begin(), points.end());
        m_tree.Add(m_waterRegions.size() - 1, m_waterRegions.back().GetRect());
      }
    }
    LOG(LINFO, ("Load", total, "water geometries"));
  }

  bool IsWaterBoundaries(FeatureBuilder1 const & fb)
  {
    ++m_totalFeatures;

    if (fb.FindType(m_boundaryType, 2) == ftype::GetEmptyValue())
      return false;

    ++m_totalBorders;

    array<m2::PointD, 3> pts = {{{0, 0}, {0, 0}, {0, 0}}};
    array<size_t, 3> hits = {{0, 0, 0}};

    // For check we select first, last and middle point in line.
    auto const & line = fb.GetGeometry().front();
    pts[0] = line.front();
    pts[1] = *(line.cbegin() + line.size() / 2);
    pts[2] = line.back();

    double constexpr kExtension = 0.01;

    for (size_t i = 0; i < pts.size(); ++i)
    {
      m2::PointD const & p = pts[i];
      m2::RectD r(p.x - kExtension, p.y - kExtension, p.x + kExtension, p.y + kExtension);
      m_tree.ForEachInRect(r, [&](size_t index)
      {
        ++m_selectedPolygons;
        hits[i] += m_waterRegions[index].Contains(p) ? 1 : 0;
      });
    }

    size_t const state = (hits[0] & 0x01) + (hits[1] & 0x01) + (hits[2] & 0x01);

    // whole border on water
    if (state == 3)
    {
      LOG(LINFO, ("Boundary", (state == 3 ? "deleted" : "kept"), "hits:", hits,
                  DebugPrint(fb.GetParams()), fb.GetOsmIdsString()));
      ++m_skippedBorders;
      return true;
    }

    // state == 0 whole border on land, else partial intersection
    return false;
  }
};
}

/// Process FeatureBuilder1 for world map. Main functions:
/// - check for visibility in world map
/// - merge linear features
template <class FeatureOutT>
class WorldMapGenerator
{
  class EmitterImpl : public FeatureEmitterIFace
  {
    FeatureOutT m_output;
    map<uint32_t, size_t> m_mapTypes;

  public:
    explicit EmitterImpl(feature::GenerateInfo const & info)
      : m_output(info.GetTmpFileName(WORLD_FILE_NAME))
    {
      LOG(LINFO, ("Output World file:", info.GetTmpFileName(WORLD_FILE_NAME)));
    }

    ~EmitterImpl() override
    {
      Classificator const & c = classif();
      
      stringstream ss;
      for (auto const & p : m_mapTypes)
        ss << c.GetReadableObjectName(p.first) << " : " <<  p.second << endl;
      LOG(LINFO, ("World types:\n", ss.str()));
    }

    /// This function is called after merging linear features.
    void operator()(FeatureBuilder1 const & fb) override
    {
      // do additional check for suitable size of feature
      if (NeedPushToWorld(fb) &&
          scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    void CalcStatistics(FeatureBuilder1 const & fb)
    {
      for (uint32_t type : fb.GetTypes())
        ++m_mapTypes[type];
    }

    bool NeedPushToWorld(FeatureBuilder1 const & fb) const
    {
      // GetMinFeatureDrawScale also checks suitable size for AREA features
      return (scales::GetUpperWorldScale() >= fb.GetMinFeatureDrawScale());
    }

    void PushSure(FeatureBuilder1 const & fb) { m_output(fb); }
  };

  EmitterImpl m_worldBucket;
  FeatureTypesProcessor m_typesCorrector;
  FeatureMergeProcessor m_merger;
  WaterBoundaryChecker m_boundaryChecker;

public:
  explicit WorldMapGenerator(feature::GenerateInfo const & info)
      : m_worldBucket(info),
        m_merger(POINT_COORD_BITS - (scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2),
        m_boundaryChecker(info)
  {
    // Do not strip last types for given tags,
    // for example, do not cut 'admin_level' in  'boundary-administrative-XXX'.
    char const * arr1[][3] = {{"boundary", "administrative", "2"},
                              {"boundary", "administrative", "3"},
                              {"boundary", "administrative", "4"}};

    for (size_t i = 0; i < ARRAY_SIZE(arr1); ++i)
      m_typesCorrector.SetDontNormalizeType(arr1[i]);

    char const * arr2[] = {"boundary", "administrative", "4", "state"};
    m_typesCorrector.SetDontNormalizeType(arr2);
  }

  void operator()(FeatureBuilder1 fb)
  {
    if (!m_worldBucket.NeedPushToWorld(fb))
      return;

    m_worldBucket.CalcStatistics(fb);
    // skip visible water boundary
    if (m_boundaryChecker.IsWaterBoundaries(fb))
      return;

    if (fb.GetGeomType() == feature::GEOM_LINE)
    {
      MergedFeatureBuilder1 * p = m_typesCorrector(fb);
      if (p)
        m_merger(p);
      return;
    }

    if (feature::PreprocessForWorldMap(fb))
      m_worldBucket.PushSure(fb);
  }

  void DoMerge() { m_merger.DoMerge(m_worldBucket); }
};

template <class FeatureOutT>
class CountryMapGenerator
{
  FeatureOutT m_bucket;

public:
  explicit CountryMapGenerator(feature::GenerateInfo const & info) : m_bucket(info) {}

  void operator()(FeatureBuilder1 fb)
  {
    if (feature::PreprocessForCountryMap(fb))
      m_bucket(fb);
  }

  inline FeatureOutT const & Parent() const { return m_bucket; }
};
