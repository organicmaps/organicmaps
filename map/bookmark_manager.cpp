#include "map/bookmark_manager.hpp"
#include "map/search_api.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_id_storage.hpp"
#include "map/track_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/classificator.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/serdes_json.hpp"
#include "coding/zip_creator.hpp"

#include "geometry/rect_intersect.hpp"

#include "base/file_name_utils.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>


namespace
{
std::string const kLastEditedBookmarkCategory = "LastBookmarkCategory";
std::string const kLastEditedBookmarkColor = "LastBookmarkColor";
std::string const kMetadataFileName = "bm.json";
std::string const kSortingTypeProperty = "sortingType";
std::string const kLargestBookmarkSymbolName = "bookmark-default-m";
size_t const kMinCommonTypesCount = 3;
double const kNearDistanceInMeters = 20 * 1000.0;
double const kMyPositionTrackSnapInMeters = 20.0;

class FindMarkFunctor
{
public:
  FindMarkFunctor(UserMark const ** mark, double & minD, m2::AnyRectD const & rect)
    : m_mark(mark)
    , m_minD(minD)
    , m_rect(rect)
  {
    m_globalCenter = rect.GlobalCenter();
  }

  void operator()(UserMark const * mark)
  {
    m2::PointD const & org = mark->GetPivot();
    if (m_rect.IsPointInside(org))
    {
      double minDCandidate = m_globalCenter.SquaredLength(org);
      if (minDCandidate < m_minD)
      {
        *m_mark = mark;
        m_minD = minDCandidate;
      }
    }
  }

  UserMark const ** m_mark;
  double & m_minD;
  m2::AnyRectD const & m_rect;
  m2::PointD m_globalCenter;
};

BookmarkManager::SharingResult GetFileForSharing(BookmarkManager::KMLDataCollectionPtr collection)
{
  auto const & kmlToShare = collection->front();
  std::string fileName = RemoveInvalidSymbols(kml::GetDefaultStr(kmlToShare.second->m_categoryData.m_name));
  if (fileName.empty())
    fileName = base::GetNameFromFullPathWithoutExt(kmlToShare.first);

  auto const filePath = base::JoinPath(GetPlatform().TmpDir(), fileName + kKmlExtension);
  SCOPE_GUARD(fileGuard, std::bind(&base::DeleteFileX, filePath));

  auto const categoryId = kmlToShare.second->m_categoryData.m_id;

  if (!SaveKmlFileSafe(*kmlToShare.second, filePath, KmlFileType::Text))
  {
    return BookmarkManager::SharingResult(categoryId, BookmarkManager::SharingResult::Code::FileError,
                                          "Bookmarks file does not exist.");
  }

  auto const tmpFilePath = base::JoinPath(GetPlatform().TmpDir(), fileName + kKmzExtension);
  if (!CreateZipFromPathDeflatedAndDefaultCompression(filePath, tmpFilePath))
  {
    return BookmarkManager::SharingResult(categoryId, BookmarkManager::SharingResult::Code::ArchiveError,
                                          "Could not create archive.");
  }

  return BookmarkManager::SharingResult(categoryId, tmpFilePath);
}

std::string ToString(BookmarkManager::SortingType type)
{
  switch (type)
  {
  case BookmarkManager::SortingType::ByTime: return "ByTime";
  case BookmarkManager::SortingType::ByType: return "ByType";
  case BookmarkManager::SortingType::ByDistance: return "ByDistance";
  }
  UNREACHABLE();
}

bool GetSortingType(std::string const & typeStr, BookmarkManager::SortingType & type)
{
  if (typeStr == ToString(BookmarkManager::SortingType::ByTime))
    type = BookmarkManager::SortingType::ByTime;
  else if (typeStr == ToString(BookmarkManager::SortingType::ByType))
    type = BookmarkManager::SortingType::ByType;
  else if (typeStr == ToString(BookmarkManager::SortingType::ByDistance))
    type = BookmarkManager::SortingType::ByDistance;
  else
    return false;
  return true;
}
}  // namespace

BookmarkManager::BookmarkManager(Callbacks && callbacks)
  : m_callbacks(std::move(callbacks))
  , m_changesTracker(this)
  , m_bookmarksChangesTracker(this)
  , m_drapeChangesTracker(this)
  , m_needTeardown(false)
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());

  m_userMarkLayers.reserve(UserMark::USER_MARK_TYPES_COUNT - 1);
  for (uint32_t i = 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkLayer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD{});
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});

  m_trackInfoMarkId = CreateUserMark<TrackInfoMark>(m2::PointD{})->GetId();
}

BookmarkManager::EditSession BookmarkManager::GetEditSession()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return EditSession(*this);
}

UserMark const * BookmarkManager::GetMark(kml::MarkId markId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (IsBookmark(markId))
    return GetBookmark(markId);
  return GetUserMark(markId);
}

UserMark const * BookmarkManager::GetUserMark(kml::MarkId markId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  return (it != m_userMarks.end()) ? it->second.get() : nullptr;
}

UserMark * BookmarkManager::GetUserMarkForEdit(kml::MarkId markId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  if (it != m_userMarks.end())
  {
    m_changesTracker.OnUpdateMark(markId);
    return it->second.get();
  }
  return nullptr;
}

void BookmarkManager::DeleteUserMark(kml::MarkId markId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(!IsBookmark(markId), ());
  auto it = m_userMarks.find(markId);
  auto const groupId = it->second->GetGroupId();
  GetGroup(groupId)->DetachUserMark(markId);
  m_changesTracker.OnDeleteMark(markId);
  m_userMarks.erase(it);
}

Bookmark * BookmarkManager::CreateBookmark(kml::BookmarkData && bmData)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return AddBookmark(std::make_unique<Bookmark>(std::move(bmData)));
}

Bookmark * BookmarkManager::CreateBookmark(kml::BookmarkData && bm, kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const & c = classif();
  CHECK(c.HasTypesMapping(), ());
  std::stringstream ss;
  for (size_t i = 0; i < bm.m_featureTypes.size(); ++i)
  {
    ss << c.GetReadableObjectName(c.GetTypeForIndex(bm.m_featureTypes[i]));
    if (i + 1 < bm.m_featureTypes.size())
      ss << ",";
  }

  bm.m_timestamp = kml::TimestampClock::now();
  bm.m_viewportScale = static_cast<uint8_t>(df::GetZoomLevel(m_viewport.GetScale()));

  auto * bookmark = CreateBookmark(std::move(bm));
  bookmark->Attach(groupId);
  auto * group = GetBmCategory(groupId);
  group->AttachUserMark(bookmark->GetId());
  m_changesTracker.OnAttachBookmark(bookmark->GetId(), groupId);
  group->SetIsVisible(true);

  SetLastEditedBmCategory(groupId);
  SetLastEditedBmColor(bookmark->GetData().m_color.m_predefinedColor);

  return bookmark;
}

Bookmark const * BookmarkManager::GetBookmark(kml::MarkId markId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_bookmarks.find(markId);
  return (it != m_bookmarks.end()) ? it->second.get() : nullptr;
}

Bookmark * BookmarkManager::GetBookmarkForEdit(kml::MarkId markId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_bookmarks.find(markId);
  if (it == m_bookmarks.end())
    return nullptr;

  auto const groupId = it->second->GetGroupId();
  if (groupId != kml::kInvalidMarkGroupId)
    m_changesTracker.OnUpdateMark(markId);

  return it->second.get();
}

void BookmarkManager::AttachBookmark(kml::MarkId bmId, kml::MarkGroupId catId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Attach(catId);
  GetGroup(catId)->AttachUserMark(bmId);
  m_changesTracker.OnAttachBookmark(bmId, catId);
}

void BookmarkManager::DetachBookmark(kml::MarkId bmId, kml::MarkGroupId catId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Detach();
  DetachUserMark(bmId, catId);
}

void BookmarkManager::DeleteBookmark(kml::MarkId bmId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmark(bmId), ());
  auto const it = m_bookmarks.find(bmId);
  CHECK(it != m_bookmarks.end(), ());
  auto const groupId = it->second->GetGroupId();

  if (groupId != kml::kInvalidMarkGroupId)
    DetachUserMark(bmId, groupId);

  m_changesTracker.OnDeleteMark(bmId);
  m_bookmarks.erase(it);
}

void BookmarkManager::DetachUserMark(kml::MarkId bmId, kml::MarkGroupId catId)
{
  GetGroup(catId)->DetachUserMark(bmId);
  for (auto const compilationId : GetCategoryData(catId).m_compilationIds)
  {
    GetGroup(compilationId)->DetachUserMark(bmId);
  }
  m_changesTracker.OnDetachBookmark(bmId, catId);
}

void BookmarkManager::DeleteCompilations(kml::GroupIdCollection const & compilations)
{
  for (auto const compilationId : compilations)
  {
    m_compilations.erase(compilationId);
  }
}

Track * BookmarkManager::CreateTrack(kml::TrackData && trackData)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return AddTrack(std::make_unique<Track>(std::move(trackData), false /* interactive */));
}

Track const * BookmarkManager::GetTrack(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  return (it != m_tracks.end()) ? it->second.get() : nullptr;
}

Track * BookmarkManager::GetTrackForEdit(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  if (it == m_tracks.end())
    return nullptr;

  return it->second.get();
}

void BookmarkManager::MoveTrack(kml::TrackId trackID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  DetachTrack(trackID, curGroupID);
  AttachTrack(trackID, newGroupID);
}

void BookmarkManager::AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  it->second->Attach(groupId);
  GetBmCategory(groupId)->AttachTrack(trackId);
}

void BookmarkManager::DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  it->second->Detach();
  GetBmCategory(groupId)->DetachTrack(trackId);
}

void BookmarkManager::DeleteTrack(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  DeleteTrackSelectionMark(trackId);
  auto it = m_tracks.find(trackId);
  auto const groupId = it->second->GetGroupId();
  if (groupId != kml::kInvalidMarkGroupId)
    GetBmCategory(groupId)->DetachTrack(trackId);
  m_changesTracker.OnDeleteLine(trackId);
  m_tracks.erase(it);
}

void BookmarkManager::GetDirtyGroups(kml::GroupIdSet & dirtyGroups) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & group : m_userMarkLayers)
  {
    if (!group->IsDirty())
      continue;
    auto const groupId = static_cast<kml::MarkGroupId>(group->GetType());
    dirtyGroups.insert(groupId);
  }
  for (auto const & group : m_categories)
  {
    if (!group.second->IsDirty())
      continue;
    dirtyGroups.insert(group.first);
  }
}

void BookmarkManager::OnEditSessionOpened()
{
  ++m_openedEditSessionsCount;
}

void BookmarkManager::OnEditSessionClosed()
{
  ASSERT_GREATER(m_openedEditSessionsCount, 0, ());
  if (--m_openedEditSessionsCount == 0)
    NotifyChanges();
}

void BookmarkManager::NotifyChanges()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_changesTracker.AcceptDirtyItems();
  if (!m_firstDrapeNotification &&
    !m_changesTracker.HasChanges() &&
    !m_bookmarksChangesTracker.HasChanges() &&
    !m_drapeChangesTracker.HasChanges())
  {
    return;
  }

  if (m_changesTracker.HasBookmarksChanges())
    NotifyBookmarksChanged();

  if (m_changesTracker.HasCategoriesChanges())
    NotifyCategoriesChanged();

  m_bookmarksChangesTracker.AddChanges(m_changesTracker);
  m_drapeChangesTracker.AddChanges(m_changesTracker);
  m_changesTracker.ResetChanges();

  if (!m_notificationsEnabled)
    return;

  if (m_bookmarksChangesTracker.HasBookmarksChanges())
  {
    kml::GroupIdCollection categoriesToSave;
    for (auto groupId : m_bookmarksChangesTracker.GetUpdatedGroupIds())
    {
      if (IsBookmarkCategory(groupId) && GetBmCategory(groupId)->IsAutoSaveEnabled())
        categoriesToSave.push_back(groupId);
    }

    SaveBookmarks(categoriesToSave);
    SendBookmarksChanges(m_bookmarksChangesTracker);
  }
  m_bookmarksChangesTracker.ResetChanges();

  if (!m_drapeChangesTracker.HasChanges())
    return;

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    auto engine = lock.Get();
    for (auto groupId : m_drapeChangesTracker.GetUpdatedGroupIds())
    {
      auto * group = GetGroup(groupId);
      engine->ChangeVisibilityUserMarksGroup(groupId, group->IsVisible());
    }

    for (auto groupId : m_drapeChangesTracker.GetRemovedGroupIds())
      engine->ClearUserMarksGroup(groupId);

    engine->UpdateUserMarks(&m_drapeChangesTracker, m_firstDrapeNotification);
    m_firstDrapeNotification = false;

    engine->InvalidateUserMarks();
    m_drapeChangesTracker.ResetChanges();
  }
}

