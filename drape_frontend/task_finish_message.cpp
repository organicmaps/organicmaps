#include "task_finish_message.hpp"

namespace df
{
  TaskFinishMessage::TaskFinishMessage(threads::IRoutine * routine)
    : m_routine(routine)
  {
    SetType(TaskFinish);
  }

  threads::IRoutine * TaskFinishMessage::GetRoutine() const
  {
    return m_routine;
  }
}
