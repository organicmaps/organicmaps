#pragma once

#include "generator/feature_merger.hpp"
#include "generator/generate_info.hpp"

#include "geometry/tree4d.hpp"

#include "indexer/scales.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

/// Process FeatureBuilder1 for world map. Main functions:
/// - check for visibility in world map
/// - merge linear features
template <class FeatureOutT>
class WorldMapGenerator
{
  class EmitterImpl : public FeatureEmitterIFace
  {
    FeatureOutT m_output;
    uint32_t m_boundaryType;
    deque<m2::RegionD> m_waterRegions;

    m4::Tree<size_t> m_tree;

  public:
    explicit EmitterImpl(feature::GenerateInfo const & info)
      : m_output(info.GetTmpFileName(WORLD_FILE_NAME))
    {
      m_boundaryType = classif().GetTypeByPath({"boundary", "administrative"});
      LoadWaterGeometry(my::JoinFoldersToPath({info.m_intermediateDir},
                                                  string(WORLD_COASTS_FILE_NAME) + ".rawdump"));
      LOG(LINFO, ("Output World file:", info.GetTmpFileName(WORLD_FILE_NAME)));
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

    /// This function is called after merging linear features.
    virtual void operator()(FeatureBuilder1 const & fb)
    {
      // do additional check for suitable size of feature
      if (NeedPushToWorld(fb) &&
          scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    bool IsWaterBoundaries(FeatureBuilder1 const & fb) const
    {
      return false;

      if (fb.FindType(m_boundaryType, 2) == ftype::GetEmptyValue())
        return false;

      m2::PointD pts[3] = {{0, 0}, {0, 0}, {0, 0}};
      size_t hits[3] = {0, 0, 0};

      // For check we select first, last and middle point in line.
      auto const & line = fb.GetGeometry().front();
      pts[0] = line.front();
      pts[1] = *(line.cbegin() + line.size() / 2);
      pts[2] = line.back();

      m_tree.ForEachInRect(fb.GetLimitRect(), [&](size_t index)
      {
        hits[0] += m_waterRegions[index].Contains(pts[0]) ? 1 : 0;
        hits[1] += m_waterRegions[index].Contains(pts[1]) ? 1 : 0;
        hits[2] += m_waterRegions[index].Contains(pts[2]) ? 1 : 0;
      });

      size_t const state = (hits[0] & 0x01) + (hits[1] & 0x01) + (hits[2] & 0x01);

      // whole border on water
      if (state == 3)
      {
        LOG(LINFO, ("Boundary", (state == 3 ? "deleted" : "kept"), "hits:", hits,
                    DebugPrint(fb.GetParams()), fb.GetOsmIdsString()));
        return true;
      }

      // state == 0 whole border on land, else partial intersection
      return false;
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

public:
  template <class TInfo>
  explicit WorldMapGenerator(TInfo const & info)
      : m_worldBucket(info),
        m_merger(POINT_COORD_BITS - (scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2)
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

    /// @todo It's not obvious to integrate link->way conversion.
    /// Review it in future.
    /*
    char const * arr3[][2]  = {
      { "highway", "motorway_link" },
      { "highway", "primary_link" },
      { "highway", "secondary_link" },
      { "highway", "trunk_link" }
    };
    char const * arr4[][2]  = {
      { "highway", "motorway" },
      { "highway", "primary" },
      { "highway", "secondary" },
      { "highway", "trunk" }
    };
    STATIS_ASSERT(ARRAY_SIZE(arr3) == ARRAY_SIZE(arr4));

    for (size_t i = 0; i < ARRAY_SIZE(arr3); ++i)
      m_typesCorrector.SetMappingTypes(arr3[i], arr4[i]);
    */
  }

  void operator()(FeatureBuilder1 fb)
  {
    if (m_worldBucket.NeedPushToWorld(fb))
    {
      // skip visible water boundary
      if (m_worldBucket.IsWaterBoundaries(fb))
        return;

      if (fb.GetGeomType() == feature::GEOM_LINE)
      {
        MergedFeatureBuilder1 * p = m_typesCorrector(fb);
        if (p)
          m_merger(p);
      }
      else
      {
        if (feature::PreprocessForWorldMap(fb))
          m_worldBucket.PushSure(fb);
      }
    }
  }

  void DoMerge() { m_merger.DoMerge(m_worldBucket); }
};

template <class FeatureOutT>
class CountryMapGenerator
{
  FeatureOutT m_bucket;

public:
  template <class TInfo>
  explicit CountryMapGenerator(TInfo const & info)
      : m_bucket(info)
  {
  }

  void operator()(FeatureBuilder1 fb)
  {
    if (feature::PreprocessForCountryMap(fb))
      m_bucket(fb);
  }

  FeatureOutT const & Parent() const { return m_bucket; }
};
