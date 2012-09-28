#include "move_screen_task.hpp"

#include "framework.hpp"

MoveScreenTask::MoveScreenTask(Framework * framework,
                               m2::PointD const & startPt,
                               m2::PointD const & endPt,
                               double interval)
  : anim::SegmentInterpolation(startPt,
                               endPt,
                               interval,
                               m_outPt),
    m_framework(framework)
{}

void MoveScreenTask::OnStep(double ts)
{
  anim::SegmentInterpolation::OnStep(ts);
  m_framework->GetNavigator().SetOrg(m_outPt);
}

void MoveScreenTask::OnEnd(double ts)
{
  anim::SegmentInterpolation::OnEnd(ts);
  m_framework->GetNavigator().SetOrg(m_outPt);
}
