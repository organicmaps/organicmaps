#pragma once

#include "tile_info.hpp"

#include "../base/thread.hpp"

#ifdef DEBUG
#include "../base/object_tracker.hpp"
#endif

#include "../std/weak_ptr.hpp"

namespace df
{

class EngineContext;

class ReadMWMTask : public threads::IRoutine
{
public:
  ReadMWMTask(weak_ptr<TileInfo> const & tileInfo,
              MemoryFeatureIndex & memIndex,
              model::FeaturesFetcher & model,
              EngineContext & context);

  ReadMWMTask(MemoryFeatureIndex & memIndex,
              model::FeaturesFetcher & model,
              EngineContext & context);

  virtual void Do();

  void init(weak_ptr<TileInfo> const & tileInfo);
  void Reset();

private:
  weak_ptr<TileInfo> m_tileInfo;
  MemoryFeatureIndex & m_memIndex;
  model::FeaturesFetcher & m_model;
  EngineContext & m_context;

#ifdef DEBUG
  dbg::ObjectTracker m_objTracker;
#endif
};

class ReadMWMTaskFactory
{
public:
  ReadMWMTaskFactory(MemoryFeatureIndex & memIndex,
                     model::FeaturesFetcher & model,
                     EngineContext & context)
    : m_memIndex(memIndex)
    , m_model(model)
    , m_context(context) {}

  ReadMWMTask * GetNew() const
  {
    return new ReadMWMTask(m_memIndex, m_model, m_context);
  }

private:
  MemoryFeatureIndex & m_memIndex;
  model::FeaturesFetcher & m_model;
  EngineContext & m_context;
};

} // namespace df
