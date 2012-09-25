#include "animator.hpp"
#include "rotate_screen_task.hpp"
#include "framework.hpp"

#include "../anim/controller.hpp"

#include "../geometry/angles.hpp"

Animator::Animator(Framework * framework)
  : m_framework(framework)
{
  m_rotationThreshold = ang::DegreeToRad(10);
}

void Animator::RotateScreen(double startAngle, double endAngle, double duration)
{
  bool shouldRotate = false;

  if (m_rotateScreenTask
   && !m_rotateScreenTask->IsCancelled()
   && !m_rotateScreenTask->IsEnded())
  {
    // if the end angle seriously changed we should re-create rotation task.
    if (fabs(ang::GetShortestDistance(m_rotateScreenTask->EndAngle(), endAngle)) > m_rotationThreshold)
      shouldRotate = true;
  }
  else
  {
    // if there are no current rotate screen task or the task is finished already
    // we check for the distance between current screen angle and headingAngle
    if (fabs(ang::GetShortestDistance(startAngle, endAngle)) > m_rotationThreshold)
      shouldRotate = true;
  }

  if (shouldRotate)
  {
    StopRotation();
    m_rotateScreenTask.reset(new RotateScreenTask(m_framework,
                                                  startAngle,
                                                  endAngle,
                                                  duration));

    m_framework->GetAnimController()->AddTask(m_rotateScreenTask);
  }
}

void Animator::StopRotation()
{
  if (m_rotateScreenTask
   && !m_rotateScreenTask->IsEnded()
   && !m_rotateScreenTask->IsCancelled())
    m_rotateScreenTask->Cancel();

  m_rotateScreenTask.reset();
}
