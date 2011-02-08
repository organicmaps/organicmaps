#pragma once

#include "../../base/logging.hpp"

#include "../../defines.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_merger.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../std/map.hpp"
#include "../../std/list.hpp"
#include "../../std/vector.hpp"

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
  typedef map<vector<uint32_t>, FeaturesContainerT> TypesContainerT;
  TypesContainerT m_features;

private:
  /// scans all features and tries to merge them with each other
  /// @return true if one feature was merged
  bool ReMergeFeatures(FeaturesContainerT & features)
  {
    bool merged = false;
    for (FeaturesContainerT::iterator base = features.begin(); base != features.end();)
    {
      FeaturesContainerT::iterator ft = base;
      for (++ft; ft != features.end();)
      {
        if (base->MergeWith(*ft))
        {
          features.erase(ft++);
          merged = true;
        }
        else if (base->ReachedMaxPointsCount())
          break;
        else
          ++ft;
      }

      if (base->ReachedMaxPointsCount())
      {
        // emit "overflowed" features
        (*m_worldBucket)(*base);
        features.erase(base++);
      }
      else
        ++base;
    }
    return merged;
  }

  void TryToMerge(FeatureBuilder1 const & fb)
  {
    FeatureBuilder1Merger const fbm(fb);
    m_features[fbm.Type()].push_back(fbm);
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
    for (TypesContainerT::iterator it = m_features.begin(); it != m_features.end(); ++it)
    {
      while (ReMergeFeatures(it->second))
      {}
      // emit all merged features
      for (FeaturesContainerT::iterator itF = it->second.begin(); itF != it->second.end(); ++itF)
        (*m_worldBucket)(*itF);
    }

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
