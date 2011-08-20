#pragma once

#include "feature_merger.hpp"
#include "generate_info.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/scales.hpp"

#include "../defines.hpp"


inline int GetMinFeatureDrawScale(FeatureBuilder1 const & fb)
{
  FeatureBase const fBase = fb.GetFeatureBase();
  int const minScale = feature::MinDrawableScaleForFeature(fBase);

  // some features become invisible after merge processing, so -1 is possible
  return (minScale == -1 ? 1000 : minScale);
}

template <class FeatureOutT>
class WorldMapGenerator
{
  class WorldEmitter : public FeatureEmitterIFace
  {
    FeatureOutT m_output;

  public:
    template <class TInit>
    WorldEmitter(TInit const & initData) : m_output(WORLD_FILE_NAME, initData)
    {
    }

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (NeedPushToWorld(fb) && scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    bool NeedPushToWorld(FeatureBuilder1 const & fb) const
    {
      return (scales::GetUpperWorldScale() >= GetMinFeatureDrawScale(fb));
    }

    void PushSure(FeatureBuilder1 const & fb) { m_output(fb); }
  };

  WorldEmitter m_worldBucket;
  FeatureTypesProcessor m_typesCorrector;
  FeatureMergeProcessor m_merger;

public:
  template <class T>
  WorldMapGenerator(T const & info) : m_worldBucket(typename FeatureOutT::InitDataType(
        info.m_datFilePrefix, info.m_datFileSuffix)), m_merger(30)
  {
    // Do not strip last types for given tags,
    // for example, do not cut "-2" in  "boundary-administrative-2"
    char const * arrDontNormalize[][3] = {
      { "boundary", "administrative", "2" }
    };

    for (size_t i = 0; i < ARRAY_SIZE(arrDontNormalize); ++i)
      m_typesCorrector.SetDontNormalizeType(arrDontNormalize[i]);
  }

  ~WorldMapGenerator()
  {
    DoMerge();
  }

  void operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket.NeedPushToWorld(fb))
    {
      // Always try to merge coastlines
      if (fb.GetGeomType() == feature::GEOM_LINE)
        m_merger(m_typesCorrector(fb));
      else
        m_worldBucket.PushSure(fb);
    }
  }

  void DoMerge()
  {
    m_merger.DoMerge(m_worldBucket);
  }
};
