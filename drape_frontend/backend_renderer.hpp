#pragma once

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/viewport.hpp"

#include "drape_gui/layer_render.hpp"
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

class BackendRenderer : public BaseRenderer
{
public:

  struct Params : BaseRenderer::Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator, ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng, MapDataProvider const & model,
           gui::Handlers const & guiHandlers)
      : BaseRenderer::Params(commutator, factory, texMng)
      , m_model(model)
      , m_guiHandlers(guiHandlers)
    {}

    MapDataProvider const & m_model;
    gui::Handlers m_guiHandlers;
  };

  BackendRenderer(Params const & params);

  ~BackendRenderer() override;

protected:
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void RecacheGui(gui::Skin::ElementName elements);

private:
  MapDataProvider m_model;
  drape_ptr<BatchersPool> m_batchersPool;
  drape_ptr<ReadManager> m_readManager;
  gui::LayerCacher m_guiCacher;
  gui::Handlers m_guiHandlers;

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
private:
  void AcceptMessage(ref_ptr<Message> message);

  /////////////////////////////////////////
  //             ThreadPart              //
  /////////////////////////////////////////
private:
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
};

} // namespace df
