#pragma once

#include "drape_frontend/arrow3d.hpp"
#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/drape_engine_params.hpp"
#include "drape_frontend/gui/layer_render.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/overlay_batcher.hpp"
#include "drape_frontend/requested_tiles.hpp"
#include "drape_frontend/traffic_generator.hpp"
#include "drape_frontend/transit_scheme_builder.hpp"
#include "drape_frontend/user_mark_generator.hpp"

#include "drape/pointers.hpp"
#include "drape/viewport.hpp"

#include <functional>
#include <memory>

namespace dp
{
class GraphicsContextFactory;
class TextureManager;
}  // namespace dp

namespace df
{
class Message;
class ReadManager;
class RouteBuilder;
class MetalineManager;

class BackendRenderer : public BaseRenderer
{
public:
  using TUpdateCurrentCountryFn = std::function<void(m2::PointD const &, int)>;

  struct Params : BaseRenderer::Params
  {
    Params(dp::ApiVersion apiVersion, ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::GraphicsContextFactory> factory, ref_ptr<dp::TextureManager> texMng,
           MapDataProvider const & model, TUpdateCurrentCountryFn const & updateCurrentCountryFn,
           ref_ptr<RequestedTiles> requestedTiles, bool allow3dBuildings, bool trafficEnabled, bool isolinesEnabled,
           bool simplifiedTrafficColors, std::optional<Arrow3dCustomDecl> arrow3dCustomDecl,
           OnGraphicsContextInitialized const & onGraphicsContextInitialized)
      : BaseRenderer::Params(apiVersion, commutator, factory, texMng, onGraphicsContextInitialized)
      , m_model(model)
      , m_updateCurrentCountryFn(updateCurrentCountryFn)
      , m_requestedTiles(requestedTiles)
      , m_allow3dBuildings(allow3dBuildings)
      , m_trafficEnabled(trafficEnabled)
      , m_isolinesEnabled(isolinesEnabled)
      , m_simplifiedTrafficColors(simplifiedTrafficColors)
      , m_arrow3dCustomDecl(std::move(arrow3dCustomDecl))
    {}

    MapDataProvider const & m_model;
    TUpdateCurrentCountryFn m_updateCurrentCountryFn;
    ref_ptr<RequestedTiles> m_requestedTiles;
    bool m_allow3dBuildings;
    bool m_trafficEnabled;
    bool m_isolinesEnabled;
    bool m_simplifiedTrafficColors;
    std::optional<Arrow3dCustomDecl> m_arrow3dCustomDecl;
  };

  explicit BackendRenderer(Params && params);
  ~BackendRenderer() override;

  void Teardown();

protected:
  std::unique_ptr<threads::IRoutine> CreateRoutine() override;

  void RenderFrame() override;

  void OnContextCreate() override;
  void OnContextDestroy() override;

private:
  void RecacheGui(gui::TWidgetsInitInfo const & initInfo, bool needResetOldGui);
  void RecacheChoosePositionMark();
  void RecacheMapShapes();
  void CleanupTextures();

#ifdef RENDER_DEBUG_INFO_LABELS
  void RecacheDebugLabels();
#endif

  void AcceptMessage(ref_ptr<Message> message) override;

  class Routine : public threads::IRoutine
  {
  public:
    explicit Routine(BackendRenderer & renderer);

    void Do() override;

  private:
    BackendRenderer & m_renderer;
  };

  void ReleaseResources();

  void InitContextDependentResources();
  void FlushGeometry(TileKey const & key, dp::RenderState const & state, drape_ptr<dp::RenderBucket> && buffer);

  void FlushTransitRenderData(TransitRenderData && renderData);
  void FlushTrafficRenderData(TrafficRenderData && renderData);
  void FlushUserMarksRenderData(TUserMarksRenderData && renderData);

  void CleanupOverlays(TileKey const & tileKey);

  MapDataProvider m_model;
  drape_ptr<BatchersPool<TileKey, TileKeyStrictComparator>> m_batchersPool;
  drape_ptr<ReadManager> m_readManager;
  drape_ptr<RouteBuilder> m_routeBuilder;
  drape_ptr<TransitSchemeBuilder> m_transitBuilder;
  drape_ptr<TrafficGenerator> m_trafficGenerator;
  drape_ptr<UserMarkGenerator> m_userMarkGenerator;
  drape_ptr<DrapeApiBuilder> m_drapeApiBuilder;
  gui::LayerCacher m_guiCacher;

  ref_ptr<RequestedTiles> m_requestedTiles;

  TOverlaysRenderData m_overlays;

  TUpdateCurrentCountryFn m_updateCurrentCountryFn;

  drape_ptr<MetalineManager> m_metalineManager;

  gui::TWidgetsInitInfo m_lastWidgetsInfo;

  std::optional<Arrow3dCustomDecl> m_arrow3dCustomDecl;
  Arrow3d::PreloadedData m_arrow3dPreloadedData;

#ifdef DEBUG
  bool m_isTeardowned;
#endif
};
}  // namespace df
