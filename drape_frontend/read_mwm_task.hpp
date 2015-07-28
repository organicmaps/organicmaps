#pragma once

#include "drape_frontend/tile_info.hpp"

#include "base/thread.hpp"

#ifdef DEBUG
#include "base/object_tracker.hpp"
#endif

#include "std/shared_ptr.hpp"

namespace dp
{
  class TextureManager;
}

namespace df
{

class ReadMWMTask : public threads::IRoutine
{
public:
  ReadMWMTask(MemoryFeatureIndex & memIndex,
              MapDataProvider & model);

  virtual void Do();

  void Init(shared_ptr<TileInfo> const & tileInfo, ref_ptr<dp::TextureManager> texMng);
  void Reset();
  TileKey GetTileKey() const;

private:
  shared_ptr<TileInfo> m_tileInfo;
  ref_ptr<dp::TextureManager> m_texMng;

  MemoryFeatureIndex & m_memIndex;
  MapDataProvider & m_model;

#ifdef DEBUG
  dbg::ObjectTracker m_objTracker;
  bool m_checker;
#endif
};

class ReadMWMTaskFactory
{
public:
  ReadMWMTaskFactory(MemoryFeatureIndex & memIndex,
                     MapDataProvider & model)
    : m_memIndex(memIndex)
    , m_model(model) {}

  /// Caller must handle object life cycle
  ReadMWMTask * GetNew() const
  {
    return new ReadMWMTask(m_memIndex, m_model);
  }

private:
  MemoryFeatureIndex & m_memIndex;
  MapDataProvider & m_model;
};

} // namespace df
