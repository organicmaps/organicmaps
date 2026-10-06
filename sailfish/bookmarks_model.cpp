#include "sailfish/bookmarks_model.hpp"

#include "sailfish/bookmarks_io.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/helpers.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/bookmarks_search_params.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/search_api.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "geometry/mercator.hpp"

#include "platform/distance.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <QDateTime>
#include <QPointer>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace sailfish
{
namespace
{
static_assert(BookmarksModel::ByType == static_cast<int>(BookmarkManager::SortingType::ByType));
static_assert(BookmarksModel::ByDistance == static_cast<int>(BookmarkManager::SortingType::ByDistance));
static_assert(BookmarksModel::ByTime == static_cast<int>(BookmarkManager::SortingType::ByTime));
static_assert(BookmarksModel::ByName == static_cast<int>(BookmarkManager::SortingType::ByName));

// Closes a place page showing an item of the list, which the core can't restore from the trash.
void DeleteList(Framework & framework, kml::MarkGroupId listId)
{
  auto & manager = framework.GetBookmarkManager();
  if (framework.HasPlacePageInfo())
  {
    auto const & info = framework.GetCurrentPlacePageInfo();
    kml::MarkGroupId groupId = kml::kInvalidMarkGroupId;
    if (info.IsBookmark())
    {
      if (auto const * bookmark = manager.GetBookmark(info.GetBookmarkId()))
        groupId = bookmark->GetGroupId();
    }
    else if (info.IsTrack())
    {
      if (auto const * track = manager.GetTrack(info.GetTrackId()))
        groupId = track->GetGroupId();
    }
    if (groupId == listId)
      framework.DeactivateMapSelection();
  }
  manager.GetEditSession().DeleteBmCategory(listId, false /* permanently */);
}

std::vector<std::string> ToStdStrings(QStringList const & strings)
{
  std::vector<std::string> result;
  for (auto const & s : strings)
    result.push_back(s.toStdString());
  return result;
}
}  // namespace

// static
BookmarksNotifier & BookmarksNotifier::Instance()
{
  static BookmarksNotifier notifier;
  return notifier;
}

// static
void BookmarksNotifier::LoadBookmarks(Framework & framework)
{
  auto & manager = framework.GetBookmarkManager();
  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onFinished = []
  {
    emit Instance().changed();
    emit Instance().loaded();
  };
  callbacks.m_onFileSuccess = [](std::string const & path, bool)
  { emit Instance().fileLoaded(QString::fromStdString(path), true); };
  callbacks.m_onFileError = [](std::string const & path, bool)
  { emit Instance().fileLoaded(QString::fromStdString(path), false); };
  manager.SetAsyncLoadingCallbacks(std::move(callbacks));
  manager.SetBookmarksChangedCallback([] { emit Instance().changed(); });
  framework.LoadBookmarks();
}

BookmarkCategoriesModel::BookmarkCategoriesModel(QObject * parent)
  : QAbstractListModel(parent)
  , m_framework(GetFramework())
{
  connect(&BookmarksNotifier::Instance(), &BookmarksNotifier::changed, this, &BookmarkCategoriesModel::Reset);
  Reset();
}

void BookmarkCategoriesModel::Reset()
{
  beginResetModel();
  auto const ids = m_framework.GetBookmarkManager().GetSortedBmGroupIdList();
  m_ids.assign(ids.begin(), ids.end());
  endResetModel();
  emit visibilityChanged();
  emit recentlyDeletedChanged();
}

bool BookmarkCategoriesModel::allInvisible() const
{
  return m_framework.GetBookmarkManager().AreAllCategoriesInvisible();
}

void BookmarkCategoriesModel::setAllVisible(bool visible)
{
  m_framework.GetBookmarkManager().SetAllCategoriesVisibility(visible);
  emit dataChanged(index(0), index(rowCount() - 1), {VisibleRole});
  emit visibilityChanged();
}

int BookmarkCategoriesModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_ids.size());
}

