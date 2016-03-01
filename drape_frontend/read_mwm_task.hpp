#pragma once

#include "drape_frontend/tile_info.hpp"

#include "base/thread.hpp"

#ifdef DEBUG
#include "base/object_tracker.hpp"
#endif

#include "std/shared_ptr.hpp"
#include "std/weak_ptr.hpp"

namespace df
{

class ReadMWMTask : public threads::IRoutine
{
public:
  ReadMWMTask(MapDataProvider & model);

  void Do() override;

  void Init(shared_ptr<TileInfo> const & tileInfo);
  void Reset() override;
  bool IsCancelled() const override;
  TileKey const & GetTileKey() const { return m_tileKey; }

private:
  weak_ptr<TileInfo> m_tileInfo;
  TileKey m_tileKey;
  MapDataProvider & m_model;

#ifdef DEBUG
  dbg::ObjectTracker m_objTracker;
  bool m_checker;
#endif
};

class ReadMWMTaskFactory
{
public:
  ReadMWMTaskFactory(MapDataProvider & model)
    : m_model(model) {}

  /// Caller must handle object life cycle
  ReadMWMTask * GetNew() const
  {
    return new ReadMWMTask(m_model);
  }

private:
  MapDataProvider & m_model;
};

} // namespace df