kml::MarkIdSet const & BookmarkManager::GetUserMarkIds(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserMarks();
}

kml::TrackIdSet const & BookmarkManager::GetTrackIds(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserLines();
}

bool BookmarkManager::GetLastSortingType(kml::MarkGroupId groupId, SortingType & sortingType) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return false;

  std::string sortingTypeStr;
  if (m_metadata.GetEntryProperty(entryName, kSortingTypeProperty, sortingTypeStr))
    return GetSortingType(sortingTypeStr, sortingType);
  return false;
}

void BookmarkManager::SetLastSortingType(kml::MarkGroupId groupId, SortingType sortingType)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return;

  m_metadata.m_entriesProperties[entryName].m_values[kSortingTypeProperty] = ToString(sortingType);
  SaveMetadata();
}

void BookmarkManager::ResetLastSortingType(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return;

  m_metadata.m_entriesProperties[entryName].m_values.erase(kSortingTypeProperty);
  SaveMetadata();
}

std::vector<BookmarkManager::SortingType> BookmarkManager::GetAvailableSortingTypes(
  kml::MarkGroupId groupId, bool hasMyPosition) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(groupId), ());

  auto const * group = GetGroup(groupId);
  CHECK(group != nullptr, ());

  bool byTypeChecked = false;
  bool byTimeChecked = false;

  std::map<BookmarkBaseType, size_t> typesCount;
  for (auto markId : group->GetUserMarks())
  {
    auto const & bookmarkData = GetBookmark(markId)->GetData();

    if (!byTypeChecked && !bookmarkData.m_featureTypes.empty())
    {
      auto const type = GetBookmarkBaseType(bookmarkData.m_featureTypes);
      if (type == BookmarkBaseType::Hotel)
      {
        byTypeChecked = true;
      }
      else if (type != BookmarkBaseType::None)
      {
        auto const count = ++typesCount[type];
        byTypeChecked = (count == kMinCommonTypesCount);
      }
    }

    if (!byTimeChecked)
      byTimeChecked = !kml::IsEqual(bookmarkData.m_timestamp, kml::Timestamp());

    if (byTypeChecked && byTimeChecked)
      break;
  }

  if (!byTimeChecked)
  {
    for (auto trackId : group->GetUserLines())
    {
      if (!kml::IsEqual(GetTrack(trackId)->GetData().m_timestamp, kml::Timestamp()))
      {
        byTimeChecked = true;
        break;
      }
    }
  }

  std::vector<SortingType> sortingTypes;
  if (byTypeChecked)
    sortingTypes.push_back(SortingType::ByType);
  if (hasMyPosition && !group->GetUserMarks().empty())
    sortingTypes.push_back(SortingType::ByDistance);
  if (byTimeChecked)
    sortingTypes.push_back(SortingType::ByTime);

  return sortingTypes;
}

template <typename T, typename R>
BookmarkManager::SortedByTimeBlockType GetSortedByTimeBlockType(
  std::chrono::duration<T, R> const & timePeriod)
{
  auto constexpr kDay = std::chrono::hours(24);
  auto constexpr kWeek = 7 * kDay;
  auto constexpr kMonth = 31 * kDay;
  auto constexpr kYear = 365 * kDay;

  if (timePeriod < kWeek)
    return BookmarkManager::SortedByTimeBlockType::WeekAgo;
  if (timePeriod < kMonth)
    return BookmarkManager::SortedByTimeBlockType::MonthAgo;
  if (timePeriod < kYear)
    return BookmarkManager::SortedByTimeBlockType::MoreThanMonthAgo;
  return BookmarkManager::SortedByTimeBlockType::MoreThanYearAgo;
}

// static
std::string BookmarkManager::GetSortedByTimeBlockName(SortedByTimeBlockType blockType)
{
  switch (blockType)
  {
  case SortedByTimeBlockType::WeekAgo:
    return platform::GetLocalizedString("week_ago_sorttype");
  case SortedByTimeBlockType::MonthAgo:
    return platform::GetLocalizedString("month_ago_sorttype");
  case SortedByTimeBlockType::MoreThanMonthAgo:
    return platform::GetLocalizedString("moremonth_ago_sorttype");
  case SortedByTimeBlockType::MoreThanYearAgo:
    return platform::GetLocalizedString("moreyear_ago_sorttype");
  case SortedByTimeBlockType::Others:
    return GetOthersSortedBlockName();
  }
  UNREACHABLE();
}

// static
std::string BookmarkManager::GetTracksSortedBlockName()
{
  return platform::GetLocalizedString("tracks_title");
}

// static
std::string BookmarkManager::GetOthersSortedBlockName()
{
  return platform::GetLocalizedString("others_sorttype");
}

// static
std::string BookmarkManager::GetNearMeSortedBlockName()
{
  return platform::GetLocalizedString("near_me_sorttype");
}

std::string BookmarkManager::GetLocalizedRegionAddress(m2::PointD const & pt)
{
  CHECK(m_testModeEnabled, ());

  std::unique_lock<std::mutex> lock(m_regionAddressMutex);
  if (m_regionAddressGetter == nullptr)
  {
    LOG(LWARNING, ("Region address getter is not set. Address getting failed."));
    return {};
  }
  return m_regionAddressGetter->GetLocalizedRegionAddress(pt);
}

ElevationInfo BookmarkManager::MakeElevationInfo(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const track = GetTrack(trackId);
  CHECK(track != nullptr, ());

  return ElevationInfo(*track);
}

void BookmarkManager::UpdateElevationMyPosition(kml::TrackId const & trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  static_assert(TrackSelectionMark::kInvalidDistance < 0, "");
  double myPositionDistance = TrackSelectionMark::kInvalidDistance;
  if (m_myPositionMark->HasPosition())
  {
    double const kEps = 1e-5;
    if (m_lastElevationMyPosition.EqualDxDy(m_myPositionMark->GetPivot(), kEps))
      return;
    m_lastElevationMyPosition = m_myPositionMark->GetPivot();

    auto const snapRect = mercator::RectByCenterXYAndSizeInMeters(m_myPositionMark->GetPivot(),
                                                                  kMyPositionTrackSnapInMeters);
    auto const selectionInfo = FindNearestTrack(
      snapRect, [trackId](Track const *track) { return track->GetId() == trackId; });
    if (selectionInfo.m_trackId == trackId)
      myPositionDistance = selectionInfo.m_distanceInMeters;
  }
  else
  {
    m_lastElevationMyPosition = m2::PointD::Zero();
  }

  auto const markId = GetTrackSelectionMarkId(trackId);
  auto es = GetEditSession();
  auto trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);

  double const kEpsMeters = 1e-2;
  if (!base::AlmostEqualAbs(trackSelectionMark->GetMyPositionDistance(),
                            myPositionDistance, kEpsMeters))
  {
    trackSelectionMark->SetMyPositionDistance(myPositionDistance);
    if (m_elevationMyPositionChanged)
      m_elevationMyPositionChanged();
  }
}

double BookmarkManager::GetElevationMyPosition(kml::TrackId const & trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const markId = GetTrackSelectionMarkId(trackId);
  CHECK(markId != kml::kInvalidMarkId, ());

  auto const trackSelectionMark = GetMark<TrackSelectionMark>(markId);
  return trackSelectionMark->GetMyPositionDistance();
}

void BookmarkManager::SetElevationMyPositionChangedCallback(
    ElevationMyPositionChangedCallback const & cb)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_elevationMyPositionChanged = cb;
}

void BookmarkManager::SetElevationActivePoint(kml::TrackId const & trackId, double targetDistance)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const track = GetTrack(trackId);
  CHECK(track != nullptr, ());

  m2::PointD pt;
  VERIFY(track->GetPoint(targetDistance, pt), (trackId, targetDistance));

  SetTrackSelectionInfo(TrackSelectionInfo(trackId, pt, targetDistance),
                        false /* notifyListeners */);

  m_drapeEngine.SafeCall(&df::DrapeEngine::SelectObject,
                         df::SelectionShape::ESelectedObject::OBJECT_TRACK, pt, FeatureID(),
                         false /* isAnim */, false /* isGeometrySelectionAllowed */,
                         true /* isSelectionShapeVisible */);
}

double BookmarkManager::GetElevationActivePoint(kml::TrackId const & trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const markId = GetTrackSelectionMarkId(trackId);
  CHECK(markId != kml::kInvalidMarkId, ());

  auto const trackSelectionMark = GetMark<TrackSelectionMark>(markId);
  return trackSelectionMark->GetDistance();
}

void BookmarkManager::SetElevationActivePointChangedCallback(ElevationActivePointChangedCallback const & cb)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_elevationActivePointChanged = cb;
}

BookmarkManager::TrackSelectionInfo BookmarkManager::FindNearestTrack(
    m2::RectD const & touchRect, TracksFilter const & tracksFilter) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  TrackSelectionInfo selectionInfo;

  auto minSquaredDist = std::numeric_limits<double>::max();
  for (auto const & pair : m_categories)
  {
    auto const & category = *pair.second;
    if (!category.IsVisible())
      continue;

    for (auto trackId : category.GetUserLines())
    {
      auto const track = GetTrack(trackId);
      if (!track->IsInteractive() || (tracksFilter && !tracksFilter(track)))
        continue;

      auto const trackRect = track->GetLimitRect();

      if (!trackRect.IsIntersect(touchRect))
        continue;

      auto const & pointsWithAlt = track->GetPointsWithAltitudes();
      for (size_t i = 0; i + 1 < pointsWithAlt.size(); ++i)
      {
        auto pt1 = pointsWithAlt[i].GetPoint();
        auto pt2 = pointsWithAlt[i + 1].GetPoint();
        if (!m2::Intersect(touchRect, pt1, pt2))
          continue;

        m2::ParametrizedSegment<m2::PointD> seg(pt1, pt2);
        auto const closestPoint = seg.ClosestPointTo(touchRect.Center());
        auto const squaredDist = closestPoint.SquaredLength(touchRect.Center());
        if (squaredDist >= minSquaredDist)
          continue;

        minSquaredDist = squaredDist;
        selectionInfo.m_trackId = trackId;
        selectionInfo.m_trackPoint = closestPoint;

        auto const segDistInMeters = mercator::DistanceOnEarth(pointsWithAlt[i].GetPoint(),
                                                               closestPoint);
        selectionInfo.m_distanceInMeters = segDistInMeters + track->GetLengthMeters(i);
      }
    }
  }

  return selectionInfo;
}

BookmarkManager::TrackSelectionInfo BookmarkManager::GetTrackSelectionInfo(kml::TrackId const & trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const markId = GetTrackSelectionMarkId(trackId);
  if (markId == kml::kInvalidMarkId)
    return {};

  auto const mark = GetMark<TrackSelectionMark>(markId);
  return TrackSelectionInfo(trackId, mark->GetPivot(), mark->GetDistance());
}

kml::MarkId BookmarkManager::GetTrackSelectionMarkId(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK_NOT_EQUAL(trackId, kml::kInvalidTrackId, ());

  for (auto markId : GetUserMarkIds(UserMark::Type::TRACK_SELECTION))
  {
    auto const * mark = GetMark<TrackSelectionMark>(markId);
    if (mark->GetTrackId() == trackId)
      return markId;
  }
  return kml::kInvalidMarkId;
}

int BookmarkManager::GetTrackSelectionMarkMinZoom(kml::TrackId trackId) const
{
  auto track = GetTrack(trackId);
  CHECK(track != nullptr, ());

  auto const zoom = std::min(df::GetDrawTileScale(track->GetLimitRect()), 14);
  return zoom;
}

void BookmarkManager::SetTrackSelectionMark(kml::TrackId trackId, m2::PointD const & pt,
                                            double distance)
{
  auto const markId = GetTrackSelectionMarkId(trackId);

  TrackSelectionMark * trackSelectionMark = nullptr;
  if (markId == kml::kInvalidMarkId)
  {
    trackSelectionMark = CreateUserMark<TrackSelectionMark>(pt);
    trackSelectionMark->SetTrackId(trackId);

    if (m_drapeEngine)
      trackSelectionMark->SetMinVisibleZoom(GetTrackSelectionMarkMinZoom(trackId));
  }
  else
  {
    trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
    trackSelectionMark->SetPosition(pt);
  }
  trackSelectionMark->SetDistance(distance);

  auto const isVisible = IsVisible(GetTrack(trackId)->GetGroupId());
  trackSelectionMark->SetIsVisible(isVisible);
}

void BookmarkManager::DeleteTrackSelectionMark(kml::TrackId trackId)
{
  if (trackId == m_selectedTrackId)
    m_selectedTrackId = kml::kInvalidTrackId;

  auto const markId = GetTrackSelectionMarkId(trackId);
  if (markId != kml::kInvalidMarkId)
    DeleteUserMark(markId);

  ResetTrackInfoMark(trackId);
}

