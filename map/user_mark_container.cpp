#include "map/user_mark_container.hpp"
#include "map/search_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"

#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <utility>

UserMarkContainer::UserMarkContainer(UserMark::Type type,
                                     Listeners const & listeners)
  : m_type(type)
  , m_listeners(listeners)
{
}

UserMarkContainer::~UserMarkContainer()
{
  Clear();
}

void UserMarkContainer::NotifyListeners()
{
  if (!IsDirty())
    return;

  if (m_listeners.m_createListener != nullptr && !m_createdMarks.empty())
  {
    df::IDCollection marks(m_createdMarks.begin(), m_createdMarks.end());
    m_listeners.m_createListener(*this, marks);
  }
  if (m_listeners.m_updateListener != nullptr && !m_updatedMarks.empty())
  {
    df::IDCollection marks(m_updatedMarks.begin(), m_updatedMarks.end());
    m_listeners.m_updateListener(*this, marks);
  }
  if (m_listeners.m_deleteListener != nullptr && !m_removedMarks.empty())
  {
    df::IDCollection marks(m_removedMarks.begin(), m_removedMarks.end());
    m_listeners.m_deleteListener(*this, marks);
  }
}

size_t UserMarkContainer::GetUserPointCount() const
{
  return m_userMarks.size();
}

size_t UserMarkContainer::GetUserLineCount() const
{
  return m_userLines.size();
}

bool UserMarkContainer::IsVisible() const
{
  return m_isVisible;
}

size_t UserMarkContainer::GetUserMarkCount() const
{
  return GetUserPointCount();
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

void UserMarkContainer::AcceptChanges(df::MarkIDCollection & groupMarks,
                                      df::MarkIDCollection & createdMarks,
                                      df::MarkIDCollection & removedMarks)
{
  groupMarks.Clear();
  createdMarks.Clear();
  removedMarks.Clear();

  groupMarks.m_marksID.reserve(m_userMarks.size());
  for(auto const & markId : m_userMarks)
    groupMarks.m_marksID.push_back(markId);

  createdMarks.m_marksID.reserve(m_createdMarks.size());
  for (auto const & markId : m_createdMarks)
    createdMarks.m_marksID.push_back(markId);
  m_createdMarks.clear();

  removedMarks.m_marksID.reserve(m_removedMarks.size());
  for (auto const & markId : m_removedMarks)
    removedMarks.m_marksID.push_back(markId);
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
