#pragma once

#include "generator/feature_maker_base.hpp"
#include "generator/feature_merger.hpp"
#include "generator/filter_world.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/polygon.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "water_boundary_checker.hpp"

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
    explicit EmitterImpl(std::string const & worldFilename) : m_output(worldFilename)
    {
      LOG_SHORT(LINFO, ("Output World file:", worldFilename));
    }

    ~EmitterImpl()
    {
      Classificator const & c = classif();

      std::stringstream ss;
      ss << std::endl;
      for (auto const & p : m_mapTypes)
        ss << c.GetReadableObjectName(p.first) << " : " << p.second << std::endl;
      LOG_SHORT(LINFO, ("World types:", ss.str()));
    }

    // This functor is called by m_merger after merging linear features.
    void operator()(feature::FeatureBuilder const & fb) override
    {
      static uint32_t const ferryType = classif().GetTypeByPath({"route", "ferry"});
      static uint32_t const boundaryType = classif().GetTypeByPath({"boundary", "administrative"});
      static uint32_t const highwayType = classif().GetTypeByPath({"highway"});

      int thresholdLevel = scales::GetUpperWorldScale();
      if (fb.HasType(ferryType) || fb.HasType(boundaryType, 2))
      {
        // Discard too short ferry and boundary lines
        // (boundaries along the coast are being "torn" into small pieces
        // by the coastline in WaterBoundaryChecker::ProcessBoundary()).
        thresholdLevel = scales::GetUpperWorldScale() - 2;
      }
      else if (fb.HasType(highwayType, 1))
      {
        // Discard too short roads incl. V-like approaches to roundabouts / other roads
        // and small roundabouts that were not merged into longer roads for some reason.
        thresholdLevel = scales::GetUpperWorldScale() + 2;
      }

      // TODO(pastk): there seems to be two area size checks: here and in PushFeature().
      if (scales::IsGoodForLevel(thresholdLevel, fb.GetLimitRect()))
        PushSure(fb);
    }

    void CalcStatistics(feature::FeatureBuilder const & fb)
    {
      for (uint32_t type : fb.GetTypes())
        ++m_mapTypes[type];
    }

    bool NeedPushToWorld(feature::FeatureBuilder const & fb) const { return generator::FilterWorld::IsGoodScale(fb); }

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
  /// @param[in]  coastGeomFilename     Can be empty if no need to cut borders by water.
  /// @param[in]  popularPlacesFilename Can be empty if no need in popular places.
  WorldMapGenerator(std::string const & worldFilename, std::string const & coastGeomFilename,
                    std::string const & popularPlacesFilename)
    : m_worldBucket(worldFilename)
    , m_merger(kFeatureSorterPointCoordBits - (scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2)
    , m_popularPlacesFilename(popularPlacesFilename)
  {
    // Do not strip last types for given tags,
    // for example, do not cut 'admin_level' in  'boundary-administrative-XXX'.
    char const * arr1[][3] = {
        {"boundary", "administrative", "2"}, {"boundary", "administrative", "3"}, {"boundary", "administrative", "4"}};

    for (size_t i = 0; i < ARRAY_SIZE(arr1); ++i)
      m_typesCorrector.SetDontNormalizeType(arr1[i]);

    // Merge motorways into trunks.
    // TODO : merge e.g. highway-trunk_link into highway-trunk?
    char const *marr1[2] = {"highway", "motorway"}, *marr2[2] = {"highway", "trunk"};
    m_typesCorrector.SetMappingTypes(marr1, marr2);

    if (popularPlacesFilename.empty())
      LOG(LWARNING, ("popular_places_data option not set. Popular attractions will not be added to World.mwm"));

    // Can be empty in tests.
    if (!coastGeomFilename.empty())
      m_boundaryChecker.LoadWaterGeometry(coastGeomFilename);
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
      feature::FeatureBuilder originalFeature;
      if (forcePushToWorld)
        originalFeature = fb;

      if (!PushFeature(fb) && forcePushToWorld)
      {
        // We push Point with all the same tags, names and center instead of Line/Area,
        // because we do not need geometry for invisible features (just search index and placepage
        // data) and want to avoid size checks applied to areas.
        if (!originalFeature.IsPoint())
          generator::TransformToPoint(originalFeature);

        m_worldBucket.PushSure(originalFeature);
      }
    }
    else
    {
      std::vector<feature::FeatureBuilder> boundaryParts;
      m_boundaryChecker.ProcessBoundary(fb, boundaryParts);
      for (auto & f : boundaryParts)
        PushFeature(f);
    }
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
      /// @todo Initial area threshold to push area objects into World.mwm
      auto const & geometry = fb.GetOuterGeometry();
      if (GetPolygonArea(geometry.begin(), geometry.end()) < 0.0025)
        return false;
    }
    default: break;
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
