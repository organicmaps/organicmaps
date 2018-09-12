#include "map/user_mark_layer.hpp"
#include "map/search_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/tile_key.hpp"

#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

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
  m_tracks.clear();
}

bool UserMarkLayer::IsEmpty() const
{
  return m_userMarks.empty() && m_tracks.empty();
}

void UserMarkLayer::SetIsVisible(bool isVisible)
{
  if (m_isVisible != isVisible)
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

void UserMarkLayer::AttachUserMark(kml::MarkId markId)
{
  SetDirty();
  m_userMarks.insert(markId);
}

void UserMarkLayer::DetachUserMark(kml::MarkId markId)
{
  SetDirty();
  m_userMarks.erase(markId);
}

void UserMarkLayer::AttachTrack(kml::TrackId trackId)
{
  SetDirty();
  m_tracks.insert(trackId);
}

void UserMarkLayer::DetachTrack(kml::TrackId trackId)
{
  SetDirty();
  m_tracks.erase(trackId);
}
