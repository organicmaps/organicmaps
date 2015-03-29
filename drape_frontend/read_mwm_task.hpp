#pragma once

#include "drape_frontend/tile_info.hpp"

#include "base/thread.hpp"

#ifdef DEBUG
#include "base/object_tracker.hpp"
#endif

#include "std/weak_ptr.hpp"

namespace df
{

class EngineContext;

class ReadMWMTask : public threads::IRoutine
{
public:
  ReadMWMTask(MemoryFeatureIndex & memIndex,
              MapDataProvider & model);

  virtual void Do();

  void Init(weak_ptr<TileInfo> const & tileInfo);
  void Reset();

private:
  weak_ptr<TileInfo> m_tileInfo;
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

  ReadMWMTask * GetNew() const
  {
    return new ReadMWMTask(m_memIndex, m_model);
  }

private:
  MemoryFeatureIndex & m_memIndex;
  MapDataProvider & m_model;
};

} // namespace df
