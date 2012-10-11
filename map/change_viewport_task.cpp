#include "change_viewport_task.hpp"
#include "framework.hpp"

ChangeViewportTask::ChangeViewportTask(m2::AnyRectD const & startRect,
                                       m2::AnyRectD const & endRect,
                                       double rotationSpeed,
                                       Framework *framework)
  : anim::AnyRectInterpolation(startRect,
                               endRect,
                               rotationSpeed,
                               m_outRect),
    m_framework(framework)
{
}

void ChangeViewportTask::OnStep(double ts)
{
  anim::AnyRectInterpolation::OnStep(ts);
  m_framework->GetNavigator().SetFromRect(m_outRect);
}

void ChangeViewportTask::OnEnd(double ts)
{
  anim::AnyRectInterpolation::OnEnd(ts);
  m_framework->GetNavigator().SetFromRect(m_outRect);
}

bool ChangeViewportTask::IsVisual() const
{
  return true;
}
