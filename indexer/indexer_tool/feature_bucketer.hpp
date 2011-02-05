#pragma once

#include "../../base/base.hpp"
#include "../../base/logging.hpp"

#include "../../coding/file_writer.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../defines.hpp"

#include "../../std/string.hpp"

#include <boost/scoped_ptr.hpp>

namespace feature
{

// Groups features in buckets according to their coordinates.
template <class FeatureOutT, class FeatureClipperT, class BoundsT, typename CellIdT>
class CellFeatureBucketer
{
  typedef typename FeatureClipperT::feature_builder_t feature_builder_t;

  void Init()
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
    if (m_maxWorldZoom >= 0)
      m_worldBucket.reset(new FeatureOutT(WORLD_FILE_NAME, m_FeatureOutInitData));
  }

public:
  template <class TInfo>
  explicit CellFeatureBucketer(TInfo & info)
  : m_Level(info.cellBucketingLevel), m_FeatureOutInitData(info.datFilePrefix, info.datFileSuffix),
    m_maxWorldZoom(info.maxScaleForWorldFeatures)
  {
    Init();
  }

  CellFeatureBucketer(int level, typename FeatureOutT::InitDataType const & initData)
    : m_Level(level), m_FeatureOutInitData(initData), m_maxWorldZoom(-1)
  {
    Init();
  }

  void operator () (feature_builder_t const & fb)
  {
    int minScale = feature::MinDrawableScaleForFeature(fb.GetFeatureBase());
    if (minScale == -1)
    {
      LOG(LWARNING, ("Non-drawable feature found, ignoring"));
      return;
    }
    // separately store features needed for world map
    if (m_worldBucket && m_maxWorldZoom >= minScale)
      (*m_worldBucket)(fb);

    FeatureClipperT clipper(fb);
    // TODO: Is feature fully inside GetLimitRect()?
    m2::RectD const limitRect = fb.GetLimitRect();
    for (uint32_t i = 0; i < m_Buckets.size(); ++i)
    {
      // First quick and dirty limit rect intersection.
      // Clipper may (or may not) do a better intersection.
      if (m_Buckets[i].m_Rect.IsIntersect(limitRect))
      {
        feature_builder_t clippedFb;
        if (clipper(m_Buckets[i].m_Rect, clippedFb))
        {
          if (!m_Buckets[i].m_pOut)
            m_Buckets[i].m_pOut = new FeatureOutT(BucketName(i), m_FeatureOutInitData);

          (*(m_Buckets[i].m_pOut))(clippedFb);
        }
      }
    }
  }

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
  typedef FeatureBuilder1 feature_builder_t;

private:
  feature_builder_t const & m_Feature;

public:
  explicit SimpleFeatureClipper(feature_builder_t const & f) : m_Feature(f)
  {
  }

  bool operator () (m2::RectD const & /*rect*/, feature_builder_t & clippedF) const
  {
    clippedF = m_Feature;
    return true;
  }
};

}