QVariant BookmarkCategoriesModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const id = m_ids[static_cast<size_t>(index.row())];
  auto const & manager = m_framework.GetBookmarkManager();
  switch (role)
  {
  case IdRole: return QVariant::fromValue<quint64>(id);
  case NameRole: return QString::fromStdString(manager.GetCategoryName(id));
  case BookmarksCountRole: return static_cast<int>(manager.GetUserMarkIds(id).size());
  case TracksCountRole: return static_cast<int>(manager.GetTrackIds(id).size());
  case VisibleRole: return manager.IsVisible(id);
  default: return {};
  }
}

QHash<int, QByteArray> BookmarkCategoriesModel::roleNames() const
{
  return {{IdRole, "listId"},
          {NameRole, "name"},
          {BookmarksCountRole, "bookmarksCount"},
          {TracksCountRole, "tracksCount"},
          {VisibleRole, "isVisible"}};
}

void BookmarkCategoriesModel::setVisible(int row, bool visible)
{
  ASSERT(row >= 0 && row < rowCount(), (row));
  m_framework.GetBookmarkManager().GetEditSession().SetIsVisible(m_ids[static_cast<size_t>(row)], visible);
  // Visibility doesn't trigger the changed callback.
  emit dataChanged(index(row), index(row), {VisibleRole});
  emit visibilityChanged();
}

void BookmarkCategoriesModel::deleteList(quint64 listId)
{
  auto & manager = m_framework.GetBookmarkManager();
  // The core keeps at least one list. A refused delete, after another list was deleted during the remorse timer,
  // still resets to bring back the collapsed row.
  if (manager.HasBmCategory(listId) && manager.GetBmGroupsCount() > 1)
    DeleteList(m_framework, listId);
  Reset();
}

int BookmarkCategoriesModel::recentlyDeletedCount() const
{
  return static_cast<int>(m_framework.GetBookmarkManager().GetRecentlyDeletedCategoriesCount());
}

QVariantList BookmarkCategoriesModel::recentlyDeleted() const
{
  QVariantList result;
  auto const collection = m_framework.GetBookmarkManager().GetRecentlyDeletedCategories();
  for (auto const & [path, data] : *collection)
  {
    auto const time = Platform::GetFileCreationTime(path);
    result.append(QVariantMap{{"name", QString::fromStdString(GetPreferredBookmarkStr(data->m_categoryData.m_name))},
                              {"path", QString::fromStdString(path)},
                              {"date", time > 0 ? QDateTime::fromTime_t(static_cast<uint>(time)) : QDateTime()}});
  }
  std::sort(result.begin(), result.end(), [](QVariant const & a, QVariant const & b)
  { return a.toMap()["date"].toDateTime() > b.toMap()["date"].toDateTime(); });
  return result;
}

void BookmarkCategoriesModel::recoverDeleted(QStringList const & paths)
{
  // The recovered lists load in the background; the changed callback follows.
  m_framework.GetBookmarkManager().RecoverRecentlyDeletedCategoriesAtPaths(ToStdStrings(paths));
  emit recentlyDeletedChanged();
}

void BookmarkCategoriesModel::deleteForever(QStringList const & paths)
{
  m_framework.GetBookmarkManager().DeleteRecentlyDeletedCategoriesAtPaths(ToStdStrings(paths));
  emit recentlyDeletedChanged();
}

void BookmarkCategoriesModel::showOnMap(int row)
{
  ASSERT(row >= 0 && row < rowCount(), (row));
  m_framework.ShowBookmarkCategory(m_ids[static_cast<size_t>(row)]);
}

BookmarksModel::BookmarksModel(QObject * parent) : QAbstractListModel(parent), m_framework(GetFramework())
{
  auto & notifier = BookmarksNotifier::Instance();
  connect(&notifier, &BookmarksNotifier::changed, this, &BookmarksModel::Reset);
  connect(&notifier, &BookmarksNotifier::positionFound, this, [this]
  {
    // Sorting by distance becomes available; a saved one waits for it.
    emit listInfoChanged();
    if (m_sortingType == ByDistance)
      Reset();
    else if (rowCount() > 0)
      emit dataChanged(index(0), index(rowCount() - 1), {DistanceRole});
  });
}

void BookmarksModel::setListId(quint64 id)
{
  if (id == m_listId)
    return;
  m_listId = id;
  auto const & manager = m_framework.GetBookmarkManager();
  BookmarkManager::SortingType type;
  m_sortingType = manager.HasBmCategory(id) && manager.GetLastSortingType(id, type) ? static_cast<int>(type) : -1;
  emit listIdChanged();
  emit sortingTypeChanged();
  Reset();
}

