#include "pin_click_manager.hpp"
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

void PinClickManager::OnShowMark(UserMark const * mark)
{
  if (mark != NULL)
    OnActivateUserMark(mark->Copy());
  SetBalloonVisible(mark != NULL);
}

void PinClickManager::SetBalloonVisible(bool isVisible)
{
  if (!isVisible && m_f.HasActiveUserMark())
    OnDismiss();

  m_f.Invalidate();
}

void PinClickManager::RemovePin()
{
  m_f.ActivateUserMark(NULL);
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

void PinClickManager::OnActivateUserMark(UserMarkCopy * mark)
{
  m_userMarkListener(mark);
}

void PinClickManager::OnDismiss()
{
  // Can be called before the listeners will be attached (clearing on activity start).
  if (m_dismissListener)
    m_dismissListener();
}
