#include "read_mwm_task.hpp"

#include "../std/vector.hpp"

namespace df
{
  ReadMWMTask::ReadMWMTask(TileKey const & tileKey,
                           MemoryFeatureIndex & index,
                           EngineContext &context)
    : m_tileInfo(tileKey)
    , m_isFinished(false)
    , m_index(index)
    , m_context(context)
  {
  }

  void ReadMWMTask::Do()
  {
    if (m_tileInfo.m_featureInfo.empty())
      ReadTileIndex();

    vector<size_t> indexesToRead;
    m_index.ReadFeaturesRequest(m_tileInfo.m_featureInfo, indexesToRead);

    if (!indexesToRead.empty())
      m_context.BeginReadTile(m_tileInfo.m_key);

    for (size_t i = 0; i < indexesToRead.size(); ++i)
    {
      FeatureInfo & info = m_tileInfo.m_featureInfo[i];
      ReadGeometry(info.m_id);
      info.m_isOwner = true;
    }

    if (!indexesToRead.empty())
      m_context.EndReadTile(m_tileInfo.m_key);
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
    /// TODO read index specified by m_tileInfo(m_x & m_y & m_zoomLevel)
    /// TODO insert readed FeatureIDs into m_tileInfo.m_featureInfo;
  }

  void ReadMWMTask::ReadGeometry(const FeatureID & id)
  {
    ///TODO read geometry
    ///TODO proccess geometry by styles
    ///foreach shape in shapes
    ///  m_context.InsertShape(shape);
  }
}