void BookmarkManager::SetTrackInfoMark(kml::TrackId trackId, m2::PointD const & pt)
{
  auto trackInfoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  trackInfoMark->SetPosition(pt);
  auto const isVisible = IsVisible(GetTrack(trackId)->GetGroupId());
  trackInfoMark->SetIsVisible(isVisible);
  trackInfoMark->SetTrackId(trackId);
}

void BookmarkManager::ResetTrackInfoMark(kml::TrackId trackId)
{
  auto trackInfoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  if (trackInfoMark->GetTrackId() == trackId)
  {
    trackInfoMark->SetPosition(m2::PointD::Zero());
    trackInfoMark->SetIsVisible(false);
    trackInfoMark->SetTrackId(kml::kInvalidTrackId);
  }
}

void BookmarkManager::SetTrackSelectionInfo(TrackSelectionInfo const & trackSelectionInfo,
                                            bool notifyListeners)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK_NOT_EQUAL(trackSelectionInfo.m_trackId, kml::kInvalidTrackId, ());

  auto es = GetEditSession();
  auto const markId = GetTrackSelectionMarkId(trackSelectionInfo.m_trackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  trackSelectionMark->SetPosition(trackSelectionInfo.m_trackPoint);
  trackSelectionMark->SetDistance(trackSelectionInfo.m_distanceInMeters);

  if (notifyListeners && m_elevationActivePointChanged != nullptr)
    m_elevationActivePointChanged();
}

void BookmarkManager::SetDefaultTrackSelection(kml::TrackId trackId, bool showInfoSign)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto track = GetTrack(trackId);
  CHECK(track != nullptr, ());
  CHECK(track->IsInteractive(), ());

  auto const & points = track->GetPointsWithAltitudes();
  auto const pt = points[points.size() / 2].GetPoint();
  auto const distance = track->GetLengthMeters(points.size() / 2);

  auto es = GetEditSession();
  if (showInfoSign)
    SetTrackInfoMark(trackId, pt);
  SetTrackSelectionMark(trackId, pt, distance);
}

void BookmarkManager::OnTrackSelected(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto es = GetEditSession();
  ResetTrackInfoMark(trackId);

  auto const markId = GetTrackSelectionMarkId(trackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto * trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  trackSelectionMark->SetIsVisible(false);

  m_selectedTrackId = trackId;
}

void BookmarkManager::OnTrackDeselected()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_selectedTrackId == kml::kInvalidTrackId)
    return;

  auto const markId = GetTrackSelectionMarkId(m_selectedTrackId);
  CHECK_NOT_EQUAL(markId, kml::kInvalidMarkId, ());

  auto es = GetEditSession();
  auto * trackSelectionMark = GetMarkForEdit<TrackSelectionMark>(markId);
  auto const isVisible = IsVisible(GetTrack(m_selectedTrackId)->GetGroupId());
  trackSelectionMark->SetIsVisible(isVisible);

  m_selectedTrackId = kml::kInvalidTrackId;
}

kml::GroupIdCollection BookmarkManager::GetChildrenCategories(kml::MarkGroupId parentId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetCompilationOfType(parentId, kml::CompilationType::Category);
}

kml::GroupIdCollection BookmarkManager::GetChildrenCollections(kml::MarkGroupId parentId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetCompilationOfType(parentId, kml::CompilationType::Collection);
}

kml::GroupIdCollection BookmarkManager::GetCompilationOfType(kml::MarkGroupId parentId,
                                                             kml::CompilationType type) const
{
  kml::GroupIdCollection result;
  auto const & compilations = GetCategoryData(parentId).m_compilationIds;
  std::copy_if(compilations.cbegin(), compilations.cend(), std::back_inserter(result),
               [this, type](auto const groupId)
               {
                 auto const compilation = m_compilations.find(groupId);
                 CHECK(compilation != m_compilations.end(), ());
                 auto const & child = *compilation->second;
                 return child.GetCategoryData().m_type == type;
               });

  return result;
}

bool BookmarkManager::IsCompilation(kml::MarkGroupId id) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_compilations.find(id) != m_compilations.cend();
}

kml::CompilationType BookmarkManager::GetCompilationType(kml::MarkGroupId id) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const compilation = m_compilations.find(id);
  CHECK(compilation != m_compilations.cend(), ());
  return compilation->second->GetCategoryData().m_type;
}

void BookmarkManager::PrepareBookmarksAddresses(std::vector<SortBookmarkData> & bookmarksForSort,
                                                AddressesCollection & newAddresses)
{
  CHECK(m_regionAddressGetter != nullptr, ());

  for (auto & markData : bookmarksForSort)
  {
    if (!markData.m_address.IsValid())
    {
      markData.m_address = m_regionAddressGetter->GetNearbyRegionAddress(markData.m_point);
      if (markData.m_address.IsValid())
        newAddresses.emplace_back(markData.m_id, markData.m_address);
    }
  }
}

void BookmarkManager::FilterInvalidData(SortedBlocksCollection & sortedBlocks,
                                        AddressesCollection & newAddresses) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto & block : sortedBlocks)
  {
    FilterInvalidBookmarks(block.m_markIds);
    FilterInvalidTracks(block.m_trackIds);
  }

  base::EraseIf(sortedBlocks, [](SortedBlock const & block)
                              {
                                return block.m_trackIds.empty() && block.m_markIds.empty();
                              });

  base::EraseIf(newAddresses, [this](std::pair<kml::MarkId,
                                               search::ReverseGeocoder::RegionAddress> const & item)
                              {
                                return GetBookmark(item.first) == nullptr;
                              });
}

void BookmarkManager::SetBookmarksAddresses(AddressesCollection const & addresses)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto session = GetEditSession();
  for (auto const & item : addresses)
  {
    // Use inner method GetBookmarkForEdit to save address even if the bookmarks is not editable.
    auto bm = GetBookmarkForEdit(item.first);
    bm->SetAddress(item.second);
  }
}

void BookmarkManager::AddTracksSortedBlock(std::vector<SortTrackData> const & sortedTracks,
                                           SortedBlocksCollection & sortedBlocks) const
{
  if (!sortedTracks.empty())
  {
    SortedBlock tracksBlock;
    tracksBlock.m_blockName = GetTracksSortedBlockName();
    tracksBlock.m_trackIds.reserve(sortedTracks.size());
    for (auto const & track : sortedTracks)
      tracksBlock.m_trackIds.push_back(track.m_id);
    sortedBlocks.emplace_back(std::move(tracksBlock));
  }
}

void BookmarkManager::SortTracksByTime(std::vector<SortTrackData> & tracks) const
{
  bool hasTimestamp = false;
  for (auto const & track : tracks)
  {
    if (!kml::IsEqual(track.m_timestamp, kml::Timestamp()))
    {
      hasTimestamp = true;
      break;
    }
  }

  if (!hasTimestamp)
    return;

  std::sort(tracks.begin(), tracks.end(),
            [](SortTrackData const & lbm, SortTrackData const & rbm)
            {
              return lbm.m_timestamp > rbm.m_timestamp;
            });
}

void BookmarkManager::SortByDistance(std::vector<SortBookmarkData> const & bookmarksForSort,
                                     std::vector<SortTrackData> const & tracksForSort,
                                     m2::PointD const & myPosition,
                                     SortedBlocksCollection & sortedBlocks)
{
  CHECK(m_regionAddressGetter != nullptr, ());

  AddTracksSortedBlock(tracksForSort, sortedBlocks);

  std::vector<std::pair<SortBookmarkData const *, double>> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
  {
    auto const distance = mercator::DistanceOnEarth(mark.m_point, myPosition);
    sortedMarks.push_back(std::make_pair(&mark, distance));
  }

  std::sort(sortedMarks.begin(), sortedMarks.end(),
            [](std::pair<SortBookmarkData const *, double> const & lbm,
               std::pair<SortBookmarkData const *, double> const & rbm)
            {
              return lbm.second < rbm.second;
            });

  std::map<search::ReverseGeocoder::RegionAddress, size_t> regionBlockIndices;
  SortedBlock othersBlock;
  for (auto markItem : sortedMarks)
  {
    auto const & mark = *markItem.first;
    auto const distance = markItem.second;

    if (!mark.m_address.IsValid())
    {
      othersBlock.m_markIds.push_back(mark.m_id);
      continue;
    }

    auto const currentRegion =
      distance < kNearDistanceInMeters ? search::ReverseGeocoder::RegionAddress() : mark.m_address;

    size_t blockIndex;
    auto const it = regionBlockIndices.find(currentRegion);
    if (it == regionBlockIndices.end())
    {
      SortedBlock regionBlock;
      if (currentRegion.IsValid())
        regionBlock.m_blockName = m_regionAddressGetter->GetLocalizedRegionAddress(currentRegion);
      else
        regionBlock.m_blockName = GetNearMeSortedBlockName();

      blockIndex = sortedBlocks.size();
      regionBlockIndices[currentRegion] = blockIndex;
      sortedBlocks.push_back(regionBlock);
    }
    else
    {
      blockIndex = it->second;
    }

    sortedBlocks[blockIndex].m_markIds.push_back(mark.m_id);
  }

  if (!othersBlock.m_markIds.empty())
  {
    othersBlock.m_blockName = GetOthersSortedBlockName();
    sortedBlocks.emplace_back(std::move(othersBlock));
  }
}

void BookmarkManager::SortByTime(std::vector<SortBookmarkData> const & bookmarksForSort,
                                 std::vector<SortTrackData> const & tracksForSort,
                                 SortedBlocksCollection & sortedBlocks) const
{
  std::vector<SortTrackData> sortedTracks = tracksForSort;
  SortTracksByTime(sortedTracks);
  AddTracksSortedBlock(sortedTracks, sortedBlocks);

  std::vector<SortBookmarkData const *> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
    sortedMarks.push_back(&mark);

  std::sort(sortedMarks.begin(), sortedMarks.end(),
            [](SortBookmarkData const * lbm, SortBookmarkData const * rbm)
            {
              return lbm->m_timestamp > rbm->m_timestamp;
            });

  auto const currentTime = kml::TimestampClock::now();

  std::optional<SortedByTimeBlockType> lastBlockType;
  SortedBlock currentBlock;
  for (auto mark : sortedMarks)
  {
    auto currentBlockType = SortedByTimeBlockType::Others;
    if (mark->m_timestamp != kml::Timestamp())
      currentBlockType = GetSortedByTimeBlockType(currentTime - mark->m_timestamp);

    if (!lastBlockType)
    {
      lastBlockType = currentBlockType;
      currentBlock.m_blockName = GetSortedByTimeBlockName(currentBlockType);
    }

    if (currentBlockType != *lastBlockType)
    {
      sortedBlocks.push_back(currentBlock);
      currentBlock = SortedBlock();
      currentBlock.m_blockName = GetSortedByTimeBlockName(currentBlockType);
    }
    lastBlockType = currentBlockType;
    currentBlock.m_markIds.push_back(mark->m_id);
  }
  if (!currentBlock.m_markIds.empty())
    sortedBlocks.push_back(currentBlock);
}

void BookmarkManager::SortByType(std::vector<SortBookmarkData> const & bookmarksForSort,
                                 std::vector<SortTrackData> const & tracksForSort,
                                 SortedBlocksCollection & sortedBlocks) const
{
  AddTracksSortedBlock(tracksForSort, sortedBlocks);

  std::vector<SortBookmarkData const *> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
    sortedMarks.push_back(&mark);

  std::sort(sortedMarks.begin(), sortedMarks.end(),
            [](SortBookmarkData const * lbm, SortBookmarkData const * rbm)
            {
              return lbm->m_timestamp > rbm->m_timestamp;
            });

  std::map<BookmarkBaseType, size_t> typesCount;
  size_t othersTypeMarksCount = 0;
  for (auto mark : sortedMarks)
  {
    auto const type = mark->m_type;
    if (type == BookmarkBaseType::None)
    {
      ++othersTypeMarksCount;
      continue;
    }

    ++typesCount[type];
  }

  std::vector<std::pair<BookmarkBaseType, size_t>> sortedTypes;
  for (auto const & typeCount : typesCount)
  {
    if (typeCount.second < kMinCommonTypesCount && typeCount.first != BookmarkBaseType::Hotel)
      othersTypeMarksCount += typeCount.second;
    else
      sortedTypes.push_back(typeCount);
  }

  std::sort(sortedTypes.begin(), sortedTypes.end(),
            [](std::pair<BookmarkBaseType, size_t> const & l,
               std::pair<BookmarkBaseType, size_t> const & r){ return l.second > r.second; });

  std::map<BookmarkBaseType, size_t> blockIndices;
  sortedBlocks.reserve(sortedBlocks.size() + sortedTypes.size() + (othersTypeMarksCount > 0 ? 1 : 0));
  for (size_t i = 0; i < sortedTypes.size(); ++i)
  {
    auto const type = sortedTypes[i].first;
    SortedBlock typeBlock;
    typeBlock.m_blockName = GetLocalizedBookmarkBaseType(type);
    typeBlock.m_markIds.reserve(sortedTypes[i].second);
    blockIndices[type] = sortedBlocks.size();
    sortedBlocks.emplace_back(std::move(typeBlock));
  }

  if (othersTypeMarksCount > 0)
  {
    SortedBlock othersBlock;
    othersBlock.m_blockName = GetOthersSortedBlockName();
    othersBlock.m_markIds.reserve(othersTypeMarksCount);
    sortedBlocks.emplace_back(std::move(othersBlock));
  }

  for (auto mark : sortedMarks)
  {
    auto const type = mark->m_type;
    if (type == BookmarkBaseType::None ||
      (type != BookmarkBaseType::Hotel && typesCount[type] < kMinCommonTypesCount))
    {
      sortedBlocks.back().m_markIds.push_back(mark->m_id);
    }
    else
    {
      sortedBlocks[blockIndices[type]].m_markIds.push_back(mark->m_id);
    }
  }
}