QString BookmarksModel::name() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return manager.HasBmCategory(m_listId) ? QString::fromStdString(manager.GetCategoryName(m_listId)) : QString();
}

QString BookmarksModel::description() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
    return {};
  return QString::fromStdString(kml::GetDefaultStr(manager.GetCategoryData(m_listId).m_description));
}

QString BookmarksModel::descriptionText() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
    return {};
  auto const & data = manager.GetCategoryData(m_listId);
  auto text = kml::GetDefaultStr(data.m_annotation);
  if (text.empty())
    text = kml::GetDefaultStr(data.m_description);
  // Plain text keeps its line breaks in HTML.
  return QString::fromStdString(text).replace('\n', QStringLiteral("<br>"));
}

int BookmarksModel::bookmarksCount() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return manager.HasBmCategory(m_listId) ? static_cast<int>(manager.GetUserMarkIds(m_listId).size()) : 0;
}

int BookmarksModel::tracksCount() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return manager.HasBmCategory(m_listId) ? static_cast<int>(manager.GetTrackIds(m_listId).size()) : 0;
}

void BookmarksModel::setSortingType(int type)
{
  if (type == m_sortingType)
    return;
  m_sortingType = type;
  if (auto & manager = m_framework.GetBookmarkManager(); manager.HasBmCategory(m_listId))
  {
    if (type < 0)
      manager.ResetLastSortingType(m_listId);
    else
      manager.SetLastSortingType(m_listId, static_cast<BookmarkManager::SortingType>(type));
  }
  emit sortingTypeChanged();
  Reset();
}

QVariantList BookmarksModel::sortingTypes() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  QVariantList types;
  if (!manager.HasBmCategory(m_listId))
    return types;
  for (auto const type : manager.GetAvailableSortingTypes(m_listId, m_framework.GetCurrentPosition().has_value()))
    types.append(static_cast<int>(type));
  return types;
}

void BookmarksModel::setFilter(QString const & filter)
{
  if (filter == m_filter)
    return;
  // Indexes the list on the search thread, ahead of the query.
  if (m_filter.trimmed().isEmpty() && !filter.trimmed().isEmpty())
  {
    if (auto & manager = m_framework.GetBookmarkManager(); manager.HasBmCategory(m_listId))
      manager.PrepareForSearch(m_listId);
  }
  m_filter = filter;
  emit filterChanged();
  Reset();
}

void BookmarksModel::setListInfo(QString const & name, QString const & description)
{
  auto & manager = m_framework.GetBookmarkManager();
  ASSERT(manager.HasBmCategory(m_listId), (m_listId));
  ASSERT(!name.trimmed().isEmpty(), ());
  // Only changes: each one saves the list file.
  auto session = manager.GetEditSession();
  if (name.trimmed() != this->name())
    session.SetCategoryName(m_listId, name.trimmed().toStdString());
  if (description.trimmed() != this->description())
    session.SetCategoryDescription(m_listId, description.trimmed().toStdString());
}

void BookmarksModel::Reset()
{
  int const request = ++m_request;
  // A renamed list changes the name.
  emit listInfoChanged();
  auto & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
  {
    SetItems({});
    return;
  }
  if (!m_filter.trimmed().isEmpty())
  {
    Search(request);
    return;
  }

  auto const position = m_framework.GetCurrentPosition();
  auto const type = static_cast<BookmarkManager::SortingType>(m_sortingType);
  // A saved order that isn't available now, like by distance before the first fix, which the core CHECKs, is kept
  // for later.
  if (m_sortingType < 0 || !base::IsExist(manager.GetAvailableSortingTypes(m_listId, position.has_value()), type))
  {
    SetDefaultOrder();
    return;
  }

  BookmarkManager::SortParams params;
  params.m_groupId = m_listId;
  params.m_sortingType = type;
  if (position)
  {
    params.m_hasMyPosition = true;
    params.m_myPosition = *position;
  }
  // Results come on the GUI thread, possibly after the list page is gone.
  QPointer<BookmarksModel> self(this);
  params.m_onResults =
      [self, request](BookmarkManager::SortedBlocksCollection && blocks, BookmarkManager::SortParams::Status status)
  {
    if (!self || request != self->m_request)
      return;
    // Cancelled when nothing is left to sort, e.g. after the last item was deleted: the user's choice stays.
    if (status != BookmarkManager::SortParams::Status::Completed)
    {
      self->SetDefaultOrder();
      return;
    }
    std::vector<Item> items;
    for (auto const & block : blocks)
    {
      QString const name = QString::fromStdString(block.m_blockName);
      for (auto const id : block.m_trackIds)
        items.push_back({id, true /* isTrack */, name});
      for (auto const id : block.m_markIds)
        items.push_back({id, false /* isTrack */, name});
    }
    self->SetItems(std::move(items));
  };
  manager.GetSortedCategory(params);
}

