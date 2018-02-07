#include "map/user_mark_container.hpp"
#include "map/search_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"

#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <utility>

UserMarkContainer::UserMarkContainer(UserMark::Type type)
  : m_type(type)
{
}

UserMarkContainer::~UserMarkContainer()
{
  Clear();
}

bool UserMarkContainer::IsVisible() const
{
  return m_isVisible;
}

bool UserMarkContainer::IsVisibilityChanged() const
{
  return m_isVisible != m_wasVisible;
}

UserMark::Type UserMarkContainer::GetType() const
{
  return m_type;
}

void UserMarkContainer::Clear()
{
  SetDirty();
  m_userMarks.clear();
}

void UserMarkContainer::SetIsVisible(bool isVisible)
{
  if (IsVisible() != isVisible)
  {
    SetDirty();
    m_isVisible = isVisible;
  }
}

void UserMarkContainer::ResetChanges()
{
  m_isDirty = false;
  m_wasVisible = m_isVisible;
}

void UserMarkContainer::AttachUserMark(df::MarkID markId)
{
  SetDirty();
  m_userMarks.insert(markId);
}

void UserMarkContainer::DetachUserMark(df::MarkID markId)
{
  SetDirty();
  m_userMarks.erase(markId);
}
