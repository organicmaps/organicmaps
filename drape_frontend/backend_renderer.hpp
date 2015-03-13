#pragma once

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/viewport.hpp"
#include "drape_frontend/map_data_provider.hpp"

#include "drape_gui/layer_render.hpp"
#include "drape/pointers.hpp"

#include "base/thread.hpp"

namespace dp
{

class OGLContextFactory;
class TextureManager;

}

namespace df
{

class Message;
class ThreadsCommutator;
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

private:
  MapDataProvider m_model;
  EngineContext m_engineContext;
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

  void StartThread();
  void StopThread();
  void ReleaseResources();

  void InitGLDependentResource();
  void FlushGeometry(dp::TransferPointer<Message> message);

private:
  threads::Thread m_selfThread;
  dp::RefPointer<ThreadsCommutator> m_commutator;
  dp::RefPointer<dp::OGLContextFactory> m_contextFactory;
};

} // namespace df
