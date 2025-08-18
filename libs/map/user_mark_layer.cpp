#include "map/user_mark_layer.hpp"

UserMarkLayer::UserMarkLayer(UserMark::Type type) : m_type(type) {}

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
    SetDirty(false /* updateModificationDate */);
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