void BookmarksModel::Search(int request)
{
  using Params = search::BookmarksSearchParams;
  Params params;
  params.m_query = m_filter.trimmed().toStdString();
  params.m_groupId = m_listId;
  // Results come on the GUI thread, possibly after the list page is gone.
  QPointer<BookmarksModel> self(this);
  params.m_onResults = [self, request](Params::Results results, Params::Status status)
  {
    if (!self || request != self->m_request || status == Params::Status::Cancelled)
      return;
    // Not "not found" before the first results.
    if (status == Params::Status::InProgress && results.empty())
      return;
    self->m_framework.GetBookmarkManager().FilterInvalidBookmarks(results);
    std::vector<Item> items;
    for (auto const id : results)
      items.push_back({id, false /* isTrack */, {}});
    self->SetItems(std::move(items));
  };
  // Debug commands like ?dark aren't searched.
  if (!m_framework.GetSearchAPI().SearchInBookmarks(std::move(params)))
    SetItems({});
}

void BookmarksModel::SetDefaultOrder()
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
  {
    SetItems({});
    return;
  }
  auto const & trackIds = manager.GetTrackIds(m_listId);
  auto const & markIds = manager.GetUserMarkIds(m_listId);
  QString const tracksBlock = QString::fromStdString(BookmarkManager::GetTracksSortedBlockName());
  QString const marksBlock = QString::fromStdString(BookmarkManager::GetBookmarksSortedBlockName());
  std::vector<Item> items;
  for (auto const id : trackIds)
    items.push_back({id, true /* isTrack */, tracksBlock});
  for (auto const id : markIds)
    items.push_back({id, false /* isTrack */, marksBlock});
  SetItems(std::move(items));
}

void BookmarksModel::SetItems(std::vector<Item> && items)
{
  beginResetModel();
  m_items = std::move(items);
  endResetModel();
}

QString BookmarksModel::ItemName(Item const & item) const
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (item.m_isTrack)
  {
    auto const * track = manager.GetTrack(item.m_id);
    return track ? QString::fromStdString(track->GetName()) : QString();
  }
  auto const * bookmark = manager.GetBookmark(item.m_id);
  return bookmark ? QString::fromStdString(bookmark->GetPreferredName()) : QString();
}

int BookmarksModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant BookmarksModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const & item = m_items[static_cast<size_t>(index.row())];
  switch (role)
  {
  case IdRole: return QVariant::fromValue<quint64>(item.m_id);
  case IsTrackRole: return item.m_isTrack;
  case NameRole: return ItemName(item);
  case BlockRole: return item.m_block;
  default: break;
  }

  auto const & manager = m_framework.GetBookmarkManager();
  if (item.m_isTrack)
  {
    auto const * track = manager.GetTrack(item.m_id);
    if (!track)
      return {};
    switch (role)
    {
    case TypeRole:
      return QString::fromStdString(platform::Distance::CreateFormatted(track->GetLengthMeters()).ToString());
    case ColorRole: return ColorName(track->GetColor(0));
    case VisibleRole: return track->IsVisible();
    default: return {};
    }
  }

  auto const * bookmark = manager.GetBookmark(item.m_id);
  if (!bookmark)
    return {};
  switch (role)
  {
  case TypeRole: return QString::fromStdString(kml::GetLocalizedFeatureType(bookmark->GetData().m_featureTypes));
  case ColorRole: return ColorName(bookmark->GetColorForRendering());
  case DistanceRole:
  {
    auto const position = m_framework.GetCurrentPosition();
    if (!position)
      return QString();
    auto const meters = mercator::DistanceOnEarth(*position, bookmark->GetPivot());
    return QString::fromStdString(platform::Distance::CreateFormatted(meters).ToString());
  }
  default: return {};
  }
}

