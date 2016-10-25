#pragma once

#include "drape_frontend/gui/layer_render.hpp"

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/overlay_batcher.hpp"
#include "drape_frontend/requested_tiles.hpp"
#include "drape_frontend/traffic_generator.hpp"
#include "drape_frontend/viewport.hpp"

#include "drape/pointers.hpp"

namespace dp
{

class OGLContextFactory;
class TextureManager;

}

namespace df
{

class Message;
class BatchersPool;
class ReadManager;
class RouteBuilder;

class BackendRenderer : public BaseRenderer
{
public:
  using TUpdateCurrentCountryFn = function<void (m2::PointD const &, int)>;

  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator, ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng, MapDataProvider const & model,
           TUpdateCurrentCountryFn const & updateCurrentCountryFn,
           ref_ptr<RequestedTiles> requestedTiles, bool allow3dBuildings)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_model(model)
      , m_updateCurrentCountryFn(updateCurrentCountryFn)
      , m_requestedTiles(requestedTiles)
      , m_allow3dBuildings(allow3dBuildings)
    {}

    MapDataProvider const & m_model;
    TUpdateCurrentCountryFn m_updateCurrentCountryFn;
    ref_ptr<RequestedTiles> m_requestedTiles;
    bool m_allow3dBuildings;
  };

  BackendRenderer(Params const & params);
  ~BackendRenderer() override;

  void Teardown();

protected:
  unique_ptr<threads::IRoutine> CreateRoutine() override;

  void OnContextCreate() override;
  void OnContextDestroy() override;

private:
  void RecacheGui(gui::TWidgetsInitInfo const & initInfo, bool needResetOldGui);
  void RecacheChoosePositionMark();
  void RecacheMapShapes();

#ifdef RENRER_DEBUG_INFO_LABELS
  void RecacheDebugLabels();
#endif

  void AcceptMessage(ref_ptr<Message> message) override;

  class Routine : public threads::IRoutine
  {
  public:
    Routine(BackendRenderer & renderer);

    void Do() override;

  private:
    BackendRenderer & m_renderer;
  };

  void ReleaseResources();

  void InitGLDependentResource();
  void FlushGeometry(drape_ptr<Message> && message);

  void CleanupOverlays(TileKey const & tileKey);

  MapDataProvider m_model;
  drape_ptr<BatchersPool> m_batchersPool;
  drape_ptr<ReadManager> m_readManager;
  drape_ptr<RouteBuilder> m_routeBuilder;
  drape_ptr<TrafficGenerator> m_trafficGenerator;
  drape_ptr<DrapeApiBuilder> m_drapeApiBuilder;
  gui::LayerCacher m_guiCacher;

  ref_ptr<RequestedTiles> m_requestedTiles;

  TOverlaysRenderData m_overlays;

  TUpdateCurrentCountryFn m_updateCurrentCountryFn;

#ifdef DEBUG
  bool m_isTeardowned;
#endif
};

} // namespace df
