#pragma once

#include "generator/feature_merger.hpp"
#include "generator/generate_info.hpp"

#include "indexer/scales.hpp"

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

  public:
    explicit EmitterImpl(feature::GenerateInfo const & info)
      : m_output(info.GetTmpFileName(WORLD_FILE_NAME))
    {
      LOG(LINFO, ("Output World file:", info.GetTmpFileName(WORLD_FILE_NAME)));
    }

    /// This function is called after merging linear features.
    virtual void operator() (FeatureBuilder1 const & fb)
    {
      // do additional check for suitable size of feature
      if (NeedPushToWorld(fb) && scales::IsGoodForLevel(scales::GetUpperWorldScale(), fb.GetLimitRect()))
        PushSure(fb);
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
    char const * arr1[][3] = {
      { "boundary", "administrative", "2" },
      { "boundary", "administrative", "3" },
      { "boundary", "administrative", "4" }
    };

    for (size_t i = 0; i < ARRAY_SIZE(arr1); ++i)
      m_typesCorrector.SetDontNormalizeType(arr1[i]);

    char const * arr2[]  = { "boundary", "administrative", "4", "state" };
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

  void DoMerge()
  {
    m_merger.DoMerge(m_worldBucket);
  }
};

template <class FeatureOutT>
class CountryMapGenerator
{
  FeatureOutT m_bucket;

public:
  template <class TInfo>
  explicit CountryMapGenerator(TInfo const & info) : m_bucket(info) {}

  void operator()(FeatureBuilder1 fb)
  {
    if (feature::PreprocessForCountryMap(fb))
      m_bucket(fb);
  }

  FeatureOutT const & Parent() const { return m_bucket; }
};
