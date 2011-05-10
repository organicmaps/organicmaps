#pragma once

#include "feature_merger.hpp"

#include "../indexer/feature_visibility.hpp"

#include "../defines.hpp"


template <class FeatureOutT>
class WorldMapGenerator
{
  class WorldEmitter : public FeatureEmitterIFace
  {
    FeatureOutT m_output;
  public:
    template <class TInit> WorldEmitter(TInit const & initData)
      : m_output(WORLD_FILE_NAME, initData)
    {
    }
    virtual void operator() (FeatureBuilder1 const & fb)
    {
      m_output(fb);
    }
  };

  /// if NULL, separate world data file is not generated
  scoped_ptr<WorldEmitter> m_worldBucket;

  /// features visible before or at this scale level will go to World map
  int m_maxWorldScale;
  bool m_mergeCoastlines;

  FeatureMergeProcessor m_processor;

public:
  WorldMapGenerator(int maxWorldScale, bool mergeCoastlines,
                    typename FeatureOutT::InitDataType const & initData)
  : m_maxWorldScale(maxWorldScale), m_mergeCoastlines(mergeCoastlines), m_processor(30)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new WorldEmitter(initData));

    // fill vector with types that need to be merged
    //static size_t const MAX_TYPES_IN_PATH = 3;
    //char const * arrMerge[][MAX_TYPES_IN_PATH] = {
    //  {"natural", "coastline", ""},
    //  {"boundary", "administrative", "2"},

    //  {"highway", "motorway", ""},
    //  {"highway", "motorway_link", ""},
    //  {"highway", "motorway", "oneway"},
    //  {"highway", "motorway_link", "oneway"},

    //  {"highway", "primary", ""},
    //  {"highway", "primary_link", ""},

    //  {"highway", "trunk", ""},
    //  {"highway", "trunk_link", ""},

    //  {"natural", "water", ""}
    //};
  }
  ~WorldMapGenerator()
  {
    DoMerge();
  }

  void operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket)
    {
      FeatureBase fBase = fb.GetFeatureBase();
      int const minScale = feature::MinDrawableScaleForFeature(fBase);
      CHECK_GREATER(minScale, -1, ("Non-drawable feature found!?"));

      if (m_maxWorldScale >= minScale)
      {
        if (m_mergeCoastlines && fBase.GetFeatureType() == feature::GEOM_LINE)
          m_processor(fb);
        else
          (*m_worldBucket)(fb);
      }
    }
  }

  void DoMerge()
  {
    if (m_worldBucket)
      m_processor.DoMerge(*m_worldBucket);
  }
};
