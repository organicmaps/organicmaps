#include "tile_info.hpp"

#include "../map/feature_vec_model.hpp"
#include "../indexer/mercator.hpp"

#include "line_shape.hpp"
#include "area_shape.hpp"
#include "engine_context.hpp"

namespace
{
  df::AreaShape * CreateFakeShape1()
  {
    df::AreaShape * shape = new df::AreaShape(Extract(0xFFEEAABB), 0.3f);
    shape->AddTriangle(m2::PointF(0.0f, 0.0f),
                       m2::PointF(1.0f, 0.0f),
                       m2::PointF(0.0f, 1.0f));

    shape->AddTriangle(m2::PointF(1.0f, 0.0f),
                       m2::PointF(0.0f, 1.0f),
                       m2::PointF(1.0f, 1.0f));
    return shape;
  }

  df::AreaShape * CreateFakeShape2()
  {
    df::AreaShape * shape = new df::AreaShape(Extract(0xFF66AAFF), 0.0f);
    shape->AddTriangle(m2::PointF(-0.5f, 0.5f),
                       m2::PointF(0.5f, 1.5f),
                       m2::PointF(0.5f, -0.5f));

    shape->AddTriangle(m2::PointF(0.5f, -0.5f),
                       m2::PointF(0.5f, 1.5f),
                       m2::PointF(1.5f, 0.5f));
    return shape;
  }

  df::LineShape * CreateFakeLine1()
  {
    vector<m2::PointF> points;
    const float magn = 4;

    for (float x = -4*math::pi; x <= 4*math::pi; x+= math::pi/32)
      points.push_back(m2::PointF(x, magn*sinf(x)));

    return new df::LineShape(points, Extract(0xFFFF0000), .0f, 1.f);
  }

  df::LineShape * CreateFakeLine2()
  {
    vector<m2::PointF> points;
    points.push_back(m2::PointF(0,0));
    points.push_back(m2::PointF(0,0)); // to check for zero-normal
    points.push_back(m2::PointF(4,0));
    points.push_back(m2::PointF(8,4));
    points.push_back(m2::PointF(4,4));
    points.push_back(m2::PointF(0,4));
    points.push_back(m2::PointF(0,0));
    return new df::LineShape(points, Extract(0xFF00FF00), .5f, 0.5f);
  }
}

namespace df
{

  TileInfo::TileInfo(TileKey const & key)
  : m_key(key)
{}

m2::RectD TileInfo::GetGlobalRect() const
{
  double const worldSizeDevisor = 1 << m_key.m_zoomLevel;
  double const rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDevisor;
  double const rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / worldSizeDevisor;

  m2::RectD tileRect(m_key.m_x * rectSizeX,
                     m_key.m_y * rectSizeY,
                     (m_key.m_x + 1) * rectSizeX,
                     (m_key.m_y + 1) * rectSizeY);

  return tileRect;
}

  void TileInfo::ReadFeatureIndex(model::FeaturesFetcher const & model)
{
    if (DoNeedReadIndex())
{
  threads::MutexGuard guard(m_mutex);
      CheckCanceled();
      model.ForEachFeatureID(GetGlobalRect(), *this, m_key.m_zoomLevel);
      sort(m_featureInfo.begin(), m_featureInfo.end());
    }
}

  namespace
  {
    struct IDsAccumulator
    {
      IDsAccumulator(vector<FeatureID> & ids, vector<FeatureInfo> const & src)
        : m_ids(ids)
        , m_src(src)
{
}

      void operator()(size_t index)
{
        ASSERT_LESS(index, m_src.size(), ());
        m_ids.push_back(m_src[index].m_id);
}

      vector<FeatureID> & m_ids;
      vector<FeatureInfo> const & m_src;
    };
}

  void TileInfo::ReadFeatures(model::FeaturesFetcher const & model, 
                              MemoryFeatureIndex & memIndex,
                              EngineContext & context)
{
  CheckCanceled();
  vector<size_t> indexes;
    RequestFeatures(memIndex, indexes);

    vector<FeatureID> featuresToRead;
    for_each(indexes.begin(), indexes.end(), IDsAccumulator(featuresToRead, m_featureInfo));

    model.ReadFeatures(*this, featuresToRead);
  }

  void TileInfo::Cancel(MemoryFeatureIndex & memIndex)
  {
    m_isCanceled = true;
    threads::MutexGuard guard(m_mutex);
    memIndex.RemoveFeatures(m_featureInfo);
  }

  bool TileInfo::operator ()(FeatureType const & f)
  {
    return true;
}

void TileInfo::operator ()(FeatureID const & id)
{
  m_featureInfo.push_back(id);
  CheckCanceled();
}

  //====================================================//

  bool TileInfo::DoNeedReadIndex() const
  {
    return m_featureInfo.empty();
  }

  void TileInfo::RequestFeatures(MemoryFeatureIndex & memIndex, vector<size_t> & featureIndexes)
  {
    threads::MutexGuard guard(m_mutex);
    memIndex.ReadFeaturesRequest(m_featureInfo, featureIndexes);
  }

  void TileInfo::CheckCanceled()
  {
    if (m_isCanceled)
      MYTHROW(ReadCanceledException, ());
  }
}
