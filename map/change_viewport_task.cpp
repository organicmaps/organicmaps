#include "map/change_viewport_task.hpp"
#include "map/framework.hpp"

ChangeViewportTask::ChangeViewportTask(m2::AnyRectD const & startRect,
                                       m2::AnyRectD const & endRect,
                                       double rotationSpeed,
                                       Framework * framework)
  : BaseT(startRect, endRect, rotationSpeed, m_outRect),
    m_framework(framework)
{
}

void ChangeViewportTask::OnStep(double ts)
{
//  BaseT::OnStep(ts);
//  m_framework->ShowRectEx(m_outRect.GetGlobalRect());
}

void ChangeViewportTask::OnEnd(double ts)
{
//  BaseT::OnEnd(ts);
//  m_framework->ShowRectEx(m_outRect.GetGlobalRect());
}

bool ChangeViewportTask::IsVisual() const
{
  return true;
}
