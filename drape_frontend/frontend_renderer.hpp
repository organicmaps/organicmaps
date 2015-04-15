#pragma once

#include "base/thread.hpp"

#ifdef DRAW_INFO
  #include "base/timer.hpp"
  #include "std/vector.hpp"
  #include "std/numeric.hpp"
#endif

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/tile_tree.hpp"
#include "drape_frontend/backend_renderer.hpp"
#include "drape_frontend/render_group.hpp"
#include "drape_frontend/my_position.hpp"

#include "drape_gui/layer_render.hpp"

#include "drape/pointers.hpp"
#include "drape/glstate.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/uniform_values_storage.hpp"

#include "geometry/screenbase.hpp"

#include "std/map.hpp"

namespace dp { class RenderBucket; }

namespace df
{

class FrontendRenderer : public BaseRenderer
{
public:
  FrontendRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                   dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                   dp::RefPointer<dp::TextureManager> textureManager,
                   Viewport viewport);

  ~FrontendRenderer() override;

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

  void SetModelView(ScreenBase const & screen);

protected:
  virtual void AcceptMessage(dp::RefPointer<Message> message);
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void RenderScene();
  void RefreshProjection();
  void RefreshModelView();

  void ResolveTileKeys();
  void ResolveTileKeys(int tileScale);
  int GetCurrentZoomLevel() const;

private:
  class Routine : public threads::IRoutine
  {
   public:
    Routine(FrontendRenderer & renderer);

    // threads::IRoutine overrides:
    void Do() override;

   private:
    FrontendRenderer & m_renderer;
  };

  void ReleaseResources();

private:
  void DeleteRenderData();

  // it applies new model-view matrix to the scene (this matrix will be used on next frame)
  void UpdateScene();

  void AddToRenderGroup(vector<unique_ptr<RenderGroup>> & groups,
                        dp::GLState const & state,
                        dp::MasterPointer<dp::RenderBucket> & renderBucket,
                        TileKey const & newTile);
  void OnAddRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                        dp::MasterPointer<dp::RenderBucket> & renderBucket);
  void OnDeferRenderGroup(TileKey const & tileKey, dp::GLState const & state,
                          dp::MasterPointer<dp::RenderBucket> & renderBucket);

  void OnActivateTile(TileKey const & tileKey);
  void OnRemoveTile(TileKey const & tileKey);

private:
  dp::RefPointer<dp::TextureManager> m_textureManager;
  dp::MasterPointer<dp::GpuProgramManager> m_gpuProgramManager;

private:
  vector<unique_ptr<RenderGroup>> m_renderGroups;
  vector<unique_ptr<RenderGroup>> m_deferredRenderGroups;
  vector<unique_ptr<UserMarkRenderGroup>> m_userMarkRenderGroups;
  set<TileKey> m_userMarkVisibility;

  dp::MasterPointer<gui::LayerRenderer> m_guiRenderer;
  dp::MasterPointer<MyPosition> m_myPositionMark;

  dp::UniformValuesStorage m_generalUniforms;

  Viewport m_viewport;
  ScreenBase m_view;

  unique_ptr<TileTree> m_tileTree;

  ScreenBase m_newView;
  mutex m_modelViewMutex;

  dp::OverlayTree m_overlayTree;
};

} // namespace df
