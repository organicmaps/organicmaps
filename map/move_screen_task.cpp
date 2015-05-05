#include "map/move_screen_task.hpp"

#include "map/framework.hpp"

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
  m2::PointD oldPt = m_outPt;
  anim::SegmentInterpolation::OnStep(ts);
//  Navigator & nav = m_framework->GetNavigator();
//  nav.SetOrg(nav.Screen().GetOrg() + m_outPt - oldPt);
}

void MoveScreenTask::OnEnd(double ts)
{
  anim::SegmentInterpolation::OnEnd(ts);
//  Navigator & nav = m_framework->GetNavigator();
//  nav.SetOrg(m_outPt);
  m_framework->UpdateUserViewportChanged();
}

bool MoveScreenTask::IsVisual() const
{
  return true;
}
