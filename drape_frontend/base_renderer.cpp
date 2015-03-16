#include "base_renderer.hpp"
#include "threads_commutator.hpp"
#include "../std/utility.hpp"

#include "../base/logging.hpp"


namespace df
{

BaseRenderer::BaseRenderer(dp::RefPointer<ThreadsCommutator> commutator,
                           dp::RefPointer<dp::OGLContextFactory> oglcontextfactory)
  : m_commutator(commutator)
  , m_contextFactory(oglcontextfactory)
  , m_isEnabled(true)
  , m_renderingEnablingCompletionHandler(nullptr)
  , m_wasNotified(false)
{
}

void BaseRenderer::StartThread()
{
  m_selfThread.Create(this);
}

void BaseRenderer::StopThread()
{
  IRoutine::Cancel();

  if (!m_isEnabled)
  {
    WakeUp();
  }
  CloseQueue();
  m_selfThread.Join();
}

void BaseRenderer::SetRenderingEnabled(bool const isEnabled)
{
  // here we have to wait for completion of internal SetRenderingEnabled
  mutex completionMutex;
  condition_variable completionCondition;
  bool notified = false;
  auto completionHandler = [&]()
  {
    lock_guard<mutex> lock(completionMutex);
    notified = true;
    completionCondition.notify_one();
  };

  SetRenderingEnabled(isEnabled, completionHandler);

  unique_lock<mutex> lock(completionMutex);
  completionCondition.wait(lock, [&notified] { return notified; });
}

void BaseRenderer::SetRenderingEnabled(bool const isEnabled, TCompletionHandler completionHandler)
{
  if (isEnabled == m_isEnabled)
  {
    if (completionHandler != nullptr)
      completionHandler();

    return;
  }

  m_renderingEnablingCompletionHandler = move(completionHandler);
  if (isEnabled)
  {
    // wake up rendering thread
    WakeUp();
  }
  else
  {
    // here we set up value only if rendering disabled
    m_isEnabled = false;

    // if renderer thread is waiting for message let it go
    CancelMessageWaiting();
  }
}

void BaseRenderer::CheckRenderingEnabled()
{
  if (!m_isEnabled)
  {
    // nofity initiator-thread about rendering disabling
    Notify();

    // wait for signal
    unique_lock<mutex> lock(m_renderingEnablingMutex);
    m_renderingEnablingCondition.wait(lock, [this] { return m_wasNotified; });

    // here rendering is enabled again
    m_wasNotified = false;
    m_isEnabled = true;

    // nofity initiator-thread about rendering enabling
    // m_renderingEnablingCompletionHandler will be setup before awakening of this thread
    Notify();
  }
}

void BaseRenderer::Notify()
{
  if (m_renderingEnablingCompletionHandler != nullptr)
    m_renderingEnablingCompletionHandler();

  m_renderingEnablingCompletionHandler = nullptr;
}

void BaseRenderer::WakeUp()
{
  lock_guard<mutex> lock(m_renderingEnablingMutex);
  m_wasNotified = true;
  m_renderingEnablingCondition.notify_one();
}

} // namespace df
