#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile_cache.hpp"
#include "tile_set.hpp"
#include "gpu_drawer.hpp"

#include "geometry/screenbase.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"
#include "base/commands_queue.hpp"

#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

namespace graphics
{
  class ResourceManager;
  class PacketsQueue;

  namespace gl
  {
    class RenderContext;
  }
}

class WindowHandle;
class Drawer;

class TileRenderer
{
protected:

  core::CommandsQueue m_queue;

  shared_ptr<graphics::ResourceManager> m_resourceManager;

  struct ThreadData
  {
    GPUDrawer * m_drawer;
    GPUDrawer::Params m_drawerParams;
    unsigned m_threadSlot;
    shared_ptr<graphics::RenderContext> m_renderContext;
    shared_ptr<graphics::gl::RenderBuffer> m_colorBuffer;
    shared_ptr<graphics::gl::RenderBuffer> m_depthBuffer;
  };

  buffer_vector<ThreadData, 4> m_threadData;

  TileCache m_tileCache;

  /// set of already rendered tiles, which are waiting
  /// for the CoverageGenerator to process them
  TileSet m_tileSet;

  size_t m_tileSize;

  typedef pair<size_t, size_t> TileSizeT;
  TileSizeT GetTileSizes() const;

  RenderPolicy::TRenderFn m_renderFn;
  vector<graphics::Color> m_bgColors;
  int m_sequenceID;

  bool m_isPaused;

  threads::Mutex m_tilesInProgressMutex;
  set<Tiler::RectInfo> m_tilesInProgress;

  void InitializeThreadGL(core::CommandsQueue::Environment const & env);
  void FinalizeThreadGL(core::CommandsQueue::Environment const & env);

protected:

  virtual void DrawTile(core::CommandsQueue::Environment const & env,
                        Tiler::RectInfo const & rectInfo,
                        int sequenceID);

public:

  /// constructor.
  TileRenderer(size_t tileSize,
               unsigned tasksCount,
               vector<graphics::Color> const & bgColors,
               RenderPolicy::TRenderFn const & renderFn,
               shared_ptr<graphics::RenderContext> const & primaryRC,
               shared_ptr<graphics::ResourceManager> const & rm,
               double visualScale,
               graphics::PacketsQueue ** packetsQueue);
  /// destructor.
  virtual ~TileRenderer();
  void Shutdown();
  /// add command to the commands queue.
  void AddCommand(Tiler::RectInfo const & rectInfo,
                  int sequenceID,
                  core::CommandsQueue::Chain const & afterTileFns = core::CommandsQueue::Chain());
  /// get tile cache.
  TileCache & GetTileCache();
  /// Move active tile to cache if tile alrady rendered
  void CacheActiveTile(Tiler::RectInfo const & rectInfo);

  void SetSequenceID(int sequenceID);

  void CancelCommands();

  void ClearCommands();

  bool HasTile(Tiler::RectInfo const & rectInfo);

  /// add tile to the temporary set of rendered tiles, cache it and lock it in the cache.
  /// temporary set is necessary to carry the state between corresponding Tile rendering
  /// commands and MergeTile commands.
  void AddActiveTile(Tile const & tile);
  /// remove tile from the TileSet.
  /// @param doUpdateCache shows, whether we should
  void RemoveActiveTile(Tiler::RectInfo const & rectInfo, int sequenceID);

  void SetIsPaused(bool flag);

  size_t TileSize() const;
};
