#pragma once

#include "message_acceptor.hpp"
#include "engine_context.hpp"
#include "viewport.hpp"

#include "../drape/pointers.hpp"

#include "../map/feature_vec_model.hpp"

#include "../base/thread.hpp"

namespace dp
{

class OGLContextFactory;
class TextureSetHolder;

}

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
  BackendRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                  dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                  dp::RefPointer<dp::TextureSetHolder> textureHolder);

  ~BackendRenderer();

private:
  model::FeaturesFetcher m_model;
  EngineContext m_engineContext;
  dp::MasterPointer<BatchersPool> m_batchersPool;
  dp::MasterPointer<ReadManager>  m_readManager;

  /////////////////////////////////////////
  //           MessageAcceptor           //
  /////////////////////////////////////////
private:
  void AcceptMessage(dp::RefPointer<Message> message);

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
  void FlushGeometry(dp::TransferPointer<Message> message);

private:
  threads::Thread m_selfThread;
  dp::RefPointer<ThreadsCommutator> m_commutator;
  dp::RefPointer<dp::OGLContextFactory> m_contextFactory;
  dp::RefPointer<dp::TextureSetHolder> m_textures;
};

} // namespace df
