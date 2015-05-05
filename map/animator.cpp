#include "map/animator.hpp"
#include "map/rotate_screen_task.hpp"
#include "map/change_viewport_task.hpp"
#include "map/move_screen_task.hpp"
#include "map/framework.hpp"

#include "anim/controller.hpp"

#include "geometry/angles.hpp"

Animator::Animator(Framework * framework)
  : m_framework(framework)
{}

void Animator::StopAnimation(shared_ptr<anim::Task> const & task)
{
  if (task)
  {
    task->Lock();

    if (!task->IsEnded()
     && !task->IsCancelled())
      task->Cancel();

    task->Unlock();
  }
}

void Animator::RotateScreen(double startAngle, double endAngle)
{
  if (m_rotateScreenTask)
    m_rotateScreenTask->Lock();

  bool const inProgress =
      m_rotateScreenTask &&
      !m_rotateScreenTask->IsCancelled() &&
      !m_rotateScreenTask->IsEnded();

  if (inProgress)
  {
    m_rotateScreenTask->SetEndAngle(endAngle);
  }
  else
  {
    double const eps = my::DegToRad(1.5);

    if (fabs(ang::GetShortestDistance(startAngle, endAngle)) > eps)
    {
      if (m_rotateScreenTask)
      {
        m_rotateScreenTask->Unlock();
        m_rotateScreenTask.reset();
      }

      m_rotateScreenTask.reset(new RotateScreenTask(m_framework,
                                                    startAngle,
                                                    endAngle,
                                                    GetRotationSpeed()));

      //m_framework->GetAnimController()->AddTask(m_rotateScreenTask);
      return;
    }
  }

  if (m_rotateScreenTask)
    m_rotateScreenTask->Unlock();
}

void Animator::StopRotation()
{
  StopAnimation(m_rotateScreenTask);
  m_rotateScreenTask.reset();
}

shared_ptr<MoveScreenTask> const & Animator::MoveScreen(m2::PointD const & startPt,
                                                        m2::PointD const & endPt,
                                                        double speed)
{
  StopMoveScreen();

  m_moveScreenTask.reset(new MoveScreenTask(m_framework,
                                            startPt,
                                            endPt,
                                            speed));

  //m_framework->GetAnimController()->AddTask(m_moveScreenTask);

  return m_moveScreenTask;
}

void Animator::StopMoveScreen()
{
  StopAnimation(m_moveScreenTask);
  m_moveScreenTask.reset();
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

  //m_framework->GetAnimController()->AddTask(m_changeViewportTask);

  return m_changeViewportTask;
}

void Animator::StopChangeViewport()
{
  StopAnimation(m_changeViewportTask);
  m_changeViewportTask.reset();
}

double Animator::GetRotationSpeed() const
{
  // making full circle in ~1 seconds.
  return 6.0;
}
