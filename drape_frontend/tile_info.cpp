#include "tile_info.hpp"
#include "engine_context.hpp"
#include "stylist.hpp"
#include "rule_drawer.hpp"

#include "line_shape.hpp"
#include "area_shape.hpp"
#include "engine_context.hpp"

#include "../map/feature_vec_model.hpp"

#include "../std/bind.hpp"

namespace
{
  df::AreaShape * CreateFakeShape1()
  {
    df::AreaShape * shape = new df::AreaShape(Extract(0xFFEEAABB), 0.3f);
    shape->AddTriangle(m2::PointF(50.0f, 50.0f),
                       m2::PointF(100.0f, 50.0f),
                       m2::PointF(50.0f, 100.0f));

    shape->AddTriangle(m2::PointF(100.0f, 50.0f),
                       m2::PointF(50.0f, 100.0f),
                       m2::PointF(100.0f, 100.0f));
    return shape;
  }

  df::AreaShape * CreateFakeShape2()
  {
    df::AreaShape * shape = new df::AreaShape(Extract(0xFF66AAFF), 0.0f);
    shape->AddTriangle(m2::PointF(0.0f, 50.0f),
                       m2::PointF(50.0f, 150.0f),
                       m2::PointF(50.0f, 0.0f));

    shape->AddTriangle(m2::PointF(50.0f, 0.0f),
                       m2::PointF(50.0f, 150.0f),
                       m2::PointF(150.0f, 50.0f));
    return shape;
  }

  df::LineShape * CreateFakeLine1()
  {
    vector<m2::PointF> points;
    const float magn = 40;

    for (float x = -4*math::pi; x <= 4*math::pi; x+= math::pi/32)
      points.push_back(m2::PointF(50 * x + 100.0, magn*sinf(x) + 200.0));

    df::LineViewParams params;
    params.m_color = Extract(0xFFFF0000);
    params.m_width = 4.0f;

    return new df::LineShape(points, 0.0f, params);
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

    for (size_t i = 0; i < points.size(); ++i)
    {
      m2::PointF p = points[i] * 100;
      points[i] = p + m2::PointF(100.0, 300.0);
    }

    df::LineViewParams params;
    params.m_color = Extract(0xFF00FF00);
    params.m_width = 2.0f;

    return new df::LineShape(points, 0.5f, params);
  }

  struct IDsAccumulator
  {
    IDsAccumulator(vector<FeatureID> & ids, vector<df::FeatureInfo> const & src)
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
    vector<df::FeatureInfo> const & m_src;
  };
}

namespace df
{
  TileInfo::TileInfo(TileKey const & key)
    : m_key(key)
  {}

  m2::RectD TileInfo::GetGlobalRect() const
  {
    return m_key.GetGlobalRect();
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

  void TileInfo::ReadFeatures(model::FeaturesFetcher const & model,
                              MemoryFeatureIndex & memIndex,
                              EngineContext & context)
  {
    CheckCanceled();
    vector<size_t> indexes;
    RequestFeatures(memIndex, indexes);

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
//    vector<FeatureID> featuresToRead;
//    for_each(indexes.begin(), indexes.end(), IDsAccumulator(featuresToRead, m_featureInfo));

//    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2), context);
//    model.ReadFeatures(drawer, featuresToRead);
  }

  void TileInfo::Cancel(MemoryFeatureIndex & memIndex)
  {
    m_isCanceled = true;
    threads::MutexGuard guard(m_mutex);
    memIndex.RemoveFeatures(m_featureInfo);
  }

  void TileInfo::operator ()(FeatureID const & id)
  {
    m_featureInfo.push_back(id);
    CheckCanceled();
  }

  void TileInfo::InitStylist(const FeatureType & f, Stylist & s)
  {
    CheckCanceled();
    df::InitStylist(f, m_key.m_zoomLevel, s);
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

  void TileInfo::CheckCanceled() const
  {
    if (m_isCanceled)
      MYTHROW(ReadCanceledException, ());
  }
}
