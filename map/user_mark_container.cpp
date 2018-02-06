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

UserMark::Type UserMarkContainer::GetType() const
{
  return m_type;
}

void UserMarkContainer::Clear()
{
  SetDirty();
  for (auto const & markId : m_userMarks)
  {
    if (m_createdMarks.find(markId) == m_createdMarks.end())
      m_removedMarks.insert(markId);
  }
  m_createdMarks.clear();
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

void UserMarkContainer::Update()
{
  SetDirty();
}

void UserMarkContainer::SetDirty()
{
  m_isDirty = true;
}

bool UserMarkContainer::IsDirty() const
{
  return m_isDirty;
}

void UserMarkContainer::ResetChanges()
{
  m_createdMarks.clear();
  m_removedMarks.clear();
  m_updatedMarks.clear();
  m_isDirty = false;
}


void UserMarkContainer::AttachUserMark(df::MarkID markId)
{
  SetDirty();
  m_createdMarks.insert(markId);
  m_userMarks.insert(markId);
}

void UserMarkContainer::EditUserMark(df::MarkID markId)
{
  SetDirty();
  m_updatedMarks.insert(markId);
}

void UserMarkContainer::DetachUserMark(df::MarkID markId)
{
  SetDirty();
  auto const it = m_createdMarks.find(markId);
  if (it != m_createdMarks.end())
    m_createdMarks.erase(it);
  else
    m_removedMarks.insert(markId);
  m_userMarks.erase(markId);
}
