#pragma once

#include "generator/feature_merger.hpp"
#include "generator/generate_info.hpp"
#include "generator/popular_places_section_builder.hpp"

#include "search/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/polygon.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "defines.hpp"

namespace
{
class WaterBoundaryChecker
{
  uint32_t m_boundaryType;

  struct RegionTraits
  {
    m2::RectD const & LimitRect(m2::RegionD const & r) const { return r.GetRect(); }
  };
  m4::Tree<m2::RegionD, RegionTraits> m_tree;

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
    LOG_SHORT(LINFO, ("Features checked:", m_totalFeatures, "borders checked:", m_totalBorders,
                "borders skipped:", m_skippedBorders, "selected polygons:", m_selectedPolygons));
  }

  void LoadWaterGeometry(std::string const & rawGeometryFileName)
  {
    LOG_SHORT(LINFO, ("Loading water geometry:", rawGeometryFileName));
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

      for (size_t i = 0; i < numGeometries; ++i)
      {
        uint64_t numPoints = 0;
        file.Read(&numPoints, sizeof(numPoints));

        std::vector<m2::PointD> points(numPoints);
        file.Read(points.data(), sizeof(m2::PointD) * numPoints);
        m_tree.Add(m2::RegionD(move(points)));
      }
    }
    LOG_SHORT(LINFO, ("Load", total, "water geometries"));
  }

  bool IsBoundaries(FeatureBuilder1 const & fb)
  {
    ++m_totalFeatures;

    if (fb.FindType(m_boundaryType, 2) == ftype::GetEmptyValue())
      return false;

    ++m_totalBorders;

    return true;
  }

  enum class ProcessState
  {
    Initial,
    Water,
    Earth
  };

  void ProcessBoundary(FeatureBuilder1 const & boundary, std::vector<FeatureBuilder1> & parts)
  {
    auto const & line = boundary.GetGeometry().front();

    double constexpr kExtension = 0.01;
    ProcessState state = ProcessState::Initial;

    FeatureBuilder1::PointSeq points;

    for (size_t i = 0; i < line.size(); ++i)
    {
      m2::PointD const & p = line[i];
      m2::RectD r(p.x - kExtension, p.y - kExtension, p.x + kExtension, p.y + kExtension);
      size_t hits = 0;
      m_tree.ForEachInRect(r, [&](m2::RegionD const & rgn)
      {
        ++m_selectedPolygons;
        hits += rgn.Contains(p) ? 1 : 0;
      });

      bool inWater = (hits & 0x01) == 1;

      switch (state)
      {
      case ProcessState::Initial:
      {
        if (inWater)
        {
          state = ProcessState::Water;
        }
        else
        {
          points.push_back(p);
          state = ProcessState::Earth;
        }
        break;
      }
      case ProcessState::Water:
      {
        if (inWater)
        {
          // do nothing
        }
        else
        {
          points.push_back(p);
          state = ProcessState::Earth;
        }
        break;
      }
      case ProcessState::Earth:
      {
        if (inWater)
        {
          if (points.size() > 1)
          {
            parts.push_back(boundary);
            parts.back().ResetGeometry();
            for (auto const & pt : points)
              parts.back().AddPoint(pt);
          }
          points.clear();
          state = ProcessState::Water;
        }
        else
        {
          points.push_back(p);
        }
        break;
      }
      }
    }

    if (points.size() > 1)
    {
      parts.push_back(boundary);
      parts.back().ResetGeometry();
      for (auto const & pt : points)
        parts.back().AddPoint(pt);
    }

    if (parts.empty())
      m_skippedBorders++;
  }
};
} // namespace

/// Process FeatureBuilder1 for world map. Main functions:
/// - check for visibility in world map
/// - merge linear features
template <class FeatureOutT>
class WorldMapGenerator
{
  class EmitterImpl : public FeatureEmitterIFace
  {
    FeatureOutT m_output;
    std::map<uint32_t, size_t> m_mapTypes;

  public:
    explicit EmitterImpl(feature::GenerateInfo const & info)
      : m_output(info.GetTmpFileName(WORLD_FILE_NAME))
    {
      LOG_SHORT(LINFO, ("Output World file:", info.GetTmpFileName(WORLD_FILE_NAME)));
    }

