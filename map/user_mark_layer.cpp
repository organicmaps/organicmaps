#include "map/user_mark_layer.hpp"
#include "map/search_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"

#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <utility>

UserMarkLayer::UserMarkLayer(UserMark::Type type)
  : m_type(type)
{
}

UserMarkLayer::~UserMarkLayer()
{
  Clear();
}

bool UserMarkLayer::IsVisible() const
{
  return m_isVisible;
}

bool UserMarkLayer::IsVisibilityChanged() const
{
  return m_isVisible != m_wasVisible;
}

UserMark::Type UserMarkLayer::GetType() const
{
  return m_type;
}

void UserMarkLayer::Clear()
{
  SetDirty();
  m_userMarks.clear();
}

void UserMarkLayer::SetIsVisible(bool isVisible)
{
  if (IsVisible() != isVisible)
  {
    SetDirty();
    m_isVisible = isVisible;
  }
}

void UserMarkLayer::ResetChanges()
{
  m_isDirty = false;
  m_wasVisible = m_isVisible;
}

void UserMarkLayer::AttachUserMark(df::MarkID markId)
{
  SetDirty();
  m_userMarks.insert(markId);
}

void UserMarkLayer::DetachUserMark(df::MarkID markId)
{
  SetDirty();
  m_userMarks.erase(markId);
}
