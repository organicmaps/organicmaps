#pragma once

#include "message_acceptor.hpp"
#include "../std/atomic.hpp"
#include "../std/condition_variable.hpp"
#include "../std/function.hpp"
#include "../std/mutex.hpp"

namespace df
{

class BaseRenderer : public MessageAcceptor
{
public:
  using TCompletionHandler = function<void()>;

  BaseRenderer();
  void SetRenderingEnabled(bool const isEnabled);

protected:
  void CheckRenderingEnabled();

private:
  mutex m_renderingEnablingMutex;
  condition_variable m_renderingEnablingCondition;
  atomic<bool> m_isEnabled;
  TCompletionHandler m_renderingEnablingCompletionHandler;
  bool m_wasNotified;

  void SetRenderingEnabled(bool const isEnabled, TCompletionHandler completionHandler);
  void Notify();
};

} // namespace df
