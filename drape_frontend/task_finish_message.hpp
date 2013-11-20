#pragma once

#include "message.hpp"

namespace threads { class IRoutine; }

namespace df
{
  class TaskFinishMessage : public Message
  {
  public:
    TaskFinishMessage(threads::IRoutine * routine);

    threads::IRoutine * GetRoutine() const;

  private:
    threads::IRoutine * m_routine;
  };
}
