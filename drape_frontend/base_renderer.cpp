#include "base_renderer.hpp"
#include "../std/utility.hpp"

namespace df
{

BaseRenderer::BaseRenderer()
  : m_isEnabled(true)
  , m_renderingEnablingCompletionHandler(nullptr)
  , m_wasNotified(false)
{
}

void BaseRenderer::SetRenderingEnabled(bool const isEnabled, completion_handler_t completionHandler)
{
  if (isEnabled == m_isEnabled)
  {
    if (completionHandler != nullptr) completionHandler();
    return;
  }

  m_renderingEnablingCompletionHandler = move(completionHandler);
  if (isEnabled)
  {
    // wake up rendering thread
    unique_lock<mutex> lock(m_renderingEnablingMutex);
    m_wasNotified = true;
    m_renderingEnablingCondition.notify_one();
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
    if (m_renderingEnablingCompletionHandler != nullptr)
      m_renderingEnablingCompletionHandler();

    // wait for signal
    unique_lock<mutex> lock(m_renderingEnablingMutex);
    while(!m_wasNotified)
    {
      m_renderingEnablingCondition.wait(lock);
    }

    // here rendering is enabled again
    m_wasNotified = false;
    m_isEnabled = true;

    // nofity initiator-thread about rendering enabling
    if (m_renderingEnablingCompletionHandler != nullptr)
      m_renderingEnablingCompletionHandler();
  }
}

} // namespace df