void BookmarkManager::GetSortedCategoryImpl(SortParams const & params,
                                            std::vector<SortBookmarkData> const & bookmarksForSort,
                                            std::vector<SortTrackData> const & tracksForSort,
                                            SortedBlocksCollection & sortedBlocks)
{
  switch (params.m_sortingType)
  {
  case SortingType::ByDistance:
    CHECK(params.m_hasMyPosition, ());
    SortByDistance(bookmarksForSort, tracksForSort, params.m_myPosition, sortedBlocks);
    return;
  case SortingType::ByTime:
    SortByTime(bookmarksForSort, tracksForSort, sortedBlocks);
    return;
  case SortingType::ByType:
    SortByType(bookmarksForSort, tracksForSort, sortedBlocks);
    return;
  }
  UNREACHABLE();
}

void BookmarkManager::GetSortedCategory(SortParams const & params)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(params.m_onResults != nullptr, ());

  auto const * group = GetGroup(params.m_groupId);
  CHECK(group != nullptr, ());

  std::vector<SortBookmarkData> bookmarksForSort;
  bookmarksForSort.reserve(group->GetUserMarks().size());
  for (auto markId : group->GetUserMarks())
  {
    auto const * bm = GetBookmark(markId);
    bookmarksForSort.emplace_back(bm->GetData(), bm->GetAddress());
  }

  std::vector<SortTrackData> tracksForSort;
  tracksForSort.reserve(group->GetUserLines().size());
  for (auto trackId : group->GetUserLines())
  {
    auto const * track = GetTrack(trackId);
    tracksForSort.emplace_back(track->GetData());
  }

  if (m_testModeEnabled)
  {
    // Sort bookmarks synchronously.
    std::unique_lock<std::mutex> lock(m_regionAddressMutex);
    if (m_regionAddressGetter == nullptr)
    {
      LOG(LWARNING, ("Region address getter is not set, bookmarks sorting failed."));
      params.m_onResults({} /* sortedBlocks */, SortParams::Status::Cancelled);
      return;
    }
    AddressesCollection newAddresses;
    if (params.m_sortingType == SortingType::ByDistance)
      PrepareBookmarksAddresses(bookmarksForSort, newAddresses);

    SortedBlocksCollection sortedBlocks;
    GetSortedCategoryImpl(params, bookmarksForSort, tracksForSort, sortedBlocks);
    params.m_onResults(std::move(sortedBlocks), SortParams::Status::Completed);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Background,
                        [this, params, bookmarksForSort = std::move(bookmarksForSort),
                          tracksForSort = std::move(tracksForSort)]() mutable
  {
    std::unique_lock<std::mutex> lock(m_regionAddressMutex);
    if (m_regionAddressGetter == nullptr)
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [params]
      {
        LOG(LWARNING, ("Region address getter is not set, bookmarks sorting failed."));
        params.m_onResults({} /* sortedBlocks */, SortParams::Status::Cancelled);
      });
      return;
    }

    AddressesCollection newAddresses;
    if (params.m_sortingType == SortingType::ByDistance)
      PrepareBookmarksAddresses(bookmarksForSort, newAddresses);

    SortedBlocksCollection sortedBlocks;
    GetSortedCategoryImpl(params, bookmarksForSort, tracksForSort, sortedBlocks);

    GetPlatform().RunTask(Platform::Thread::Gui, [this, params,
                                                  newAddresses = std::move(newAddresses),
                                                  sortedBlocks = std::move(sortedBlocks)]() mutable
    {
      FilterInvalidData(sortedBlocks, newAddresses);
      if (sortedBlocks.empty())
      {
        params.m_onResults({} /* sortedBlocks */, SortParams::Status::Cancelled);
        return;
      }
      SetBookmarksAddresses(newAddresses);
      params.m_onResults(std::move(sortedBlocks), SortParams::Status::Completed);
    });
  });
}

void BookmarkManager::ClearGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_compilations.count(groupId) == 0, ());

  auto * group = GetGroup(groupId);
  for (auto markId : group->GetUserMarks())
  {
    if (IsBookmarkCategory(groupId))
    {
      m_changesTracker.OnDetachBookmark(markId, groupId);
      m_bookmarks.erase(markId);
    }
    else
    {
      m_userMarks.erase(markId);
    }
    m_changesTracker.OnDeleteMark(markId);
  }
  for (auto trackId : group->GetUserLines())
  {
    DeleteTrackSelectionMark(trackId);
    m_changesTracker.OnDeleteLine(trackId);
    m_tracks.erase(trackId);
  }
  group->Clear();
}

std::string BookmarkManager::GetCategoryName(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  return category->GetName();
}

void BookmarkManager::SetCategoryDescription(kml::MarkGroupId categoryId, std::string const & desc)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  category->SetDescription(desc);
}

void BookmarkManager::SetCategoryName(kml::MarkGroupId categoryId, std::string const & name)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  category->SetName(name);
}

void BookmarkManager::SetCategoryTags(kml::MarkGroupId categoryId, std::vector<std::string> const & tags)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  category->SetTags(tags);
}

void BookmarkManager::SetCategoryAccessRules(kml::MarkGroupId categoryId, kml::AccessRules accessRules)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  category->SetAccessRules(accessRules);
}

void BookmarkManager::SetCategoryCustomProperty(kml::MarkGroupId categoryId, std::string const & key,
                                                std::string const & value)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  category->SetCustomProperty(key, value);
}

std::string BookmarkManager::GetCategoryFileName(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  return category->GetFileName();
}

m2::RectD BookmarkManager::GetCategoryRect(kml::MarkGroupId categoryId, bool addIconsSize) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());

  m2::RectD categoryRect;
  if (category->IsEmpty())
    return categoryRect;

  for (auto markId : category->GetUserMarks())
  {
    auto const bookmark = GetBookmark(markId);
    categoryRect.Add(bookmark->GetPivot());
  }
  if (addIconsSize && !category->GetUserMarks().empty())
  {
    double const coeff = m_maxBookmarkSymbolSize.y / (m_viewport.PixelRect().SizeY() - m_maxBookmarkSymbolSize.y);
    categoryRect.Add(categoryRect.LeftTop() + m2::PointD(0.0, categoryRect.SizeY() * coeff));
  }
  for (auto trackId : category->GetUserLines())
  {
    auto const track = GetTrack(trackId);
    categoryRect.Add(track->GetLimitRect());
  }

  return categoryRect;
}

kml::CategoryData const & BookmarkManager::GetCategoryData(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const category = GetBmCategory(categoryId);
  CHECK(category != nullptr, ());
  return category->GetCategoryData();
}

kml::MarkGroupId BookmarkManager::GetCategoryId(std::string const & name) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & category : m_categories)
  {
    if (category.second->GetName() == name)
      return category.first;
  }
  return kml::kInvalidMarkGroupId;
}

UserMark const * BookmarkManager::FindMarkInRect(kml::MarkGroupId groupId, m2::AnyRectD const & rect,
                                                 bool findOnlyVisible, double & d) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const * group = GetGroup(groupId);

  UserMark const * resMark = nullptr;
  if (group->IsVisible())
  {
    FindMarkFunctor f(&resMark, d, rect);
    for (auto markId : group->GetUserMarks())
    {
      auto const * mark = GetMark(markId);
      if (findOnlyVisible && !mark->IsVisible())
        continue;

      if (mark->IsAvailableForSearch() && rect.IsPointInside(mark->GetPivot()))
        f(mark);
    }
  }
  return resMark;
}

void BookmarkManager::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const group = GetGroup(groupId);
  if (group->IsVisible() != visible)
  {
    group->SetIsVisible(visible);
    if (auto const compilationIt = m_compilations.find(groupId); compilationIt != m_compilations.end())
    {
      auto const parentId = compilationIt->second->GetParentID();
      auto const parentGroup = GetBmCategory(parentId);
      CHECK(parentGroup, (parentId));
      parentGroup->SetDirty();
      if (visible)  // visible == false handled in InferVisibility
        parentGroup->SetIsVisible(true);
    }
  }
  UpdateTrackMarksVisibility(groupId);
}

bool BookmarkManager::IsVisible(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->IsVisible();
}

bool BookmarkManager::IsSearchAllowed(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(m_callbacks.m_getSearchAPI != nullptr, ());

  if (m_callbacks.m_getSearchAPI().IsIndexingOfBookmarkGroupEnabled(groupId))
    return true;

  size_t indexedBookmarksCount = 0;
  for (auto const indexableGroupId : m_callbacks.m_getSearchAPI().GetIndexableGroups())
  {
    auto const it = m_categories.find(indexableGroupId);
    if (it == m_categories.end())
      continue;
    indexedBookmarksCount += it->second->GetUserMarks().size();
  }
  auto const bookmarksCount = GetUserMarkIds(groupId).size();
  auto const maxCount = m_callbacks.m_getSearchAPI().GetMaximumPossibleNumberOfBookmarksToIndex();
  return indexedBookmarksCount + bookmarksCount <= maxCount;
}

void BookmarkManager::PrepareForSearch(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(m_callbacks.m_getSearchAPI != nullptr, ());
  m_callbacks.m_getSearchAPI().EnableIndexingOfBookmarkGroup(groupId, true /* enable */);
}

void BookmarkManager::UpdateTrackMarksMinZoom()
{
  auto const marksIds = GetUserMarkIds(UserMark::TRACK_SELECTION);
  for (auto markId : marksIds)
  {
    auto mark = GetMarkForEdit<TrackSelectionMark>(markId);
    mark->SetMinVisibleZoom(GetTrackSelectionMarkMinZoom(mark->GetTrackId()));
  }
}

void BookmarkManager::UpdateTrackMarksVisibility(kml::MarkGroupId groupId)
{
  auto const isVisible = IsVisible(groupId);
  auto const tracksIds = GetTrackIds(groupId);
  auto infoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
  for (auto trackId : tracksIds)
  {
    auto markId = GetTrackSelectionMarkId(trackId);
    if (markId == kml::kInvalidMarkId)
      continue;
    if (infoMark->GetTrackId() == trackId && infoMark->IsVisible())
      infoMark->SetIsVisible(isVisible);
    auto mark = GetMarkForEdit<TrackSelectionMark>(markId);
    mark->SetIsVisible(isVisible);
  }
}

void BookmarkManager::RequestSymbolSizes()
{
  std::vector<std::string> symbols;
  symbols.push_back(kLargestBookmarkSymbolName);
  symbols.push_back(TrackSelectionMark::GetInitialSymbolName());

  m_drapeEngine.SafeCall(
      &df::DrapeEngine::RequestSymbolsSize, symbols,
      [this](std::map<std::string, m2::PointF> && sizes) {
        GetPlatform().RunTask(Platform::Thread::Gui, [this, sizes = std::move(sizes)]() mutable {
          auto es = GetEditSession();
          auto infoMark = GetMarkForEdit<TrackInfoMark>(m_trackInfoMarkId);
          auto const & sz = sizes.at(TrackSelectionMark::GetInitialSymbolName());
          infoMark->SetOffset(m2::PointF(0.0, -sz.y / 2));
          m_maxBookmarkSymbolSize = sizes.at(kLargestBookmarkSymbolName);
          m_symbolSizesAcquired = true;
          if (m_onSymbolSizesAcquiredFn)
            m_onSymbolSizesAcquiredFn();
        });
      });
}

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_drapeEngine.Set(engine);
  m_firstDrapeNotification = true;

  auto es = GetEditSession();
  UpdateTrackMarksMinZoom();
  RequestSymbolSizes();
}

void BookmarkManager::InitRegionAddressGetter(DataSource const & dataSource,
                                              storage::CountryInfoGetter const & infoGetter)
{
  std::unique_lock<std::mutex> lock(m_regionAddressMutex);
  m_regionAddressGetter = std::make_unique<search::RegionAddressGetter>(dataSource, infoGetter);
}

