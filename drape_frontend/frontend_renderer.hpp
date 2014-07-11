#pragma once

#include "../base/thread.hpp"

#ifdef DRAW_INFO
  #include "../base/timer.hpp"
  #include "../std/vector.hpp"
  #include "../std/numeric.hpp"
#endif

#include "message_acceptor.hpp"
#include "threads_commutator.hpp"
#include "tile_info.hpp"
#include "backend_renderer.hpp"
#include "render_group.hpp"

#include "../drape/pointers.hpp"
#include "../drape/glstate.hpp"
#include "../drape/vertex_array_buffer.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/oglcontextfactory.hpp"
#include "../drape/texture_set_controller.hpp"
#include "../drape/overlay_tree.hpp"

#include "../drape/uniform_values_storage.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/map.hpp"

class RenderBucket;

namespace df
{

class FrontendRenderer : public MessageAcceptor,
                         public threads::IRoutine
{
public:
  FrontendRenderer(RefPointer<ThreadsCommutator> commutator,
                   RefPointer<OGLContextFactory> oglcontextfactory,
                   TransferPointer<TextureSetController> textureController,
                   Viewport viewport);

  ~FrontendRenderer();

#ifdef DRAW_INFO
  double m_tpf;
  double m_fps;

  my::Timer m_timer;
  double m_frameStartTime;
  vector<double> m_tpfs;
  int m_drawedFrames;

  void BeforeDrawFrame();
  void AfterDrawFrame();
#endif

protected:
  virtual void AcceptMessage(RefPointer<Message> message);

private:
  void RenderScene();
  void RefreshProjection();
  void RefreshModelView();

  void ResolveTileKeys();
  void ResolveTileKeys(set<TileKey> & keyStorage, m2::RectD const & rect);
  void ResolveTileKeys(set<TileKey> & keyStorage, int tileScale);
  set<TileKey> & GetTileKeyStorage();

  void InvalidateRenderGroups(set<TileKey> & keyStorage);

private:
  void StartThread();
  void StopThread();
  void ThreadMain();
  void ReleaseResources();

  virtual void Do();

private:
  void DeleteRenderData();

private:
  RefPointer<ThreadsCommutator> m_commutator;
  RefPointer<OGLContextFactory> m_contextFactory;
  MasterPointer<TextureSetController> m_textureController;
  MasterPointer<GpuProgramManager> m_gpuProgramManager;
  threads::Thread m_selfThread;

private:
  vector<RenderGroup *> m_renderGroups;

  UniformValuesStorage m_generalUniforms;

  Viewport m_viewport;
  ScreenBase m_view;
  set<TileKey> m_tiles;

  OverlayTree m_overlayTree;
};

} // namespace df
