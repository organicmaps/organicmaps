#pragma once

#include "drape_frontend/gui/layer_render.hpp"

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/requested_tiles.hpp"
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
  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator, ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng, MapDataProvider const & model,
           ref_ptr<RequestedTiles> requestedTiles, bool allow3dBuildings)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_model(model)
      , m_requestedTiles(requestedTiles)
      , m_allow3dBuildings(allow3dBuildings)
    {}

    MapDataProvider const & m_model;
    ref_ptr<RequestedTiles> m_requestedTiles;
    bool m_allow3dBuildings;
  };

  BackendRenderer(Params const & params);
  ~BackendRenderer() override;

  void Teardown();

protected:
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void RecacheGui(gui::TWidgetsInitInfo const  & initInfo, gui::TWidgetsSizeInfo & sizeInfo);
  void RecacheCountryStatus();
  void RecacheMyPosition();

  void AcceptMessage(ref_ptr<Message> message) override;

  class Routine : public threads::IRoutine
  {
  public:
    Routine(BackendRenderer & renderer);

    // threads::IRoutine overrides:
    void Do() override;

  private:
    BackendRenderer & m_renderer;
  };

  void ReleaseResources();

  void InitGLDependentResource();
  void FlushGeometry(drape_ptr<Message> && message);

  MapDataProvider m_model;
  drape_ptr<BatchersPool> m_batchersPool;
  drape_ptr<ReadManager> m_readManager;
  drape_ptr<RouteBuilder> m_routeBuilder;
  gui::LayerCacher m_guiCacher;

  ref_ptr<RequestedTiles> m_requestedTiles;

#ifdef DEBUG
  bool m_isTeardowned;
#endif
};

} // namespace df
