#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "std/utility.hpp"

namespace df
{

BaseRenderer::BaseRenderer(ThreadsCommutator::ThreadName name, Params const & params)
  : m_commutator(params.m_commutator)
  , m_contextFactory(params.m_oglContextFactory)
  , m_texMng(params.m_texMng)
  , m_threadName(name)
  , m_isEnabled(true)
  , m_renderingEnablingCompletionHandler(nullptr)
  , m_wasNotified(false)
{
  m_commutator->RegisterThread(m_threadName, this);
}

void BaseRenderer::StartThread()
{
  m_selfThread.Create(CreateRoutine());
}

void BaseRenderer::StopThread()
{
  // send message to stop rendering in render thread
  m_commutator->PostMessage(m_threadName,
                            make_unique_dp<StopRenderingMessage>(),
                            MessagePriority::High);

  // wake up render thread if necessary
  if (!m_isEnabled)
  {
    WakeUp();
  }

  // wait for render thread completion
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

void BaseRenderer::ProcessStopRenderingMessage()
{
  m_selfThread.GetRoutine()->Cancel();
  CloseQueue();
}

bool BaseRenderer::CanReceiveMessage()
{
  return !m_selfThread.GetRoutine()->IsCancelled();
}

} // namespace df
