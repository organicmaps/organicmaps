#include "animator.hpp"
#include "rotate_screen_task.hpp"
#include "framework.hpp"

#include "../anim/controller.hpp"

#include "../geometry/angles.hpp"

Animator::Animator(Framework * framework)
  : m_framework(framework)
{}

void Animator::RotateScreen(double startAngle, double endAngle, double duration)
{
  if (m_rotateScreenTask)
    m_rotateScreenTask->Lock();

  if (m_rotateScreenTask
  && !m_rotateScreenTask->IsCancelled()
  && !m_rotateScreenTask->IsEnded())
    m_rotateScreenTask->SetEndAngle(endAngle);
  else
  {
    if (floor(ang::RadToDegree(fabs(ang::GetShortestDistance(startAngle, endAngle)))) > 0)
    {
      if (m_rotateScreenTask)
      {
        m_rotateScreenTask->Unlock();
        m_rotateScreenTask->Cancel();
        m_rotateScreenTask.reset();
      }

      m_rotateScreenTask.reset(new RotateScreenTask(m_framework,
                                                    startAngle,
                                                    endAngle,
                                                    duration));

      m_framework->GetAnimController()->AddTask(m_rotateScreenTask);
      return;
    }
  }

  if (m_rotateScreenTask)
    m_rotateScreenTask->Unlock();
}

void Animator::StopRotation()
{
  if (m_rotateScreenTask
   && !m_rotateScreenTask->IsEnded()
   && !m_rotateScreenTask->IsCancelled())
    m_rotateScreenTask->Cancel();

  m_rotateScreenTask.reset();
}
