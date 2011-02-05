#pragma once

#include "world_map_generator.hpp"

#include "../../base/base.hpp"

#include "../../coding/file_writer.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../indexer/feature.hpp"

#include "../../std/string.hpp"

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
  }

public:
  template <class TInfo>
  explicit CellFeatureBucketer(TInfo & info)
  : m_Level(info.cellBucketingLevel), m_FeatureOutInitData(info.datFilePrefix, info.datFileSuffix),
    m_worldMap(info.maxScaleForWorldFeatures, m_FeatureOutInitData)
  {
    Init();
  }

  /// @note this constructor doesn't support world file generation
  CellFeatureBucketer(int level, typename FeatureOutT::InitDataType const & initData)
    : m_Level(level), m_FeatureOutInitData(initData), m_worldMap(-1, initData)
  {
    Init();
  }

  void operator () (feature_builder_t const & fb)
  {
    if (m_worldMap(fb))
      return; // we do not duplicate features in world and bucket files

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
  WorldMapGenerator<FeatureOutT> m_worldMap;
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
