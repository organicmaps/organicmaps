#pragma once

#include "drape_frontend/message_acceptor.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/oglcontextfactory.hpp"
#include "drape/texture_manager.hpp"

#include "base/thread.hpp"

#include "std/atomic.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"

namespace df
{

class BaseRenderer : public MessageAcceptor
{
public:
  struct Params
  {
    Params(ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::OGLContextFactory> factory,
           ref_ptr<dp::TextureManager> texMng)
      : m_commutator(commutator)
      , m_oglContextFactory(factory)
      , m_texMng(texMng)
    {
    }

    ref_ptr<ThreadsCommutator> m_commutator;
    ref_ptr<dp::OGLContextFactory> m_oglContextFactory;
    ref_ptr<dp::TextureManager> m_texMng;
  };

  BaseRenderer(ThreadsCommutator::ThreadName name, Params const & params);

  void SetRenderingEnabled(bool const isEnabled);

protected:
  ref_ptr<ThreadsCommutator> m_commutator;
  ref_ptr<dp::OGLContextFactory> m_contextFactory;
  ref_ptr<dp::TextureManager> m_texMng;

  void StartThread();
  void StopThread();

  void CheckRenderingEnabled();
  void ProcessStopRenderingMessage();

  virtual unique_ptr<threads::IRoutine> CreateRoutine() = 0;

private:
  bool CanReceiveMessage() override;

  using TCompletionHandler = function<void()>;

private:
  threads::Thread m_selfThread;
  ThreadsCommutator::ThreadName m_threadName;

  mutex m_renderingEnablingMutex;
  condition_variable m_renderingEnablingCondition;
  atomic<bool> m_isEnabled;
  TCompletionHandler m_renderingEnablingCompletionHandler;
  bool m_wasNotified;

  void SetRenderingEnabled(bool const isEnabled, TCompletionHandler completionHandler);
  void Notify();
  void WakeUp();
};

} // namespace df