void BookmarkManager::ResetRegionAddressGetter()
{
  std::unique_lock<std::mutex> lock(m_regionAddressMutex);
  m_regionAddressGetter.reset();
}

void BookmarkManager::UpdateViewport(ScreenBase const & screen)
{
  m_viewport = screen;
}

void BookmarkManager::SetBookmarksChangedCallback(BookmarksChangedCallback && callback)
{
  m_bookmarksChangedCallback = std::move(callback);
}

void BookmarkManager::SetCategoriesChangedCallback(CategoriesChangedCallback && callback)
{
  m_categoriesChangedCallback = std::move(callback);
}

void BookmarkManager::SetAsyncLoadingCallbacks(AsyncLoadingCallbacks && callbacks)
{
  m_asyncLoadingCallbacks = std::move(callbacks);
}

bool BookmarkManager::AreSymbolSizesAcquired(
    BookmarkManager::OnSymbolSizesAcquiredCallback && callback)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_symbolSizesAcquired)
    return true;
  m_onSymbolSizesAcquiredFn = std::move(callback);
  return false;
}

void BookmarkManager::Teardown()
{
  m_needTeardown = true;
}

Bookmark * BookmarkManager::AddBookmark(std::unique_ptr<Bookmark> && bookmark)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * bm = bookmark.get();
  auto const markId = bm->GetId();
  CHECK_EQUAL(m_bookmarks.count(markId), 0, ());
  m_bookmarks.emplace(markId, std::move(bookmark));
  m_changesTracker.OnAddMark(markId);
  return bm;
}

Track * BookmarkManager::AddTrack(std::unique_ptr<Track> && track)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * t = track.get();
  auto const trackId = t->GetId();
  CHECK_EQUAL(m_tracks.count(trackId), 0, ());
  m_tracks.emplace(trackId, std::move(track));
  m_changesTracker.OnAddLine(trackId);
  return t;
}

void BookmarkManager::SaveState() const
{
  settings::Set(kLastEditedBookmarkCategory, m_lastCategoryUrl);
  settings::Set(kLastEditedBookmarkColor, static_cast<uint32_t>(m_lastColor));
}

void BookmarkManager::LoadState()
{
  settings::TryGet(kLastEditedBookmarkCategory, m_lastCategoryUrl);

  uint32_t color;
  if (settings::Get(kLastEditedBookmarkColor, color) &&
      color > static_cast<uint32_t>(kml::PredefinedColor::None) &&
      color < static_cast<uint32_t>(kml::PredefinedColor::Count))
  {
    m_lastColor = static_cast<kml::PredefinedColor>(color);
  }
  else
  {
    m_lastColor = BookmarkCategory::GetDefaultColor();
  }
}

std::string BookmarkManager::GetMetadataEntryName(kml::MarkGroupId groupId) const
{
  CHECK(IsBookmarkCategory(groupId), ());
  return GetCategoryFileName(groupId);
}

void BookmarkManager::CleanupInvalidMetadata()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::set<std::string> activeEntries;
  for (auto const & cat : m_categories)
  {
    auto const entryName = GetMetadataEntryName(cat.first);
    if (!entryName.empty())
      activeEntries.insert(entryName);
  }

  auto it = m_metadata.m_entriesProperties.begin();
  while (it != m_metadata.m_entriesProperties.end())
  {
    if (activeEntries.find(it->first) == activeEntries.end() || it->second.m_values.empty())
      it = m_metadata.m_entriesProperties.erase(it);
    else
      ++it;
  }
}

void BookmarkManager::SaveMetadata()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CleanupInvalidMetadata();

  auto const metadataFilePath = base::JoinPath(GetPlatform().WritableDir(), kMetadataFileName);
  std::string jsonStr;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(jsonStr);
    coding::SerializerJson<Sink> ser(sink);
    ser(m_metadata);
  }

  try
  {
    FileWriter w(metadataFilePath);
    w.Write(jsonStr.c_str(), jsonStr.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", metadataFilePath, "reason:", exception.what()));
  }
}

void BookmarkManager::LoadMetadata()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const metadataFilePath = base::JoinPath(GetPlatform().WritableDir(), kMetadataFileName);
  if (!GetPlatform().IsFileExistsByFullPath(metadataFilePath))
    return;

  Metadata metadata;
  std::string jsonStr;
  try
  {
    {
      FileReader r(metadataFilePath);
      r.ReadAsString(jsonStr);
    }

    if (jsonStr.empty())
      return;

    coding::DeserializerJson des(jsonStr);
    des(metadata);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", metadataFilePath, "reason:", exception.what()));
    return;
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing file:", metadataFilePath, "reason:", exception.what(),
      "json:", jsonStr));
    return;
  }

  m_metadata = metadata;
}

void BookmarkManager::ClearCategories()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto groupId : m_bmGroupsIdList)
  {
    ClearGroup(groupId);
    m_changesTracker.OnDeleteGroup(groupId);
  }

  m_compilations.clear();
  m_categories.clear();
  m_bmGroupsIdList.clear();

  m_bookmarks.clear();
  m_tracks.clear();
}

BookmarkManager::KMLDataCollectionPtr BookmarkManager::LoadBookmarks(
    std::string const & dir, std::string const & ext, KmlFileType fileType,
    BookmarksChecker const & checker)
{
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, ext, files);

  auto collection = std::make_shared<KMLDataCollection>();
  collection->reserve(files.size());
  for (auto const & file : files)
  {
    auto const filePath = base::JoinPath(dir, file);
    auto kmlData = LoadKmlFile(filePath, fileType);
    if (kmlData == nullptr)
      continue;
    if (checker && !checker(*kmlData))
      continue;
    if (m_needTeardown)
      break;
    collection->emplace_back(filePath, std::move(kmlData));
  }
  return collection;
}

void BookmarkManager::LoadBookmarks()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ClearCategories();
  LoadMetadata();

  m_loadBookmarksFinished = false;
  NotifyAboutStartAsyncLoading();
  GetPlatform().RunTask(Platform::Thread::File, [this]()
  {
    auto collection = LoadBookmarks(GetBookmarksDirectory(), kKmlExtension, KmlFileType::Text,
                                    [](kml::FileData const &)
    {
      return true;  // Allow to load any files from the bookmarks directory.
    });

    if (m_needTeardown)
      return;
    NotifyAboutFinishAsyncLoading(std::move(collection));
  });

  LoadState();
}

void BookmarkManager::LoadBookmark(std::string const & filePath, bool isTemporaryFile)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  // Defer bookmark loading in case of another asynchronous process.
  if (!m_loadBookmarksFinished || m_asyncLoadingInProgress)
  {
    m_bookmarkLoadingQueue.emplace_back(filePath, isTemporaryFile);
    return;
  }

  NotifyAboutStartAsyncLoading();
  LoadBookmarkRoutine(filePath, isTemporaryFile);
}

void BookmarkManager::LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, filePath, isTemporaryFile]()
  {
    if (m_needTeardown)
      return;

    auto collection = std::make_shared<KMLDataCollection>();

    std::string fileSavePath = GetKMLPath(filePath);
    if (!fileSavePath.empty())
    {
      auto kmlData = LoadKmlFile(fileSavePath, KmlFileType::Text);
      if (m_needTeardown)
        return;

      if (kmlData)
      {
        base::DeleteFileX(fileSavePath);
        fileSavePath = GenerateValidAndUniqueFilePathForKML(base::GetNameFromFullPathWithoutExt(std::move(fileSavePath)));

        if (!SaveKmlFileSafe(*kmlData, fileSavePath, KmlFileType::Text))
          base::DeleteFileX(fileSavePath);
        else
          collection->emplace_back(std::move(fileSavePath), std::move(kmlData));
      }
    }

    if (m_needTeardown)
      return;

    NotifyAboutFile(!collection->empty() /* success */, filePath, isTemporaryFile);

    UpdateLastModifiedTime(*collection);
    NotifyAboutFinishAsyncLoading(std::move(collection));
  });
}

void BookmarkManager::NotifyAboutStartAsyncLoading()
{
  if (m_needTeardown)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this]()
  {
    m_asyncLoadingInProgress = true;
    if (m_asyncLoadingCallbacks.m_onStarted != nullptr)
      m_asyncLoadingCallbacks.m_onStarted();
  });
}

void BookmarkManager::NotifyAboutFinishAsyncLoading(KMLDataCollectionPtr && collection)
{
  if (m_needTeardown)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, collection]()
  {
    if (!collection->empty())
    {
      CreateCategories(std::move(*collection));
    }
    else if (!m_loadBookmarksFinished)
    {
      CheckAndResetLastIds();
      CheckAndCreateDefaultCategory();
    }

    m_loadBookmarksFinished = true;

    if (!m_bookmarkLoadingQueue.empty())
    {
      ASSERT(m_asyncLoadingInProgress, ());
      LoadBookmarkRoutine(m_bookmarkLoadingQueue.front().m_filename,
                          m_bookmarkLoadingQueue.front().m_isTemporaryFile);
      m_bookmarkLoadingQueue.pop_front();
    }
    else
    {
      m_asyncLoadingInProgress = false;
      if (m_asyncLoadingCallbacks.m_onFinished != nullptr)
        m_asyncLoadingCallbacks.m_onFinished();
    }
  });
}

void BookmarkManager::NotifyAboutFile(bool success, std::string const & filePath,
                                      bool isTemporaryFile)
{
  if (m_needTeardown)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, success, filePath, isTemporaryFile]()
  {
    if (success)
    {
      if (m_asyncLoadingCallbacks.m_onFileSuccess != nullptr)
        m_asyncLoadingCallbacks.m_onFileSuccess(filePath, isTemporaryFile);
    }
    else
    {
      if (m_asyncLoadingCallbacks.m_onFileError != nullptr)
        m_asyncLoadingCallbacks.m_onFileError(filePath, isTemporaryFile);
    }
  });
}

void BookmarkManager::MoveBookmark(kml::MarkId bmID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  DetachBookmark(bmID, curGroupID);
  AttachBookmark(bmID, newGroupID);

  SetLastEditedBmCategory(newGroupID);
}

void BookmarkManager::UpdateBookmark(kml::MarkId bmID, kml::BookmarkData const & bm)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * bookmark = GetBookmarkForEdit(bmID);

  auto const prevColor = bookmark->GetColor();
  bookmark->SetData(bm);
  ASSERT(bookmark->GetGroupId() != kml::kInvalidMarkGroupId, ());

  if (prevColor != bookmark->GetColor())
    SetLastEditedBmColor(bookmark->GetColor());
}

kml::MarkGroupId BookmarkManager::LastEditedBMCategory()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (HasBmCategory(m_lastEditedGroupId))
    return m_lastEditedGroupId;

  for (auto & cat : m_categories)
  {
    if (cat.second->GetFileName() == m_lastCategoryUrl)
    {
      m_lastEditedGroupId = cat.first;
      return m_lastEditedGroupId;
    }
  }
  m_lastEditedGroupId = CheckAndCreateDefaultCategory();
  return m_lastEditedGroupId;
}

kml::PredefinedColor BookmarkManager::LastEditedBMColor() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return (m_lastColor != kml::PredefinedColor::None ? m_lastColor : BookmarkCategory::GetDefaultColor());
}

void BookmarkManager::SetLastEditedBmCategory(kml::MarkGroupId groupId)
{
  m_lastEditedGroupId = groupId;
  m_lastCategoryUrl = GetBmCategory(groupId)->GetFileName();
  SaveState();
}

void BookmarkManager::SetLastEditedBmColor(kml::PredefinedColor color)
{
  m_lastColor = color;
  SaveState();
}

BookmarkCategory const * BookmarkManager::GetBmCategory(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());

  auto const compilationIt = m_compilations.find(categoryId);
  if (compilationIt != m_compilations.cend())
    return compilationIt->second.get();

  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

BookmarkCategory * BookmarkManager::GetBmCategory(kml::MarkGroupId categoryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());

  auto const compilationIt = m_compilations.find(categoryId);
  if (compilationIt != m_compilations.cend())
    return compilationIt->second.get();

  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

void BookmarkManager::GetBookmarksInfo(kml::MarkIdSet const & marks, std::vector<BookmarkInfo> & bookmarksInfo)
{
  bookmarksInfo.clear();
  bookmarksInfo.reserve(marks.size());
  for (auto markId : marks)
  {
    if (IsBookmark(markId))
    {
      auto const * bm = GetBookmark(markId);
      bookmarksInfo.emplace_back(markId, bm->GetData(), bm->GetAddress());
    }
  }
}

void BookmarkManager::GetBookmarkGroupsInfo(MarksChangesTracker::GroupMarkIdSet const & groups,
                                            std::vector<BookmarkGroupInfo> & groupsInfo)
{
  groupsInfo.clear();
  groupsInfo.reserve(groups.size());
  for (auto const & groupMarks : groups)
  {
    auto const & markIds = groupMarks.second;
    groupsInfo.emplace_back(groupMarks.first, kml::MarkIdCollection(markIds.begin(), markIds.end()));
  }
}

