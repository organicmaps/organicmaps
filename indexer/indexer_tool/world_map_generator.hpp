#pragma once

#include "../../base/logging.hpp"

#include "../../defines.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_merger.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../std/list.hpp"

#include <boost/scoped_ptr.hpp>

template <class FeatureOutT>
class WorldMapGenerator
{
  /// if NULL, separate world data file is not generated
  boost::scoped_ptr<FeatureOutT> m_worldBucket;
  /// features visible before or at this scale level will go to World map
  int m_maxWorldScale;
  bool m_mergeCoastlines;

  typedef list<FeatureBuilder1Merger> FeaturesContainerT;
  FeaturesContainerT m_features;

private:
  /// scans all features and tries to merge them with each other
  /// @return true if one feature was merged
  bool ReMergeFeatures()
  {
    bool merged = false;
    for (FeaturesContainerT::iterator base = m_features.begin(); base != m_features.end(); ++base)
    {
      FeaturesContainerT::iterator ft = base;
      for (++ft; ft != m_features.end();)
      {
        if (base->MergeWith(*ft))
        {
          m_features.erase(ft++);
          merged = true;
        }
        else
          ++ft;
      }
    }
    return merged;
  }

  void TryToMerge(FeatureBuilder1 const & fb)
  {
    // @TODO group features by types (use map of lists?)
    for (FeaturesContainerT::iterator it = m_features.begin(); it != m_features.end(); ++it)
    {
      if (it->MergeWith(fb))
        return;
    }
    // do not loose feature if it wasn't merged
    m_features.push_back(fb);
  }

public:
  WorldMapGenerator(int maxWorldScale, bool mergeCoastlines,
                    typename FeatureOutT::InitDataType featureOutInitData)
  : m_maxWorldScale(maxWorldScale), m_mergeCoastlines(mergeCoastlines)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new FeatureOutT(WORLD_FILE_NAME, featureOutInitData));
  }

  ~WorldMapGenerator()
  {
    if (m_mergeCoastlines)
    {
      LOG(LINFO, ("Final merging of coastlines started"));
    }
    // try to merge all merged features with each other
    while (ReMergeFeatures())
    {
    }

    // emit all merged features
    for (FeaturesContainerT::iterator it = m_features.begin(); it != m_features.end(); ++it)
      (*m_worldBucket)(*it);

    if (m_mergeCoastlines)
    {
      LOG(LINFO, ("Final merging of coastlines ended"));
    }

  }

  bool operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket)
    {
      FeatureBase fBase = fb.GetFeatureBase();
      int minScale = feature::MinDrawableScaleForFeature(fBase);
      CHECK_GREATER(minScale, -1, ("Non-drawable feature found!?"));
      if (m_maxWorldScale >= minScale)
      {
        if (m_mergeCoastlines)
        {
          // we're merging only linear features,
          // areas and points are written immediately
          if (fBase.GetFeatureType() != FeatureBase::FEATURE_TYPE_LINE)
            (*m_worldBucket)(fb);
          else
            TryToMerge(fb);
        }
        else
          (*m_worldBucket)(fb);
        return true;
      }
    }
    return false;
  }
};
