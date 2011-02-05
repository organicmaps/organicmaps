#pragma once

#include "../../defines.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_visibility.hpp"

#include <boost/scoped_ptr.hpp>

template <class FeatureOutT>
class WorldMapGenerator
{
  /// if NULL, separate world data file is not generated
  boost::scoped_ptr<FeatureOutT> m_worldBucket;
  /// features visible before or at this scale level will go to World map
  int m_maxWorldScale;

public:
  WorldMapGenerator(int maxWorldScale, typename FeatureOutT::InitDataType featureOutInitData)
  : m_maxWorldScale(maxWorldScale)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new FeatureOutT(WORLD_FILE_NAME, featureOutInitData));
  }

  bool operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket)
    {
      int minScale = feature::MinDrawableScaleForFeature(fb.GetFeatureBase());
      CHECK_GREATER(minScale, -1, ("Non-drawable feature found!?"));
      // separately store features needed for world map
      if (m_maxWorldScale >= minScale)
      {
        (*m_worldBucket)(fb);
        return true;
      }
    }
    return false;
  }
};
