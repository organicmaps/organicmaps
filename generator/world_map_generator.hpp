#pragma once

#include "feature_merger.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/scales.hpp"

#include "../defines.hpp"


inline int GetMinFeatureDrawScale(FeatureBuilder1 const & fb)
{
  FeatureBase fBase = fb.GetFeatureBase();
  int const minScale = feature::MinDrawableScaleForFeature(fBase);
  CHECK_GREATER(minScale, -1, ("Non-drawable feature found!?"));
  return minScale;
}


template <class FeatureOutT>
class WorldMapGenerator
{
  class WorldEmitter : public FeatureEmitterIFace
  {
    FeatureOutT m_output;
    int m_maxWorldScale;

  public:
    template <class TInit>
    WorldEmitter(int maxScale, TInit const & initData)
      : m_maxWorldScale(maxScale), m_output(WORLD_FILE_NAME, initData)
    {
    }

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (NeedPushToWorld(fb) && scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    bool NeedPushToWorld(FeatureBuilder1 const & fb) const
    {
      return (m_maxWorldScale >= GetMinFeatureDrawScale(fb));
    }

    void PushSure(FeatureBuilder1 const & fb) { m_output(fb); }
  };

  /// if NULL, separate world data file is not generated
  scoped_ptr<WorldEmitter> m_worldBucket;

  /// features visible before or at this scale level will go to World map
  bool m_mergeCoastlines;

  FeatureMergeProcessor m_processor;

public:
  WorldMapGenerator(int maxWorldScale, bool mergeCoastlines,
                    typename FeatureOutT::InitDataType const & initData)
  : m_mergeCoastlines(mergeCoastlines), m_processor(30)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new WorldEmitter(maxWorldScale, initData));

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
    if (m_worldBucket && m_worldBucket->NeedPushToWorld(fb))
    {
      if (m_mergeCoastlines && (fb.GetGeomType() == feature::GEOM_LINE))
        m_processor(fb);
      else
        m_worldBucket->PushSure(fb);
    }
  }

  void DoMerge()
  {
    if (m_worldBucket)
      m_processor.DoMerge(*m_worldBucket);
  }
};
