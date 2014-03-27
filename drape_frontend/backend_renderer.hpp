#pragma once

#include "message_acceptor.hpp"
#include "engine_context.hpp"
#include "viewport.hpp"

#include "../drape/pointers.hpp"

#include "../map/feature_vec_model.hpp"

#include "../base/thread.hpp"

class OGLContextFactory;
class TextureSetHolder;

namespace df
{
  class Message;
  class ThreadsCommutator;
  class BatchersPool;
  class ReadManager;

  class BackendRenderer : public MessageAcceptor,
                          public threads::IRoutine
  {
  public:
    BackendRenderer(RefPointer<ThreadsCommutator> commutator,
                    RefPointer<OGLContextFactory> oglcontextfactory,
                    RefPointer<TextureSetHolder> textureHolder);

    ~BackendRenderer();

  private:
    void TileReadStarted(const TileKey & key);
    void TileReadEnded(const TileKey & key);
    void ShapeReaded(const TileKey & key, MapShape const * shape);

  private:
    model::FeaturesFetcher m_model;
    EngineContext m_engineContext;
    MasterPointer<BatchersPool> m_batchersPool;
    MasterPointer<ReadManager>  m_readManager;

    /////////////////////////////////////////
    //           MessageAcceptor           //
    /////////////////////////////////////////
  private:
    void AcceptMessage(RefPointer<Message> message);

      /////////////////////////////////////////
      //             ThreadPart              //
      /////////////////////////////////////////
  private:
    void StartThread();
    void StopThread();
    void ThreadMain();
    void ReleaseResources();
    virtual void Do();

    void InitGLDependentResource();
    void PostToRenderThreads(TransferPointer<Message> message);

  private:
    threads::Thread m_selfThread;
    RefPointer<ThreadsCommutator> m_commutator;
    RefPointer<OGLContextFactory> m_contextFactory;
    RefPointer<TextureSetHolder> m_textures;
  };
}
