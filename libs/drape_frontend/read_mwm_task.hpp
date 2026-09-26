#pragma once

#include "drape_frontend/tile_info.hpp"

#include "base/thread.hpp"
#include "drape/render_context.hpp"

#include <memory>

namespace df
{
class ReadMWMTask : public threads::IRoutine
{
public:
  explicit ReadMWMTask(MapDataProvider & model);

  void Do() override;

  void Init(std::shared_ptr<TileInfo> const & tileInfo);
  void Reset() override;
  bool IsCancelled() const override;
  TileKey const & GetTileKey() const { return m_tileKey; }

private:
  std::weak_ptr<TileInfo> m_tileInfo;
  TileKey m_tileKey;
  MapDataProvider & m_model;
  std::shared_ptr<dp::RenderContext> m_renderContext = dp::RenderContext::Current();

#ifdef DEBUG
  bool m_checker;
#endif
};

class ReadMWMTaskFactory
{
public:
  explicit ReadMWMTaskFactory(MapDataProvider & model) : m_model(model) {}

  // Caller must handle object life cycle.
  ReadMWMTask * GetNew() const { return new ReadMWMTask(m_model); }

private:
  MapDataProvider & m_model;
};
}  // namespace df