void BookmarkManager::SendBookmarksChanges(MarksChangesTracker const & changesTracker)
{
  std::vector<BookmarkInfo> bookmarksInfo;

  if (m_callbacks.m_createdBookmarksCallback != nullptr)
  {
    GetBookmarksInfo(changesTracker.GetCreatedMarkIds(), bookmarksInfo);
    if (!bookmarksInfo.empty())
      m_callbacks.m_createdBookmarksCallback(bookmarksInfo);
  }

  if (m_callbacks.m_updatedBookmarksCallback != nullptr)
  {
    GetBookmarksInfo(changesTracker.GetUpdatedMarkIds(), bookmarksInfo);
    if (!bookmarksInfo.empty())
      m_callbacks.m_updatedBookmarksCallback(bookmarksInfo);
  }

  std::vector<BookmarkGroupInfo> groupsInfo;

  if (m_callbacks.m_attachedBookmarksCallback != nullptr)
  {
    GetBookmarkGroupsInfo(changesTracker.GetAttachedBookmarks(), groupsInfo);
    if (!groupsInfo.empty())
      m_callbacks.m_attachedBookmarksCallback(groupsInfo);
  }

  if (m_callbacks.m_detachedBookmarksCallback != nullptr)
  {
    GetBookmarkGroupsInfo(changesTracker.GetDetachedBookmarks(), groupsInfo);
    if (!groupsInfo.empty())
      m_callbacks.m_detachedBookmarksCallback(groupsInfo);
  }

  if (m_callbacks.m_deletedBookmarksCallback != nullptr)
  {
    kml::MarkIdCollection bookmarkIds;
    auto const & removedIds = changesTracker.GetRemovedMarkIds();
    bookmarkIds.reserve(removedIds.size());
    for (auto markId : removedIds)
    {
      if (IsBookmark(markId))
        bookmarkIds.push_back(markId);
    }
    if (!bookmarkIds.empty())
      m_callbacks.m_deletedBookmarksCallback(bookmarkIds);
  }
}

void BookmarkManager::NotifyBookmarksChanged()
{
  if (m_bookmarksChangedCallback != nullptr)
    m_bookmarksChangedCallback();
}

void BookmarkManager::NotifyCategoriesChanged()
{
  if (m_categoriesChangedCallback != nullptr)
    m_categoriesChangedCallback();
}

bool BookmarkManager::HasBmCategory(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return (IsBookmarkCategory(groupId) && GetBmCategory(groupId) != nullptr);
}

void BookmarkManager::UpdateBmGroupIdList()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  size_t const count = m_categories.size();

  // All this routine to get sorted list by last modified time.

  using PairT = std::pair<kml::MarkGroupId, BookmarkCategory const *>;
  std::vector<PairT> vec;
  vec.reserve(count);
  for (auto it = m_categories.begin(); it != m_categories.end(); ++it)
    vec.emplace_back(it->first, it->second.get());

  std::sort(vec.begin(), vec.end(), [](PairT const & lhs, PairT const & rhs)
  {
    return lhs.second->GetLastModifiedTime() > rhs.second->GetLastModifiedTime();
  });

  m_bmGroupsIdList.clear();
  m_bmGroupsIdList.resize(count);
  for (size_t i = 0; i < count; ++i)
    m_bmGroupsIdList[i] = vec[i].first;
}

kml::MarkGroupId BookmarkManager::CreateBookmarkCategory(kml::CategoryData && data, bool autoSave /* = true */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // _group id_ is not persistant and assigned every time on loading from file.
  /// @todo Is there any reason to keep and update kLastBookmarkCategoryId in SecureStorage (settings).
  if (data.m_id == kml::kInvalidMarkGroupId)
    data.m_id = UserMarkIdStorage::Instance().GetNextCategoryId();
  auto groupId = data.m_id;

  CHECK_EQUAL(m_categories.count(groupId), 0, ());
  CHECK_EQUAL(m_compilations.count(groupId), 0, ());
  m_categories.emplace(groupId, std::make_unique<BookmarkCategory>(std::move(data), autoSave));
  UpdateBmGroupIdList();
  m_changesTracker.OnAddGroup(groupId);
  return groupId;
}

kml::MarkGroupId BookmarkManager::CreateBookmarkCategory(std::string const & name, bool autoSave)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const groupId = UserMarkIdStorage::Instance().GetNextCategoryId();
  CHECK_EQUAL(m_categories.count(groupId), 0, ());
  m_categories[groupId] = std::make_unique<BookmarkCategory>(name, groupId, autoSave);
  UpdateBmGroupIdList();
  m_changesTracker.OnAddGroup(groupId);
  NotifyBookmarksChanged();
  NotifyCategoriesChanged();
  return groupId;
}

BookmarkCategory * BookmarkManager::CreateBookmarkCompilation(kml::CategoryData && data)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (data.m_id == kml::kInvalidMarkGroupId)
    data.m_id = UserMarkIdStorage::Instance().GetNextCategoryId();
  auto groupId = data.m_id;
  CHECK_EQUAL(m_categories.count(groupId), 0, ());
  CHECK_EQUAL(m_compilations.count(groupId), 0, ());
  auto compilation = std::make_unique<BookmarkCategory>(std::move(data), false);
  auto result = compilation.get();
  m_compilations.emplace(groupId, std::move(compilation));

  return result;
}

kml::MarkGroupId BookmarkManager::CheckAndCreateDefaultCategory()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_categories.empty())
    return CreateBookmarkCategory(m_callbacks.m_getStringsBundle().GetString("core_my_places"));
  return m_categories.cbegin()->first;
}

void BookmarkManager::CheckAndResetLastIds()
{
  auto & idStorage = UserMarkIdStorage::Instance();
  if (m_categories.empty())
    idStorage.ResetCategoryId();
  if (m_bookmarks.empty())
    idStorage.ResetBookmarkId();
  if (m_tracks.empty())
    idStorage.ResetTrackId();
}

bool BookmarkManager::DeleteBmCategory(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_categories.find(groupId);
  if (it == m_categories.end())
    return false;

  ClearGroup(groupId);
  m_changesTracker.OnDeleteGroup(groupId);

  FileWriter::DeleteFileX(it->second->GetFileName());

  DeleteCompilations(it->second->GetCategoryData().m_compilationIds);
  m_categories.erase(it);
  UpdateBmGroupIdList();
  return true;
}

namespace
{
class BestUserMarkFinder
{
public:
  explicit BestUserMarkFinder(BookmarkManager::TTouchRectHolder const & rectHolder,
                              BookmarkManager::TFindOnlyVisibleChecker const & findOnlyVisible,
                              BookmarkManager const * manager)
    : m_rectHolder(rectHolder)
    , m_findOnlyVisible(findOnlyVisible)
    , m_d(std::numeric_limits<double>::max())
    , m_mark(nullptr)
    , m_manager(manager)
  {}

  bool operator()(kml::MarkGroupId groupId)
  {
    auto const groupType = BookmarkManager::GetGroupType(groupId);
    if (auto const * p = m_manager->FindMarkInRect(groupId, m_rectHolder(groupType), m_findOnlyVisible(groupType), m_d))
    {
      m_mark = p;
      return true;
    }
    return false;
  }

  UserMark const * GetFoundMark() const { return m_mark; }

private:
  BookmarkManager::TTouchRectHolder const m_rectHolder;
  BookmarkManager::TFindOnlyVisibleChecker const m_findOnlyVisible;
  double m_d;
  UserMark const * m_mark;
  BookmarkManager const * m_manager;
};
}  // namespace

UserMark const * BookmarkManager::FindNearestUserMark(m2::AnyRectD const & rect) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; }, [](UserMark::Type) { return false; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder,
                                                      TFindOnlyVisibleChecker const & findOnlyVisible) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Among the marks inside the rect (if any) finder stores the closest one to its center.
  BestUserMarkFinder finder(holder, findOnlyVisible, this);

  // For each type X in the condition, ordered by priority:
  //  - look for the closest mark among the marks of the same type X.
  //  - if the mark has been found, stop looking for a closer one in the other types.
  if (finder(UserMark::Type::ROUTING) ||
      finder(UserMark::Type::ROAD_WARNING) ||
      finder(UserMark::Type::SEARCH) ||
      finder(UserMark::Type::API))
  {
    return finder.GetFoundMark();
  }

  // Look for the closest bookmark.
  for (auto const & pair : m_categories)
    finder(pair.first);

  if (finder.GetFoundMark() != nullptr)
    return finder.GetFoundMark();

  // Look for the closest TRACK_INFO or TRACK_SELECTION mark.
  finder(UserMark::Type::TRACK_INFO);
  finder(UserMark::Type::TRACK_SELECTION);

  return finder.GetFoundMark();
}

UserMarkLayer const * BookmarkManager::GetGroup(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
  {
    CHECK_GREATER(groupId, 0, ());
    return m_userMarkLayers[static_cast<size_t>(groupId - 1)].get();
  }

  auto const compilationIt = m_compilations.find(groupId);
  if (compilationIt != m_compilations.cend())
    return compilationIt->second.get();

  ASSERT(m_categories.find(groupId) != m_categories.end(), ());
  return m_categories.at(groupId).get();
}

UserMarkLayer * BookmarkManager::GetGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
  {
    CHECK_GREATER(groupId, 0, ());
    return m_userMarkLayers[static_cast<size_t>(groupId - 1)].get();
  }

  auto const compilationIt = m_compilations.find(groupId);
  if (compilationIt != m_compilations.cend())
    return compilationIt->second.get();

  auto const it = m_categories.find(groupId);
  return it != m_categories.end() ? it->second.get() : nullptr;
}

void BookmarkManager::UpdateLastModifiedTime(KMLDataCollection & collection)
{
  for (auto const & [_, c] : collection)
    c->m_categoryData.m_lastModified = kml::TimestampClock::now();
}

void BookmarkManager::CreateCategories(KMLDataCollection && dataCollection, bool autoSave)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  kml::GroupIdSet loadedGroups;

  for (auto const & data : dataCollection)
  {
    auto const & fileName = data.first;
    auto & fileData = *data.second;
    auto & categoryData = fileData.m_categoryData;

    if (!UserMarkIdStorage::Instance().CheckIds(fileData) || HasDuplicatedIds(fileData))
    {
      LOG(LINFO, ("Reset bookmark ids in the file", fileName));
      //TODO: notify subscribers(like search subsystem). This KML could have been indexed.
      ResetIds(fileData);
    }

    std::unordered_map<kml::CompilationId, BookmarkCategory *> compilations;
    std::unordered_set<std::string> compilationNames;
    for (auto & compilation : fileData.m_compilationsData)
    {
      SetUniqueName(compilation, [&compilationNames](auto const & name)
      {
        return compilationNames.count(name) == 0;
      });

      auto const compilationId = compilation.m_compilationId;
      auto childGroup = CreateBookmarkCompilation(std::move(compilation));
      categoryData.m_compilationIds.push_back(childGroup->GetID());

      compilations.emplace(compilationId, childGroup);
      compilationNames.emplace(childGroup->GetName());
      childGroup->SetFileName(fileName);
      childGroup->SetServerId(fileData.m_serverId);
    }

    SetUniqueName(categoryData, [this](auto const & name) { return !IsUsedCategoryName(name); });

    UserMarkIdStorage::Instance().EnableSaving(false);

    bool const saveAfterCreation = autoSave && (categoryData.m_id == kml::kInvalidMarkGroupId);
    auto const groupId = CreateBookmarkCategory(std::move(categoryData), saveAfterCreation);
    loadedGroups.insert(groupId);
    auto * group = GetBmCategory(groupId);
    group->SetFileName(fileName);
    group->SetServerId(fileData.m_serverId);

    // Restore sensitive info from the cache.
    auto const cacheIt = m_restoringCache.find(fileName);
    if (cacheIt != m_restoringCache.end() &&
        (group->GetServerId().empty() || group->GetServerId() == cacheIt->second.m_serverId) &&
        cacheIt->second.m_accessRules != group->GetCategoryData().m_accessRules)
    {
      group->SetServerId(cacheIt->second.m_serverId);
      group->SetAccessRules(cacheIt->second.m_accessRules);
      group->EnableAutoSave(autoSave);
    }

    for (auto const & [compilationId, compilation] : compilations)
    {
      UNUSED_VALUE(compilationId);
      compilation->SetParentId(groupId);
      auto const & categoryData = group->GetCategoryData();
      compilation->SetAccessRules(categoryData.m_accessRules);
      compilation->SetAuthor(categoryData.m_authorName, categoryData.m_authorId);
    }

    for (auto & bmData : fileData.m_bookmarksData)
    {
      auto const compilationIds = bmData.m_compilations;
      auto * bm = CreateBookmark(std::move(bmData));
      bm->Attach(groupId);
      group->m_userMarks.insert(bm->GetId());
      for (auto const c : compilationIds)
      {
        auto const it = compilations.find(c);
        if (it == compilations.end())
        {
          LOG(LERROR, ("Incorrect compilation id", c, "into", fileName));
          continue;
        }
        bm->AttachCompilation(it->second->GetID());
        it->second->AttachUserMark(bm->GetId());
      }
      m_changesTracker.OnAttachBookmark(bm->GetId(), groupId);
    }
    for (auto & trackData : fileData.m_tracksData)
    {
      auto track = std::make_unique<Track>(std::move(trackData), group->HasElevationProfile());
      auto * t = AddTrack(std::move(track));
      t->Attach(groupId);
      group->m_tracks.insert(t->GetId());
      if (t->IsInteractive())
        SetDefaultTrackSelection(t->GetId(), false /* showInfoSign */);
    }
    UpdateTrackMarksVisibility(groupId);
    UserMarkIdStorage::Instance().EnableSaving(true);
  }
  m_restoringCache.clear();

  NotifyChanges();

  for (auto const & groupId : loadedGroups)
  {
    auto * group = GetBmCategory(groupId);
    group->EnableAutoSave(autoSave);
  }
}

