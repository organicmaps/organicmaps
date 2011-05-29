#pragma once

#include "feature_merger.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/scales.hpp"

#include "../defines.hpp"


inline int GetMinFeatureDrawScale(FeatureBuilder1 const & fb)
{
  FeatureBase fBase = fb.GetFeatureBase();
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
    int m_maxWorldScale;

  public:
    template <class TInit>
    WorldEmitter(int maxScale, TInit const & initData)
      : m_output(WORLD_FILE_NAME, initData), m_maxWorldScale(maxScale)
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

  FeatureTypesProcessor m_typesCorrector;
  FeatureMergeProcessor m_merger;

public:
  WorldMapGenerator(int maxWorldScale, bool mergeCoastlines,
                    typename FeatureOutT::InitDataType const & initData)
  : m_mergeCoastlines(mergeCoastlines), m_merger(30)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new WorldEmitter(maxWorldScale, initData));

    // fill vector with types that need to be replaced
    char const * arrReplace[][3] = {
      {"highway", "motorway_link", "motorway"},
      {"highway", "motorway_junction", "motorway"},
      {"highway", "primary_link", "primary"},
      {"highway", "trunk_link", "trunk"},
      {"highway", "secondary_link", "secondary"},
      {"highway", "tertiary_link", "tertiary"}
    };

    for (size_t i = 0; i < ARRAY_SIZE(arrReplace); ++i)
    {
      char const * arr1[] = { arrReplace[i][0], arrReplace[i][1] };
      char const * arr2[] = { arrReplace[i][0], arrReplace[i][2] };

      m_typesCorrector.SetMappingTypes(arr1, arr2);
    }
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
        m_merger(m_typesCorrector(fb));
      else
        m_worldBucket->PushSure(fb);
    }
  }

  void DoMerge()
  {
    if (m_worldBucket)
      m_merger.DoMerge(*m_worldBucket);
  }
};
