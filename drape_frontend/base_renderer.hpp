#pragma once

#include "message_acceptor.hpp"
#include "../std/condition_variable.hpp"
#include "../std/mutex.hpp"
#include "../std/atomic.hpp"
#include "../std/function.hpp"

namespace df
{

class BaseRenderer : public MessageAcceptor
{
public:
  using completion_handler_t = function<void()>;

  BaseRenderer();

  void SetRenderingEnabled(bool const isEnabled, completion_handler_t completionHandler);

protected:
  void CheckRenderingEnabled();

private:
  mutex m_renderingEnablingMutex;
  condition_variable m_renderingEnablingCondition;
  atomic<bool> m_isEnabled;
  completion_handler_t m_renderingEnablingCompletionHandler;
  bool m_wasNotified;
};

} // namespace df
