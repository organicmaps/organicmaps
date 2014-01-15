#include "read_mwm_task.hpp"

#include "area_shape.hpp"

#include "../base/logging.hpp"

#include "../std/vector.hpp"

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

  struct FeatureIndexFetcher
  {
  public:
    FeatureIndexFetcher(df::TileInfo & info)
      : m_info(info)
    {
    }

    void Finish()
    {
      sort(m_info.m_featureInfo.begin(), m_info.m_featureInfo.end());
    }

    void operator()(FeatureID featureID) const
    {
      m_info.m_featureInfo.push_back(featureID);
    }

  private:
    df::TileInfo & m_info;
  };
}

namespace df
{
  ReadMWMTask::ReadMWMTask(TileKey const & tileKey,
                           model::FeaturesFetcher const & model,
                           MemoryFeatureIndex & index,
                           EngineContext & context)
    : m_tileInfo(tileKey)
    , m_model(model)
    , m_index(index)
    , m_context(context)
    , m_isFinished(false)
  {
  }

  ReadMWMTask::~ReadMWMTask()
  {
    m_index.RemoveFeatures(m_tileInfo.m_featureInfo);
  }

  void ReadMWMTask::Do()
  {
    if (m_tileInfo.m_featureInfo.empty())
      ReadTileIndex();

    if (IsCancelled())
      return;

    vector<size_t> indexesToRead;
    m_index.ReadFeaturesRequest(m_tileInfo.m_featureInfo, indexesToRead);

    if (IsCancelled())
      return;

    if (!indexesToRead.empty())
    {
      m_context.BeginReadTile(m_tileInfo.m_key);

      for (size_t i = 0; i < indexesToRead.size(); ++i)
      {
        FeatureInfo & info = m_tileInfo.m_featureInfo[i];
        ReadGeometry(info.m_id);

        if (IsCancelled())
          break;
      }

      m_context.EndReadTile(m_tileInfo.m_key);
    }
  }

  TileInfo const & ReadMWMTask::GetTileInfo() const
  {
    return m_tileInfo;
  }

  void ReadMWMTask::PrepareToRestart()
  {
    m_isFinished = false;
  }

  void ReadMWMTask::Finish()
  {
    m_isFinished = true;
  }

  bool ReadMWMTask::IsFinished()
  {
    return m_isFinished;
  }

  void ReadMWMTask::ReadTileIndex()
  {
    FeatureIndexFetcher fetcher(m_tileInfo);
    m_model.ForEachFeatureID(m_tileInfo.GetGlobalRect(),
                             fetcher,
                             m_tileInfo.m_key.m_zoomLevel);
    fetcher.Finish();
  }

  void ReadMWMTask::ReadGeometry(const FeatureID & id)
  {
    if (id.m_mwm == 41)
      m_context.InsertShape(m_tileInfo.m_key, MovePointer<MapShape>(CreateFakeShape1()));
    else if (id.m_mwm == 42)
      m_context.InsertShape(m_tileInfo.m_key, MovePointer<MapShape>(CreateFakeShape2()));
    ///TODO read geometry
    ///TODO proccess geometry by styles
    ///foreach shape in shapes
    ///  m_context.InsertShape(shape);
  }
}
