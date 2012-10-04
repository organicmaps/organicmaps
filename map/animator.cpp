#include "animator.hpp"
#include "rotate_screen_task.hpp"
#include "change_viewport_task.hpp"
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
    if (fabs(ang::GetShortestDistance(startAngle, endAngle)) > math::pi / 180.0)
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
  if (m_rotateScreenTask)
    m_rotateScreenTask->Lock();

  if (m_rotateScreenTask
  && !m_rotateScreenTask->IsEnded()
  && !m_rotateScreenTask->IsCancelled())
  {
    m_rotateScreenTask->Unlock();
    m_rotateScreenTask->Cancel();
    m_rotateScreenTask.reset();
    return;
  }

  if (m_rotateScreenTask)
    m_rotateScreenTask->Unlock();

  m_rotateScreenTask.reset();
}

shared_ptr<ChangeViewportTask> const & Animator::ChangeViewport(m2::AnyRectD const & start,
                                                                m2::AnyRectD const & end,
                                                                double rotationSpeed)
{
  StopChangeViewport();

  m_changeViewportTask.reset(new ChangeViewportTask(start,
                                                    end,
                                                    rotationSpeed,
                                                    m_framework));

  m_framework->GetAnimController()->AddTask(m_changeViewportTask);

  return m_changeViewportTask;
}

void Animator::StopChangeViewport()
{
  if (m_changeViewportTask)
    m_changeViewportTask->Lock();

  if (m_changeViewportTask
  && !m_changeViewportTask->IsEnded()
  && !m_changeViewportTask->IsCancelled())
  {
    m_changeViewportTask->Unlock();
    m_changeViewportTask->Cancel();
    m_changeViewportTask.reset();
    return;
  }

  if (m_changeViewportTask)
    m_changeViewportTask->Unlock();

  m_changeViewportTask.reset();
}
