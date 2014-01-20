#include "tile_info.hpp"

#include "../map/feature_vec_model.hpp"
#include "../indexer/mercator.hpp"


//namespace
//{
//  df::AreaShape * CreateFakeShape1()
//  {
//    df::AreaShape * shape = new df::AreaShape(Extract(0xFFEEAABB), 0.3f);
//    shape->AddTriangle(m2::PointF(0.0f, 0.0f),
//                       m2::PointF(1.0f, 0.0f),
//                       m2::PointF(0.0f, 1.0f));

//    shape->AddTriangle(m2::PointF(1.0f, 0.0f),
//                       m2::PointF(0.0f, 1.0f),
//                       m2::PointF(1.0f, 1.0f));
//    return shape;
//  }

//  df::AreaShape * CreateFakeShape2()
//  {
//    df::AreaShape * shape = new df::AreaShape(Extract(0xFF66AAFF), 0.0f);
//    shape->AddTriangle(m2::PointF(-0.5f, 0.5f),
//                       m2::PointF(0.5f, 1.5f),
//                       m2::PointF(0.5f, -0.5f));

//    shape->AddTriangle(m2::PointF(0.5f, -0.5f),
//                       m2::PointF(0.5f, 1.5f),
//                       m2::PointF(1.5f, 0.5f));
//    return shape;
//  }
//}

namespace df
{

TileInfo::TileInfo(TileKey const & key, model::FeaturesFetcher & model, MemoryFeatureIndex & memIndex)
  : m_key(key)
  , m_model(model)
  , m_memIndex(memIndex)
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

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::Cancel()
{
  m_isCanceled = true;
  threads::MutexGuard guard(m_mutex);
  m_memIndex.RemoveFeatures(m_featureInfo);
}

void TileInfo::RequestFeatures(vector<size_t> & featureIndexes)
{
  threads::MutexGuard guard(m_mutex);
  m_memIndex.ReadFeaturesRequest(m_featureInfo, featureIndexes);
}

void TileInfo::CheckCanceled()
{
  if (m_isCanceled)
    MYTHROW(ReadCanceledException, ());
}

void TileInfo::ReadFeatureIndex()
{
  if (DoNeedReadIndex())
  {
    threads::MutexGuard guard(m_mutex);
    CheckCanceled();
    m_model.ForEachFeatureID(GetGlobalRect(), *this, m_key.m_zoomLevel);
  }
}

void TileInfo::ReadFeatures()
{
  CheckCanceled();
  vector<size_t> indexes;
  RequestFeatures(indexes);

  for (size_t i = 0; i < indexes.size(); ++i)
  {
    CheckCanceled();
    LOG(LINFO, ("Trying to read", indexes[i]));
  }
}

void TileInfo::operator ()(FeatureID const & id)
{
  m_featureInfo.push_back(id);
  CheckCanceled();
}

}