bool BookmarkManager::HasDuplicatedIds(kml::FileData const & fileData) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (fileData.m_categoryData.m_id != kml::kInvalidMarkGroupId &&
      m_categories.find(fileData.m_categoryData.m_id) != m_categories.cend())
  {
    return true;
  }

  for (auto const & b : fileData.m_bookmarksData)
  {
    if (b.m_id != kml::kInvalidMarkId && m_bookmarks.count(b.m_id) > 0)
      return true;
  }

  for (auto const & t : fileData.m_tracksData)
  {
    if (t.m_id != kml::kInvalidTrackId && m_tracks.count(t.m_id) > 0)
      return true;
  }

  for (auto const & c : fileData.m_compilationsData)
  {
    if (c.m_id != kml::kInvalidMarkGroupId &&
        (m_categories.find(c.m_id) != m_categories.cend() ||
         m_compilations.find(c.m_id) != m_compilations.cend()))
    {
      return true;
    }
  }

  return false;
}

template <typename UniquityChecker>
void BookmarkManager::SetUniqueName(kml::CategoryData & data, UniquityChecker checker)
{
  auto originalName = kml::GetDefaultStr(data.m_name);
  if (originalName.empty())
  {
    originalName = kDefaultBookmarksFileName;
    kml::SetDefaultStr(data.m_name, originalName);
  }

  auto uniqueName = originalName;
  int counter = 0;
  while (!checker(uniqueName))
    uniqueName = originalName + strings::to_string(++counter);

  if (counter > 0)
  {
    auto const sameCategoryId = GetCategoryId(originalName);
    if (data.m_id != kml::kInvalidMarkGroupId && data.m_id < sameCategoryId)
      SetCategoryName(sameCategoryId, uniqueName);
    else
      kml::SetDefaultStr(data.m_name, uniqueName);
  }
}

std::unique_ptr<kml::FileData> BookmarkManager::CollectBmGroupKMLData(BookmarkCategory const * group) const
{
  auto kmlData = std::make_unique<kml::FileData>();
  kmlData->m_serverId = group->GetServerId();
  kmlData->m_categoryData = group->GetCategoryData();
  auto const & markIds = group->GetUserMarks();
  kmlData->m_bookmarksData.reserve(markIds.size());
  for (auto it = markIds.rbegin(); it != markIds.rend(); ++it)
  {
    auto const * bm = GetBookmark(*it);
    kmlData->m_bookmarksData.emplace_back(bm->GetData());
  }
  auto const & lineIds = group->GetUserLines();
  kmlData->m_tracksData.reserve(lineIds.size());
  for (auto trackId : lineIds)
  {
    auto const * track = GetTrack(trackId);
    kmlData->m_tracksData.emplace_back(track->GetData());
  }

  for (auto const compilationId : group->GetCategoryData().m_compilationIds)
  {
    auto const & compilation = GetCategoryData(compilationId);
    kmlData->m_compilationsData.emplace_back(compilation);
  }
  return kmlData;
}

bool BookmarkManager::SaveBookmarkCategory(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto collection = PrepareToSaveBookmarks({groupId});
  if (!collection || collection->empty())
    return false;
  auto const & file = collection->front().first;
  auto & kmlData = *collection->front().second;
  return SaveKmlFileByExt(kmlData, file);
}

bool BookmarkManager::SaveBookmarkCategory(kml::MarkGroupId groupId, Writer & writer,
                                           KmlFileType fileType) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * group = GetBmCategory(groupId);
  auto kmlData = CollectBmGroupKMLData(group);
  return SaveKmlData(*kmlData, writer, fileType);
}

BookmarkManager::KMLDataCollectionPtr BookmarkManager::PrepareToSaveBookmarks(
  kml::GroupIdCollection const & groupIdCollection)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string const fileDir = GetBookmarksDirectory();

  if (!GetPlatform().IsFileExistsByFullPath(fileDir) && !GetPlatform().MkDirChecked(fileDir))
    return nullptr;

  auto collection = std::make_shared<KMLDataCollection>();
  for (auto const groupId : groupIdCollection)
  {
    auto * group = GetBmCategory(groupId);

    // Get valid file name from category name
    std::string file = group->GetFileName();
    if (file.empty())
    {
      std::string name = RemoveInvalidSymbols(group->GetName());
      if (name.empty())
        name = kDefaultBookmarksFileName;

      file = GenerateUniqueFileName(fileDir, std::move(name));
      group->SetFileName(file);
    }

    collection->emplace_back(std::move(file), CollectBmGroupKMLData(group));
  }
  return collection;
}

void BookmarkManager::SaveBookmarks(kml::GroupIdCollection const & groupIdCollection)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (groupIdCollection.empty())
    return;

  auto kmlDataCollection = PrepareToSaveBookmarks(groupIdCollection);
  if (!kmlDataCollection)
    return;

  if (m_testModeEnabled)
  {
    // Save bookmarks synchronously.
    for (auto const & kmlItem : *kmlDataCollection)
      SaveKmlFileByExt(*kmlItem.second, kmlItem.first);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::File, [kmlDataCollection = std::move(kmlDataCollection)]()
  {
    for (auto const & kmlItem : *kmlDataCollection)
      SaveKmlFileByExt(*kmlItem.second, kmlItem.first);
  });
}

void BookmarkManager::PrepareFileForSharing(kml::MarkGroupId categoryId, SharingHandler && handler)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(handler, ());
  if (IsCategoryEmpty(categoryId))
  {
    handler(SharingResult(categoryId, SharingResult::Code::EmptyCategory));
    return;
  }

  auto collection = PrepareToSaveBookmarks({categoryId});
  if (!collection || collection->empty())
  {
    handler(SharingResult(categoryId, SharingResult::Code::FileError));
    return;
  }

  GetPlatform().RunTask(Platform::Thread::File,
                        [collection = std::move(collection), handler = std::move(handler)]() mutable
  {
    handler(GetFileForSharing(std::move(collection)));
  });
}

bool BookmarkManager::IsCategoryEmpty(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->IsEmpty();
}

bool BookmarkManager::IsUsedCategoryName(std::string const & name) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & c : m_categories)
  {
    if (c.second->GetName() == name)
      return true;
  }
  return false;
}

bool BookmarkManager::AreAllCategoriesVisible() const
{
  return CheckVisibility(true /* isVisible */);
}

bool BookmarkManager::AreAllCategoriesInvisible() const
{
  return CheckVisibility(false /* isVisible */);
}

bool BookmarkManager::CheckVisibility(bool isVisible) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & category : m_categories)
  {
    if (category.second->IsVisible() != isVisible)
      return false;
  }

  return true;
}

void BookmarkManager::SetAllCategoriesVisibility(bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto session = GetEditSession();
  for (auto const & category : m_categories)
  {
    category.second->SetIsVisible(visible);
  }
}

bool BookmarkManager::AreAllCompilationsVisible(kml::MarkGroupId categoryId, kml::CompilationType compilationType) const
{
  return CheckCompilationsVisibility(categoryId, compilationType, true);
}

bool BookmarkManager::AreAllCompilationsInvisible(kml::MarkGroupId categoryId, kml::CompilationType compilationType) const
{
  return CheckCompilationsVisibility(categoryId, compilationType, false);
}

bool BookmarkManager::CheckCompilationsVisibility(kml::MarkGroupId categoryId, kml::CompilationType compilationType, bool isVisible) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const categoryIt = m_categories.find(categoryId);
  CHECK(categoryIt != m_categories.end(), ());
  auto & category = *categoryIt->second;
  for (kml::MarkGroupId const compilationId : category.GetCategoryData().m_compilationIds)
  {
    auto const compilationIt = m_compilations.find(compilationId);
    CHECK(compilationIt != m_compilations.cend(), ());
    auto & compilation = *compilationIt->second;
    if (compilation.GetCategoryData().m_type != compilationType)
      continue;
    if (compilation.IsVisible() != isVisible)
      return false;
  }
  return true;
}

void BookmarkManager::SetChildCategoriesVisibility(kml::MarkGroupId categoryId, kml::CompilationType compilationType,
                                                   bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto session = GetEditSession();
  auto const categoryIt = m_categories.find(categoryId);
  CHECK(categoryIt != m_categories.end(), ());
  auto & category = *categoryIt->second;
  for (kml::MarkGroupId const compilationId : category.GetCategoryData().m_compilationIds)
  {
    auto const compilationIt = m_compilations.find(compilationId);
    CHECK(compilationIt != m_compilations.cend(), ());
    auto & compilation = *compilationIt->second;
    if (compilation.GetCategoryData().m_type != compilationType)
      continue;
    if (visible != compilation.IsVisible())
    {
      compilation.SetIsVisible(visible);
      category.SetDirty();
      if (visible)
        category.SetIsVisible(true);
    }
  }
}

void BookmarkManager::SetNotificationsEnabled(bool enabled)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_notificationsEnabled == enabled)
    return;

  m_notificationsEnabled = enabled;
  if (m_notificationsEnabled && m_openedEditSessionsCount == 0)
    NotifyChanges();
}

bool BookmarkManager::AreNotificationsEnabled() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_notificationsEnabled;
}

void BookmarkManager::EnableTestMode(bool enable)
{
  UserMarkIdStorage::Instance().EnableSaving(!enable);
  m_testModeEnabled = enable;
}

void BookmarkManager::FilterInvalidBookmarks(kml::MarkIdCollection & bookmarks) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  base::EraseIf(bookmarks, [this](kml::MarkId const & id){ return GetBookmark(id) == nullptr; });
}

void BookmarkManager::FilterInvalidTracks(kml::TrackIdCollection & tracks) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  base::EraseIf(tracks, [this](kml::TrackId const & id){ return GetTrack(id) == nullptr; });
}

