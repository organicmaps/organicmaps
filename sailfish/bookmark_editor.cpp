#include "sailfish/bookmark_editor.hpp"

#include "sailfish/framework_access.hpp"
#include "sailfish/helpers.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "base/assert.hpp"

namespace sailfish
{
BookmarkEditor::BookmarkEditor(QObject * parent) : QObject(parent), m_framework(GetFramework()) {}

void BookmarkEditor::loadBookmark(quint64 id)
{
  auto const * bookmark = m_framework.GetBookmarkManager().GetBookmark(id);
  CHECK(bookmark, (id));
  m_isTrack = false;
  m_id = id;
  m_listId = bookmark->GetGroupId();
  m_name = QString::fromStdString(bookmark->GetPreferredName());
  m_description = QString::fromStdString(bookmark->GetDescription());
  m_colorIndex = PresetIndex(bookmark->GetColorForRendering());
  emit loaded();
}

void BookmarkEditor::loadTrack(quint64 id)
{
  auto const * track = m_framework.GetBookmarkManager().GetTrack(id);
  CHECK(track, (id));
  m_isTrack = true;
  m_id = id;
  m_listId = track->GetGroupId();
  m_name = QString::fromStdString(track->GetName());
  m_description = QString::fromStdString(track->GetDescription());
  m_colorIndex = PresetIndex(track->GetColor(0));
  emit loaded();
}

bool BookmarkEditor::trackVisible() const
{
  return m_isTrack && m_framework.GetBookmarkManager().GetTrack(m_id)->IsVisible();
}

void BookmarkEditor::save(QString const & name, QString const & description, int colorIndex, quint64 listId,
                          bool trackVisible)
{
  auto & manager = m_framework.GetBookmarkManager();
  // May have been deleted meanwhile, e.g. from the list page.
  if (!IsSavedUserItem(manager, m_id, m_isTrack))
    return;
  if (!manager.HasBmCategory(listId))
    listId = m_listId;
  auto const newName = name.trimmed().toStdString();
  // Presets are stored as custom colors too.
  bool const colorChanged = colorIndex != m_colorIndex;
  {
    auto session = manager.GetEditSession();
    if (m_isTrack)
    {
      auto data = manager.GetTrack(m_id)->GetData();
      kml::SetDefaultStr(data.m_name, newName);
      kml::SetDefaultStr(data.m_description, description.toStdString());
      session.UpdateTrack(m_id, data);
      if (colorChanged)
        session.ChangeTrackColor(m_id, PresetColor(colorIndex));
      if (listId != m_listId)
        session.MoveTrack(m_id, m_listId, listId);
    }
    else
    {
      auto const * bookmark = manager.GetBookmark(m_id);
      auto data = bookmark->GetData();
      // An unchanged name stays the feature name instead of becoming a custom one.
      if (bookmark->GetPreferredName() != newName)
        kml::SetDefaultStr(data.m_customName, newName);
      kml::SetDefaultStr(data.m_description, description.toStdString());
      if (colorChanged)
        data.m_color = kml::MakeCustomBookmarkColorData(PresetColor(colorIndex));
      session.UpdateBookmark(m_id, data);
      if (listId != m_listId)
        session.MoveBookmark(m_id, m_listId, listId);
    }
  }
  if (m_isTrack)
    m_framework.SetTrackVisibility(m_id, trackVisible);
  if (IsSelected())
    m_framework.UpdatePlacePageInfoForCurrentSelection();
}

bool BookmarkEditor::IsSelected() const
{
  if (!m_framework.HasPlacePageInfo())
    return false;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  return (m_isTrack ? info.GetTrackId() : info.GetBookmarkId()) == m_id;
}
}  // namespace sailfish
