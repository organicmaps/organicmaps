#pragma once

#include "message_acceptor.hpp"
#include "engine_context.hpp"

#include "../drape/pointers.hpp"
#include "../drape/oglcontextfactory.hpp"

#include "../map/feature_vec_model.hpp"

#include "../base/thread.hpp"

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
                    double visualScale,
                    int surfaceWidth,
                    int surfaceHeight);

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

    void PostToRenderThreads(TransferPointer<Message> message);

  private:
    threads::Thread m_selfThread;
    RefPointer<ThreadsCommutator> m_commutator;
    RefPointer<OGLContextFactory> m_contextFactory;
  };
}
