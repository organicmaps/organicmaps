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

void TileInfo::ReadFeatures(EngineContext & context)
{
  CheckCanceled();
  vector<size_t> indexes;
  RequestFeatures(indexes);

  for (size_t i = 0; i < indexes.size(); ++i)
  {
    CheckCanceled();
    LOG(LINFO, ("Trying to read", indexes[i]));
  }

  if (!indexes.empty() && m_key == TileKey(0,0,3))
  {
    context.BeginReadTile(m_key);
    {
      context.InsertShape(m_key, MovePointer<MapShape>(CreateFakeShape1()));
      context.InsertShape(m_key, MovePointer<MapShape>(CreateFakeShape2()));
      context.InsertShape(m_key, MovePointer<MapShape>(CreateFakeLine1()));
      context.InsertShape(m_key, MovePointer<MapShape>(CreateFakeLine2()));
    }
    context.EndReadTile(m_key);
  }
}

void TileInfo::operator ()(FeatureID const & id)
{
  m_featureInfo.push_back(id);
  CheckCanceled();
}

}
