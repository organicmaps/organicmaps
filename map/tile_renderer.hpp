#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile_cache.hpp"
#include "drawer_yg.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/commands_queue.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/vector.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class RenderState;
    class RenderContext;
    class PacketsQueue;
  }
}

class WindowHandle;
class DrawerYG;

class TileRenderer
{
protected:

  core::CommandsQueue m_queue;

  shared_ptr<yg::ResourceManager> m_resourceManager;

  struct ThreadData
  {
    DrawerYG * m_drawer;
    DrawerYG::params_t m_drawerParams;
    shared_ptr<yg::gl::BaseTexture> m_dummyRT;
    shared_ptr<yg::gl::RenderContext> m_renderContext;
    shared_ptr<yg::gl::RenderBuffer> m_depthBuffer;
  };

  buffer_vector<ThreadData, 4> m_threadData;

  shared_ptr<yg::gl::RenderContext> m_primaryContext;

  TileCache m_tileCache;

  RenderPolicy::TRenderFn m_renderFn;
  string m_skinName;
  yg::Color m_bgColor;
  int m_sequenceID;
  bool m_isExiting;

  threads::Mutex m_tilesInProgressMutex;
  set<Tiler::RectInfo> m_tilesInProgress;

  void InitializeThreadGL(core::CommandsQueue::Environment const & env);
  void FinalizeThreadGL(core::CommandsQueue::Environment const & env);

protected:

  virtual void DrawTile(core::CommandsQueue::Environment const & env,
                        Tiler::RectInfo const & rectInfo,
                        int sequenceID);

  void ReadPixels(yg::gl::PacketsQueue * glQueue, core::CommandsQueue::Environment const & env);

public:

  /// constructor.
  TileRenderer(string const & skinName,
               unsigned maxTilesCount,
               unsigned tasksCount,
               yg::Color const & bgColor,
               RenderPolicy::TRenderFn const & renderFn,
               shared_ptr<yg::gl::RenderContext> const & primaryRC,
               shared_ptr<yg::ResourceManager> const & rm,
               double visualScale,
               yg::gl::PacketsQueue ** packetsQueue);
  /// destructor.
  virtual ~TileRenderer();
  /// add command to the commands queue.
  void AddCommand(Tiler::RectInfo const & rectInfo,
                  int sequenceID,
                  core::CommandsQueue::Chain const & afterTileFns = core::CommandsQueue::Chain());
  /// get tile cache.
  TileCache & GetTileCache();
  /// wait on a condition variable for an empty queue.
  void WaitForEmptyAndFinished();

  void SetSequenceID(int sequenceID);

  void CancelCommands();

  void ClearCommands();

  bool HasTile(Tiler::RectInfo const & rectInfo);
  void AddTile(Tiler::RectInfo const & rectInfo, Tile const & tile);

  void StartTile(Tiler::RectInfo const & rectInfo);
  void FinishTile(Tiler::RectInfo const & rectInfo);

  /// checking, whether we should cancell currently rendering tiles,
  /// or continue with theirs rendering.
  void CheckCurrentTiles(vector<Tiler::RectInfo> & v);
};
