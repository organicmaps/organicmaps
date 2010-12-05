#pragma once

#include "../../base/base.hpp"

#include "../../coding/file_writer.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../std/map.hpp"
#include "../../std/string.hpp"

#include <boost/scoped_ptr.hpp>

#define WORLD_FILE_NAME "world"

namespace feature
{

// Groups features in buckets according to their coordinates.
template <class FeatureOutT, class FeatureClipperT, class BoundsT, typename CellIdT>
class CellFeatureBucketer
{
public:
  CellFeatureBucketer(int level, typename FeatureOutT::InitDataType const & featureOutInitData,
                      int maxWorldZoom = -1)
  : m_Level(level), m_FeatureOutInitData(featureOutInitData), m_maxWorldZoom(maxWorldZoom)
  {
    uint32_t const size = 1 << 2 * m_Level;
    m_Buckets.resize(size);
    for (uint32_t i = 0; i < m_Buckets.size(); ++i)
    {
      CellIdT cell = CellIdT::FromBitsAndLevel(i, m_Level);
      double minX, minY, maxX, maxY;
      CellIdConverter<BoundsT, CellIdT>::GetCellBounds(cell, minX, minY, maxX, maxY);
      m_Buckets[i].m_Rect = m2::RectD(minX, minY, maxX, maxY);
    }
    // create separate world bucket if necessary
    if (maxWorldZoom >= 0)
    {
      m_worldBucket.reset(new FeatureOutT(WORLD_FILE_NAME, m_FeatureOutInitData));
    }
  }

  void operator () (Feature const & feature)
  {
    // separately store features needed for world map
    if (m_worldBucket
        && m_maxWorldZoom >= feature::MinDrawableScaleForFeature(feature))
    {
      (*m_worldBucket)(feature);
    }

    FeatureClipperT clipper(feature);
    // TODO: Is feature fully inside GetLimitRect()?
    m2::RectD const limitRect = feature.GetLimitRect();
    for (uint32_t i = 0; i < m_Buckets.size(); ++i)
    {
      // First quick and dirty limit rect intersection.
      // Clipper may (or may not) do a better intersection.
      if (m_Buckets[i].m_Rect.IsIntersect(limitRect))
      {
        Feature clippedFeature;
        if (clipper(m_Buckets[i].m_Rect, clippedFeature))
        {
          if (!m_Buckets[i].m_pOut)
            m_Buckets[i].m_pOut = new FeatureOutT(BucketName(i), m_FeatureOutInitData);

          (*(m_Buckets[i].m_pOut))(clippedFeature);
        }
      }
    }
  }

  void operator () (FeatureBuilder const & fb) { (*this)(fb.GetFeature()); }

  template <typename F> void GetBucketNames(F f) const
  {
    for (uint32_t i = 0; i < m_Buckets.size(); ++i)
      if (m_Buckets[i].m_pOut)
        f(BucketName(i));
  }

private:
  inline string BucketName(uint32_t i) const
  {
    return CellIdT::FromBitsAndLevel(i, m_Level).ToString();
  }

  struct Bucket
  {
    Bucket() : m_pOut(NULL) {}
    ~Bucket() { delete m_pOut; }

    FeatureOutT * m_pOut;
    m2::RectD m_Rect;
  };

  int m_Level;
  typename FeatureOutT::InitDataType m_FeatureOutInitData;
  vector<Bucket> m_Buckets;
  /// if NULL, separate world data file is not generated
  boost::scoped_ptr<FeatureOutT> m_worldBucket;
  int m_maxWorldZoom;
};

class SimpleFeatureClipper
{
public:
  explicit SimpleFeatureClipper(Feature const & feature) : m_Feature(feature)
  {
  }

  bool operator () (m2::RectD const & /*rect*/, Feature & clippedFeature) const
  {
    clippedFeature = m_Feature;
    return true;
  }

private:
  Feature const & m_Feature;
};

}
