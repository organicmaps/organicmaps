#include "balloon_manager.hpp"
#include "framework.hpp"

#include "../search/result.hpp"

#include "../anim/task.hpp"
#include "../anim/controller.hpp"

#include "../graphics/depth_constants.hpp"
#include "../graphics/opengl/base_texture.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"

#include "../geometry/transformations.hpp"

#include "../gui/controller.hpp"

PinClickManager::PinClickManager(Framework & f)
  : m_f(f)
{}

void PinClickManager::Hide()
{
  m_f.Invalidate();
}

void PinClickManager::OnClick(m2::PointD const & pxPoint, bool isLongTouch)
{
  UserMark const * mark = m_f.GetUserMark(pxPoint, isLongTouch);
  if (mark != NULL)
    OnActivateUserMark(mark);
  SetBalloonVisible(mark != NULL);
}

void PinClickManager::OnBookmarkClick(BookmarkAndCategory const & bnc)
{
  Bookmark * mark = m_f.GetBmCategory(bnc.first)->GetBookmark(bnc.second);
  m_f.GetBookmarkManager ().ActivateMark(mark);
  SetBalloonVisible(true);
}

void PinClickManager::SetBalloonVisible(bool isVisible)
{
  if (!isVisible)
    OnDismiss();

  m_f.Invalidate();
}

void PinClickManager::RemovePin()
{
  m_f.GetBookmarkManager().ActivateMark(NULL);
  m_f.Invalidate();
}

void PinClickManager::Dismiss()
{
  OnDismiss();
}

void PinClickManager::ClearListeners()
{
  m_userMarkListener.clear();
  m_dismissListener.clear();
}

void PinClickManager::OnActivateUserMark(UserMark const * mark)
{
  m_userMarkListener(mark);
}

void PinClickManager::OnDismiss()
{
  // Can be called before the listeners will be attached (clearing on activity start).
  if (m_dismissListener)
    m_dismissListener();
}
