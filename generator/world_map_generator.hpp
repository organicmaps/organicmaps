#pragma once

#include "feature_merger.hpp"

#include "../indexer/scales.hpp"

#include "../defines.hpp"


/// Process FeatureBuilder1 for world map. Main functions:
/// - check for visibility in world map
/// - merge linear features
template <class FeatureOutT>
class WorldMapGenerator
{
  class EmitterImpl : public FeatureEmitterIFace
  {
    FeatureOutT m_output;

  public:
    template <class TInfo>
    explicit EmitterImpl(TInfo const & info)
      : m_output(info.m_datFilePrefix + WORLD_FILE_NAME + info.m_datFileSuffix)
    {
    }

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (NeedPushToWorld(fb) && scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
    }

    bool NeedPushToWorld(FeatureBuilder1 const & fb) const
    {
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
    : m_worldBucket(info), m_merger(POINT_COORD_BITS)
  {
    // Do not strip last types for given tags,
    // for example, do not cut 'admin_level' in  'boundary-administrative-XXX'.
    char const * arr3[][3] = {
      { "boundary", "administrative", "2" },
      { "boundary", "administrative", "3" },
      { "boundary", "administrative", "4" }
    };

    for (size_t i = 0; i < ARRAY_SIZE(arr3); ++i)
      m_typesCorrector.SetDontNormalizeType(arr3[i]);

    char const * arr4[]  = { "boundary", "administrative", "4", "state" };
    m_typesCorrector.SetDontNormalizeType(arr4);
  }

  void operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket.NeedPushToWorld(fb))
    {
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