kml::GroupIdSet BookmarkManager::MarksChangesTracker::GetAllGroupIds() const
{
  auto const & groupIds = m_bmManager->GetBmGroupsIdList();
  kml::GroupIdSet resultingSet(groupIds.begin(), groupIds.end());

  static_assert(UserMark::BOOKMARK == 0);
  for (uint32_t i = UserMark::BOOKMARK + 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    resultingSet.insert(static_cast<kml::MarkGroupId>(i));
  return resultingSet;
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisible(kml::MarkGroupId groupId) const
{
  return m_bmManager->IsVisible(groupId);
}

kml::MarkIdSet const & BookmarkManager::MarksChangesTracker::GetGroupPointIds(kml::MarkGroupId groupId) const
{
  return m_bmManager->GetUserMarkIds(groupId);
}

kml::TrackIdSet const & BookmarkManager::MarksChangesTracker::GetGroupLineIds(kml::MarkGroupId groupId) const
{
  return m_bmManager->GetTrackIds(groupId);
}

df::UserPointMark const * BookmarkManager::MarksChangesTracker::GetUserPointMark(kml::MarkId markId) const
{
  return m_bmManager->GetMark(markId);
}

df::UserLineMark const * BookmarkManager::MarksChangesTracker::GetUserLineMark(kml::TrackId lineId) const
{
  return m_bmManager->GetTrack(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddMark(kml::MarkId markId)
{
  m_createdMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteMark(kml::MarkId markId)
{
  auto const it = m_createdMarks.find(markId);
  if (it != m_createdMarks.end())
  {
    m_createdMarks.erase(it);
  }
  else
  {
    m_updatedMarks.erase(markId);
    m_removedMarks.insert(markId);
  }
}

void BookmarkManager::MarksChangesTracker::OnUpdateMark(kml::MarkId markId)
{
  if (m_createdMarks.find(markId) == m_createdMarks.end())
    m_updatedMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::InsertBookmark(kml::MarkId markId,
                                                          kml::MarkGroupId catId,
                                                          GroupMarkIdSet & setToInsert,
                                                          GroupMarkIdSet & setToErase)
{
  auto const itCat = setToErase.find(catId);
  if (itCat != setToErase.end())
  {
    auto const it = itCat->second.find(markId);
    if (it != itCat->second.end())
    {
      itCat->second.erase(it);
      if (itCat->second.empty())
        setToErase.erase(itCat);
      return;
    }
  }
  setToInsert[catId].insert(markId);
}

bool BookmarkManager::MarksChangesTracker::HasBookmarkCategories(
    kml::GroupIdSet const & groupIds) const
{
  for (auto groupId : groupIds)
  {
    if (m_bmManager->IsBookmarkCategory(groupId))
      return true;
  }
  return false;
}

void BookmarkManager::MarksChangesTracker::InferVisibility(BookmarkCategory * const group)
{
  kml::CategoryData const & categoryData = group->GetCategoryData();
  if (categoryData.m_compilationIds.empty())
    return;
  std::unordered_set<kml::MarkGroupId> visibility;
  visibility.reserve(categoryData.m_compilationIds.size());
  for (kml::MarkGroupId const compilationId : categoryData.m_compilationIds)
  {
    auto const compilation = m_bmManager->m_compilations.find(compilationId);
    CHECK(compilation != m_bmManager->m_compilations.end(), ());
    if (compilation->second->IsVisible())
      visibility.emplace(compilationId);
  }
  auto const groupId = group->GetID();
  for (kml::MarkId const userMark : m_bmManager->GetUserMarkIds(groupId))
  {
    if (!m_bmManager->IsBookmark(userMark))
      continue;
    Bookmark * const bookmark = m_bmManager->GetBookmarkForEdit(userMark);
    bool isVisible = false;
    if (bookmark->GetCompilations().empty())
    {
      // Bookmarks that not belong to any compilation have to be visible.
      // They can be hidden only by changing parental BookmarkCategory visibility to false.
      isVisible = true;
    }
    else
    {
      for (kml::MarkGroupId const compilationId : bookmark->GetCompilations())
      {
        if (visibility.count(compilationId) != 0)
        {
          isVisible = true;
          break;
        }
      }
    }
    bookmark->SetIsVisible(isVisible);
  }
}

void BookmarkManager::MarksChangesTracker::OnAttachBookmark(kml::MarkId markId,
                                                            kml::MarkGroupId catId)
{
  InsertBookmark(markId, catId, m_attachedBookmarks, m_detachedBookmarks);
}

void BookmarkManager::MarksChangesTracker::OnDetachBookmark(kml::MarkId markId,
                                                            kml::MarkGroupId catId)
{
  InsertBookmark(markId, catId, m_detachedBookmarks, m_attachedBookmarks);
}

void BookmarkManager::MarksChangesTracker::OnAddLine(kml::TrackId lineId)
{
  m_createdLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteLine(kml::TrackId lineId)
{
  auto const it = m_createdLines.find(lineId);
  if (it != m_createdLines.end())
    m_createdLines.erase(it);
  else
    m_removedLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddGroup(kml::MarkGroupId groupId)
{
  m_createdGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteGroup(kml::MarkGroupId groupId)
{
  m_updatedGroups.erase(groupId);
  m_becameVisibleGroups.erase(groupId);
  m_becameInvisibleGroups.erase(groupId);

  auto const it = m_createdGroups.find(groupId);
  if (it != m_createdGroups.end())
    m_createdGroups.erase(it);
  else
    m_removedGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnUpdateGroup(kml::MarkGroupId groupId)
{
  m_updatedGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnBecomeVisibleGroup(kml::MarkGroupId groupId)
{
  auto const it = m_becameInvisibleGroups.find(groupId);
  if (it != m_becameInvisibleGroups.end())
    m_becameInvisibleGroups.erase(it);
  else
    m_becameVisibleGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnBecomeInvisibleGroup(kml::MarkGroupId groupId)
{
  auto const it = m_becameVisibleGroups.find(groupId);
  if (it != m_becameVisibleGroups.end())
    m_becameVisibleGroups.erase(it);
  else
    m_becameInvisibleGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::AcceptDirtyItems()
{
  CHECK(m_updatedGroups.empty(), ());
  m_bmManager->GetDirtyGroups(m_updatedGroups);
  for (auto groupId : m_updatedGroups)
  {
    auto userMarkLayer = m_bmManager->GetGroup(groupId);
    if (auto const group = dynamic_cast<BookmarkCategory *>(userMarkLayer))
      InferVisibility(group);
    if (userMarkLayer->IsVisibilityChanged())
    {
      if (userMarkLayer->IsVisible())
        m_becameVisibleGroups.insert(groupId);
      else
        m_becameInvisibleGroups.insert(groupId);
    }
    userMarkLayer->ResetChanges();
  }

  kml::MarkIdSet dirtyMarks;
  for (auto const markId : m_updatedMarks)
  {
    auto const mark = m_bmManager->GetMark(markId);
    if (mark->IsDirty())
    {
      dirtyMarks.insert(markId);
      m_updatedGroups.insert(mark->GetGroupId());
      mark->ResetChanges();
    }
  }
  m_updatedMarks.swap(dirtyMarks);

  for (auto const markId : m_createdMarks)
  {
    auto const mark = m_bmManager->GetMark(markId);
    CHECK(mark->IsDirty(), ());
    mark->ResetChanges();
  }

  for (auto const lineId : m_createdLines)
  {
    auto const line = m_bmManager->GetTrack(lineId);
    CHECK(line->IsDirty(), ());
    line->ResetChanges();
  }
}

bool BookmarkManager::MarksChangesTracker::HasChanges() const
{
  return !m_updatedGroups.empty() || !m_removedGroups.empty();
}

bool BookmarkManager::MarksChangesTracker::HasBookmarksChanges() const
{
  return HasBookmarkCategories(m_updatedGroups) || HasBookmarkCategories(m_removedGroups);
}

bool BookmarkManager::MarksChangesTracker::HasCategoriesChanges() const
{
  return HasBookmarkCategories(m_createdGroups) || HasBookmarkCategories(m_removedGroups);
}

void BookmarkManager::MarksChangesTracker::ResetChanges()
{
  m_createdGroups.clear();
  m_removedGroups.clear();

  m_updatedGroups.clear();
  m_becameVisibleGroups.clear();
  m_becameInvisibleGroups.clear();

  m_createdMarks.clear();
  m_removedMarks.clear();
  m_updatedMarks.clear();

  m_createdLines.clear();
  m_removedLines.clear();

  m_attachedBookmarks.clear();
  m_detachedBookmarks.clear();
}

void BookmarkManager::MarksChangesTracker::AddChanges(MarksChangesTracker const & changes)
{
  if (!HasChanges())
  {
    *this = changes;
    return;
  }

  for (auto const groupId : changes.m_createdGroups)
    OnAddGroup(groupId);

  for (auto const groupId : changes.m_updatedGroups)
    OnUpdateGroup(groupId);

  for (auto const groupId : changes.m_becameVisibleGroups)
    OnBecomeVisibleGroup(groupId);

  for (auto const groupId : changes.m_becameInvisibleGroups)
    OnBecomeInvisibleGroup(groupId);

  for (auto const groupId : changes.m_removedGroups)
    OnDeleteGroup(groupId);

  for (auto const markId : changes.m_createdMarks)
    OnAddMark(markId);

  for (auto const markId : changes.m_updatedMarks)
    OnUpdateMark(markId);

  for (auto const markId : changes.m_removedMarks)
    OnDeleteMark(markId);

  for (auto const lineId : changes.m_createdLines)
    OnAddLine(lineId);

  for (auto const lineId : changes.m_removedLines)
    OnDeleteLine(lineId);

  for (auto const & attachedInfo : changes.m_attachedBookmarks)
  {
    for (auto const markId : attachedInfo.second)
      OnAttachBookmark(markId, attachedInfo.first);
  }

  for (auto const & detachedInfo : changes.m_detachedBookmarks)
  {
    for (auto const markId : detachedInfo.second)
      OnDetachBookmark(markId, detachedInfo.first);
  }
}

bool BookmarkManager::SortedBlock::operator==(SortedBlock const & other) const
{
  return m_blockName == other.m_blockName && m_markIds == other.m_markIds &&
    m_trackIds == other.m_trackIds;
}

bool BookmarkManager::Properties::GetProperty(std::string const & propertyName,
                                              std::string & value) const
{
  auto const it = m_values.find(propertyName);
  if (it == m_values.end())
    return false;
  value = it->second;
  return true;
}

bool BookmarkManager::Metadata::GetEntryProperty(std::string const & entryName,
                                                 std::string const & propertyName,
                                                 std::string & value) const
{
  auto const it = m_entriesProperties.find(entryName);
  if (it == m_entriesProperties.end())
    return false;

  return it->second.GetProperty(propertyName, value);
}

BookmarkManager::EditSession::EditSession(BookmarkManager & manager)
  : m_bmManager(manager)
{
  m_bmManager.OnEditSessionOpened();
}

BookmarkManager::EditSession::~EditSession()
{
  m_bmManager.OnEditSessionClosed();
}

Bookmark * BookmarkManager::EditSession::CreateBookmark(kml::BookmarkData && bmData)
{
  return m_bmManager.CreateBookmark(std::move(bmData));
}

Bookmark * BookmarkManager::EditSession::CreateBookmark(kml::BookmarkData && bmData, kml::MarkGroupId groupId)
{
  return m_bmManager.CreateBookmark(std::move(bmData), groupId);
}

Track * BookmarkManager::EditSession::CreateTrack(kml::TrackData && trackData)
{
  return m_bmManager.CreateTrack(std::move(trackData));
}

Bookmark * BookmarkManager::EditSession::GetBookmarkForEdit(kml::MarkId bmId)
{
  return m_bmManager.GetBookmarkForEdit(bmId);
}

void BookmarkManager::EditSession::DeleteUserMark(kml::MarkId markId)
{
  m_bmManager.DeleteUserMark(markId);
}

void BookmarkManager::EditSession::DeleteBookmark(kml::MarkId bmId)
{
  m_bmManager.DeleteBookmark(bmId);
}

Track * BookmarkManager::EditSession::GetTrackForEdit(kml::TrackId trackId)
{
  return m_bmManager.GetTrackForEdit(trackId);
}

void BookmarkManager::EditSession::DeleteTrack(kml::TrackId trackId)
{
  m_bmManager.DeleteTrack(trackId);
}

void BookmarkManager::EditSession::ClearGroup(kml::MarkGroupId groupId)
{
  m_bmManager.ClearGroup(groupId);
}

void BookmarkManager::EditSession::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  m_bmManager.SetIsVisible(groupId, visible);
}

void BookmarkManager::EditSession::MoveBookmark(
  kml::MarkId bmID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID)
{
  m_bmManager.MoveBookmark(bmID, curGroupID, newGroupID);
}

void BookmarkManager::EditSession::UpdateBookmark(kml::MarkId bmId, kml::BookmarkData const & bm)
{
  return m_bmManager.UpdateBookmark(bmId, bm);
}

void BookmarkManager::EditSession::AttachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId)
{
  m_bmManager.AttachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::DetachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId)
{
  m_bmManager.DetachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::MoveTrack(kml::TrackId trackID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID)
{
  m_bmManager.MoveTrack(trackID, curGroupID, newGroupID);
}

void BookmarkManager::EditSession::AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  m_bmManager.AttachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  m_bmManager.DetachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::SetCategoryName(kml::MarkGroupId categoryId, std::string const & name)
{
  m_bmManager.SetCategoryName(categoryId, name);
}

void BookmarkManager::EditSession::SetCategoryDescription(kml::MarkGroupId categoryId,
                                                          std::string const & desc)
{
  m_bmManager.SetCategoryDescription(categoryId, desc);
}

void BookmarkManager::EditSession::SetCategoryTags(kml::MarkGroupId categoryId, std::vector<std::string> const & tags)
{
  m_bmManager.SetCategoryTags(categoryId, tags);
}

void BookmarkManager::EditSession::SetCategoryAccessRules(kml::MarkGroupId categoryId, kml::AccessRules accessRules)
{
  m_bmManager.SetCategoryAccessRules(categoryId, accessRules);
}

void BookmarkManager::EditSession::SetCategoryCustomProperty(kml::MarkGroupId categoryId, std::string const & key,
                                                             std::string const & value)
{
  m_bmManager.SetCategoryCustomProperty(categoryId, key, value);
}

bool BookmarkManager::EditSession::DeleteBmCategory(kml::MarkGroupId groupId)
{
  return m_bmManager.DeleteBmCategory(groupId);
}

void BookmarkManager::EditSession::NotifyChanges()
{
  m_bmManager.NotifyChanges();
}
