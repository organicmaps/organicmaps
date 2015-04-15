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
  BackendRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                  dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                  dp::RefPointer<dp::TextureManager> textureManager,
                  MapDataProvider const & model);

  ~BackendRenderer() override;

protected:
  unique_ptr<threads::IRoutine> CreateRoutine() override;

private:
  void RecacheGui(gui::Skin::ElementName elements);

private:
  MapDataProvider m_model;
  dp::MasterPointer<BatchersPool> m_batchersPool;
  dp::MasterPointer<ReadManager>  m_readManager;
  dp::RefPointer<dp::TextureManager> m_texturesManager;
  gui::LayerCacher m_guiCacher;

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
private:
  void AcceptMessage(dp::RefPointer<Message> message);

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
  void FlushGeometry(dp::TransferPointer<Message> message);
};

} // namespace df