    ~EmitterImpl() override
    {
      Classificator const & c = classif();
      
      std::stringstream ss;
      ss << std::endl;
      for (auto const & p : m_mapTypes)
        ss << c.GetReadableObjectName(p.first) << " : " <<  p.second << std::endl;
      LOG_SHORT(LINFO, ("World types:", ss.str()));
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
  generator::PopularPlaces m_popularPlaces;

public:
  explicit WorldMapGenerator(feature::GenerateInfo const & info)
    : m_worldBucket(info)
    , m_merger(kPointCoordBits - (scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2)
    , m_boundaryChecker(info)
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

    if (!info.m_popularPlacesFilename.empty())
      generator::LoadPopularPlaces(info.m_popularPlacesFilename, m_popularPlaces);
    else
      LOG(LWARNING, ("popular_places_data option not set. Popular atractions will not be added to World.mwm"));
  }

  void operator()(FeatureBuilder1 fb)
  {
    auto const isPopularAttraction = IsPopularAttraction(fb);
    auto const isInternationalAirport =
        fb.HasType(classif().GetTypeByPath({"aeroway", "aerodrome", "international"}));
    auto const forcePushToWorld = isPopularAttraction || isInternationalAirport;

    if (!m_worldBucket.NeedPushToWorld(fb) && !forcePushToWorld)
      return;

    m_worldBucket.CalcStatistics(fb);

    if (!m_boundaryChecker.IsBoundaries(fb))
    {
      // Save original feature iff we need to force push it before PushFeature(fb) modifies fb.
      auto originalFeature = forcePushToWorld ? fb : FeatureBuilder1();

      if (PushFeature(fb) || !forcePushToWorld)
        return;

      // We push GEOM_POINT with all the same tags, names and center instead of GEOM_WAY/GEOM_AREA
      // because we do not need geometry for invisible features (just search index and placepage
      // data) and want to avoid size checks applied to areas.
      if (originalFeature.GetGeomType() != feature::GEOM_POINT)
        originalFeature.SetCenter(originalFeature.GetGeometryCenter());

      m_worldBucket.PushSure(originalFeature);
      return;
    }

    std::vector<FeatureBuilder1> boundaryParts;
    m_boundaryChecker.ProcessBoundary(fb, boundaryParts);
    for (auto & f : boundaryParts)
      PushFeature(f);
  }

  bool PushFeature(FeatureBuilder1 & fb)
  {
    switch (fb.GetGeomType())
    {
    case feature::GEOM_LINE:
    {
      MergedFeatureBuilder1 * p = m_typesCorrector(fb);
      if (p)
        m_merger(p);
      return false;
    }
    case feature::GEOM_AREA:
    {
      // This constant is set according to size statistics.
      // Added approx 4Mb of data to the World.mwm
      auto const & geometry = fb.GetOuterGeometry();
      if (GetPolygonArea(geometry.begin(), geometry.end()) < 0.01)
        return false;
    }
    default:
      break;
    }

    if (feature::PreprocessForWorldMap(fb))
    {
      m_worldBucket.PushSure(fb);
      return true;
    }

    return false;
  }

  void DoMerge() { m_merger.DoMerge(m_worldBucket); }

private:
  bool IsPopularAttraction(FeatureBuilder1 const & fb) const
  {
    if (fb.GetName().empty())
      return false;

    auto const attractionTypes =
        search::GetCategoryTypes("attractions", "en", GetDefaultCategories());
    ASSERT(is_sorted(attractionTypes.begin(), attractionTypes.end()), ());
    auto const & featureTypes = fb.GetTypes();
    if (!std::any_of(featureTypes.begin(), featureTypes.end(), [&attractionTypes](uint32_t t) {
          return binary_search(attractionTypes.begin(), attractionTypes.end(), t);
        }))
    {
      return false;
    }

    auto const it = m_popularPlaces.find(fb.GetMostGenericOsmId());
    if (it == m_popularPlaces.end())
      return false;

    // todo(@t.yan): adjust
    uint8_t const kPopularityThreshold = 11;
    if (it->second < kPopularityThreshold)
      return false;

    // todo(@t.yan): maybe check place has wikipedia link.
    return true;
  }
};

template <class FeatureOut>
class SimpleCountryMapGenerator
{
public:
  SimpleCountryMapGenerator(feature::GenerateInfo const & info) : m_bucket(info) {}

  void operator()(FeatureBuilder1 & fb)
  {
      m_bucket(fb);
  }

  FeatureOut & Parent() { return m_bucket; }

private:
  FeatureOut m_bucket;
};

template <class FeatureOut>
class CountryMapGenerator : public SimpleCountryMapGenerator<FeatureOut>
{
public:
  CountryMapGenerator(feature::GenerateInfo const & info) :
    SimpleCountryMapGenerator<FeatureOut>(info) {}

  void operator()(FeatureBuilder1 fb)
  {
    if (feature::PreprocessForCountryMap(fb))
      SimpleCountryMapGenerator<FeatureOut>::operator()(fb);
  }
};
