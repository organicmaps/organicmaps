#pragma once

#include "generator/feature_maker_base.hpp"
#include "generator/feature_merger.hpp"
#include "generator/filter_world.hpp"
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
  WaterBoundaryChecker(std::string const & rawGeometryFileName)
  {
    LoadWaterGeometry(rawGeometryFileName);
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

  bool IsBoundaries(feature::FeatureBuilder const & fb)
  {
    ++m_totalFeatures;

    auto static const kBoundaryType = classif().GetTypeByPath({"boundary", "administrative"});
    if (fb.FindType(kBoundaryType, 2) == ftype::GetEmptyValue())
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

  void ProcessBoundary(feature::FeatureBuilder const & boundary, std::vector<feature::FeatureBuilder> & parts)
  {
    auto const & line = boundary.GetGeometry().front();

    double constexpr kExtension = 0.01;
    ProcessState state = ProcessState::Initial;

    feature::FeatureBuilder::PointSeq points;

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

/// Process FeatureBuilder for world map. Main functions:
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
    explicit EmitterImpl(std::string const & worldFilename)
      : m_output(worldFilename)
    {
      LOG_SHORT(LINFO, ("Output World file:", worldFilename));
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
    void operator()(feature::FeatureBuilder const & fb) override
    {
      // do additional check for suitable size of feature
      if (NeedPushToWorld(fb) &&
          scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    void CalcStatistics(feature::FeatureBuilder const & fb)
    {
      for (uint32_t type : fb.GetTypes())
        ++m_mapTypes[type];
    }

    bool NeedPushToWorld(feature::FeatureBuilder const & fb) const
    {
      return generator::FilterWorld::IsGoodScale(fb);
    }

    void PushSure(feature::FeatureBuilder const & fb)
    {
      CalcStatistics(fb);
      m_output.Collect(fb);
    }
  };

  EmitterImpl m_worldBucket;
  FeatureTypesProcessor m_typesCorrector;
  FeatureMergeProcessor m_merger;
  WaterBoundaryChecker m_boundaryChecker;
  std::string m_popularPlacesFilename;

public:
  explicit WorldMapGenerator(std::string const & worldFilename, std::string const & rawGeometryFileName,
                             std::string const & popularPlacesFilename)
    : m_worldBucket(worldFilename)
    , m_merger(kPointCoordBits - (scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2)
    , m_boundaryChecker(rawGeometryFileName)
    , m_popularPlacesFilename(popularPlacesFilename)
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

    if (popularPlacesFilename.empty())
      LOG(LWARNING, ("popular_places_data option not set. Popular atractions will not be added to World.mwm"));
  }

  void Process(feature::FeatureBuilder & fb)
  {
    auto const forcePushToWorld = generator::FilterWorld::IsPopularAttraction(fb, m_popularPlacesFilename) ||
                                  generator::FilterWorld::IsInternationalAirport(fb);

    if (!m_worldBucket.NeedPushToWorld(fb) && !forcePushToWorld)
      return;

    if (!m_boundaryChecker.IsBoundaries(fb))
    {
      // Save original feature iff we need to force push it before PushFeature(fb) modifies fb.
      auto originalFeature = forcePushToWorld ? fb : feature::FeatureBuilder();

      if (PushFeature(fb) || !forcePushToWorld)
        return;

      // We push Point with all the same tags, names and center instead of GEOM_WAY/Area
      // because we do not need geometry for invisible features (just search index and placepage
      // data) and want to avoid size checks applied to areas.
      if (originalFeature.GetGeomType() != feature::GeomType::Point)
        generator::TransformToPoint(originalFeature);

      m_worldBucket.PushSure(originalFeature);
      return;
    }

    std::vector<feature::FeatureBuilder> boundaryParts;
    m_boundaryChecker.ProcessBoundary(fb, boundaryParts);
    for (auto & f : boundaryParts)
      PushFeature(f);
  }

  bool PushFeature(feature::FeatureBuilder & fb)
  {
    switch (fb.GetGeomType())
    {
    case feature::GeomType::Line:
    {
      MergedFeatureBuilder * p = m_typesCorrector(fb);
      if (p)
        m_merger(p);
      return false;
    }
    case feature::GeomType::Area:
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
};

template <class FeatureOut>
class SimpleCountryMapGenerator
{
public:
  SimpleCountryMapGenerator(feature::GenerateInfo const & info) : m_bucket(info) {}

  void operator()(feature::FeatureBuilder & fb)
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

  void Process(feature::FeatureBuilder fb)
  {
    if (feature::PreprocessForCountryMap(fb))
      SimpleCountryMapGenerator<FeatureOut>::operator()(fb);
  }
};