QHash<int, QByteArray> BookmarksModel::roleNames() const
{
  return {{IdRole, "itemId"},   {IsTrackRole, "isTrack"}, {NameRole, "name"},         {TypeRole, "type"},
          {BlockRole, "block"}, {ColorRole, "color"},     {DistanceRole, "distance"}, {VisibleRole, "isVisible"}};
}

BookmarksModel::Item const * BookmarksModel::ItemAt(int row) const
{
  ASSERT(row >= 0 && row < rowCount(), (row));
  // A sorted list is replaced asynchronously, so it can still hold deleted items, which the core CHECKs.
  auto const & item = m_items[static_cast<size_t>(row)];
  auto const & manager = m_framework.GetBookmarkManager();
  return (item.m_isTrack ? manager.HasTrack(item.m_id) : manager.HasBookmark(item.m_id)) ? &item : nullptr;
}

void BookmarksModel::showOnMap(int row)
{
  auto const * item = ItemAt(row);
  if (!item)
    return;
  if (item->m_isTrack)
    m_framework.ShowTrack(item->m_id);
  else
    m_framework.ShowBookmark(item->m_id);
}

QString BookmarksModel::shareText(int row) const
{
  auto const * item = ItemAt(row);
  if (!item)
    return {};
  ASSERT(!item->m_isTrack, ());
  return QString::fromStdString(m_framework.GetShareDataForBookmark(item->m_id).m_text);
}

QVariantList BookmarksModel::items(QVariantList const & rows) const
{
  QVariantList items;
  for (auto const & row : rows)
    if (auto const * item = ItemAt(row.toInt()))
      items.append(QVariantMap{{"id", QVariant::fromValue<quint64>(item->m_id)}, {"isTrack", item->m_isTrack}});
  return items;
}

void BookmarksModel::setTrackVisible(int row, bool visible)
{
  auto const * item = ItemAt(row);
  if (!item)
    return;
  ASSERT(item->m_isTrack, ());
  m_framework.SetTrackVisibility(item->m_id, visible);
  // Visibility doesn't trigger the changed callback.
  emit dataChanged(index(row), index(row), {VisibleRole});
}

void BookmarksModel::moveItems(QVariantList const & items, quint64 listId)
{
  if (!m_framework.GetBookmarkManager().HasBmCategory(listId))
    return;
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  SplitItems(items, marks, tracks);
  m_framework.GetBookmarkManager().GetEditSession().MoveBookmarksAndTracks(marks, tracks, listId);
}

void BookmarksModel::setItemsColor(QVariantList const & items, int colorIndex)
{
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  SplitItems(items, marks, tracks);
  SetColor(marks, tracks, colorIndex);
}

void BookmarksModel::setAllColor(bool tracks, int colorIndex)
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
    return;
  if (tracks)
  {
    auto const & ids = manager.GetTrackIds(m_listId);
    SetColor({}, {ids.begin(), ids.end()}, colorIndex);
  }
  else
  {
    auto const & ids = manager.GetUserMarkIds(m_listId);
    SetColor({ids.begin(), ids.end()}, {}, colorIndex);
  }
}

void BookmarksModel::SetColor(std::vector<uint64_t> const & marks, std::vector<uint64_t> const & tracks, int colorIndex)
{
  m_framework.GetBookmarkManager().GetEditSession().SetBookmarksAndTracksColor(marks, tracks, PresetColor(colorIndex));
}

void BookmarksModel::showListOnMap()
{
  auto & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_listId))
    return;
  manager.GetEditSession().SetIsVisible(m_listId, true);
  m_framework.ShowBookmarkCategory(m_listId);
}

void BookmarksModel::deleteList()
{
  auto const & manager = m_framework.GetBookmarkManager();
  // The core keeps at least one list.
  if (!manager.HasBmCategory(m_listId) || manager.GetBmGroupsCount() <= 1)
    return;
  DeleteList(m_framework, m_listId);
}
}  // namespace sailfish
