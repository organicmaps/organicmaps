#pragma once

#include "message_acceptor.hpp"
#include "threads_commutator.hpp"
#include "tile_utils.hpp"

#include "../drape/oglcontextfactory.hpp"
#include "../base/thread.hpp"

#include "../std/atomic.hpp"
#include "../std/condition_variable.hpp"
#include "../std/function.hpp"
#include "../std/mutex.hpp"

namespace df
{

class BaseRenderer : public MessageAcceptor,
                     public threads::IRoutine
{
public:
  using TCompletionHandler = function<void()>;

  BaseRenderer(ThreadsCommutator::ThreadName name,
               dp::RefPointer<ThreadsCommutator> commutator,
               dp::RefPointer<dp::OGLContextFactory> oglcontextfactory);

  void SetRenderingEnabled(bool const isEnabled);

protected:
  dp::RefPointer<ThreadsCommutator> m_commutator;
  dp::RefPointer<dp::OGLContextFactory> m_contextFactory;

  void StartThread();
  void StopThread();

  void CheckRenderingEnabled();
  void ProcessStopRenderingMessage();

private:
  bool CanReceiveMessage() override;

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
