#include "map/bookmark_manager.hpp"
#include "map/api_mark_point.hpp"
#include "map/local_ads_mark.hpp"
#include "map/routing_mark.hpp"
#include "map/search_mark.hpp"
#include "map/user.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_id_storage.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/sha1.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <list>
#include <memory>
#include <sstream>

using namespace std::placeholders;

namespace
{
std::string const kLastEditedBookmarkCategory = "LastBookmarkCategory";
// TODO(darina): Delete old setting.
std::string const kLastBookmarkType = "LastBookmarkType";
std::string const kLastEditedBookmarkColor = "LastBookmarkColor";
std::string const kDefaultBookmarksFileName = "Bookmarks";
std::string const kHotelsBookmarks = "Hotels";
std::string const kBookmarkCloudSettingsParam = "BookmarkCloudParam";

// Returns extension with a dot in a lower case.
std::string GetFileExt(std::string const & filePath)
{
  return strings::MakeLowerCase(base::GetFileExtension(filePath));
}

std::string GetFileName(std::string const & filePath)
{
  std::string ret = filePath;
  base::GetNameFromFullPath(ret);
  return ret;
}

std::string GetBookmarksDirectory()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "bookmarks");
}

std::string GetPrivateBookmarksDirectory()
{
  return base::JoinPath(GetPlatform().PrivateDir(), "bookmarks_private");
}

bool IsBadCharForPath(strings::UniChar const & c)
{
  static strings::UniChar const illegalChars[] = {':', '/', '\\', '<', '>', '\"', '|', '?', '*'};

  for (size_t i = 0; i < ARRAY_SIZE(illegalChars); ++i)
  {
    if (c < ' ' || illegalChars[i] == c)
      return true;
  }

  return false;
}

bool IsValidFilterType(BookmarkManager::CategoryFilterType const filter,
                       bool const fromCatalog)
{
  switch (filter)
  {
  case BookmarkManager::CategoryFilterType::All: return true;
  case BookmarkManager::CategoryFilterType::Public: return fromCatalog;
  case BookmarkManager::CategoryFilterType::Private: return !fromCatalog;
  }
  UNREACHABLE();
}

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
  auto const categoryName = kml::GetDefaultStr(kmlToShare.second->m_categoryData.m_name);
  std::string fileName = BookmarkManager::RemoveInvalidSymbols(categoryName, "");
  if (fileName.empty())
    fileName = base::GetNameFromFullPathWithoutExt(kmlToShare.first);
  auto const filePath = base::JoinPath(GetPlatform().TmpDir(), fileName + kKmlExtension);
  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, filePath));

  auto const categoryId = kmlToShare.second->m_categoryData.m_id;

  if (!SaveKmlFile(*kmlToShare.second, filePath, KmlFileType::Text))
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

Cloud::ConvertionResult ConvertBeforeUploading(std::string const & filePath,
                                               std::string const & convertedFilePath)
{
  std::string const fileName = base::GetNameFromFullPathWithoutExt(filePath);
  auto const tmpFilePath = base::JoinPath(GetPlatform().TmpDir(), fileName + kKmlExtension);
  SCOPE_GUARD(fileGuard, bind(&FileWriter::DeleteFileX, tmpFilePath));

  auto kmlData = LoadKmlFile(filePath, KmlFileType::Binary);
  if (kmlData == nullptr)
    return {};

  if (!SaveKmlFile(*kmlData, tmpFilePath, KmlFileType::Text))
    return {};

  Cloud::ConvertionResult result;
  result.m_hash = coding::SHA1::CalculateBase64(tmpFilePath);
  result.m_isSuccessful = CreateZipFromPathDeflatedAndDefaultCompression(tmpFilePath,
                                                                         convertedFilePath);
  return result;
}

Cloud::ConvertionResult ConvertAfterDownloading(std::string const & filePath,
                                                std::string const & convertedFilePath)
{
  std::string hash;
  auto kmlData = LoadKmzFile(filePath, hash);
  if (kmlData == nullptr)
    return {};
  
  Cloud::ConvertionResult result;
  result.m_hash = hash;
  result.m_isSuccessful = SaveKmlFile(*kmlData, convertedFilePath, KmlFileType::Binary);
  return result;
}
}  // namespace

namespace migration
{
std::string const kSettingsParam = "BookmarksMigrationCompleted";

std::string GetBackupFolderName()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "bookmarks_backup");
}

std::string CheckAndCreateBackupFolder()
{
  auto const commonBackupFolder = GetBackupFolderName();
  using Clock = std::chrono::system_clock;
  auto const ts = Clock::to_time_t(Clock::now());
  tm * t = gmtime(&ts);
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(4) << t->tm_year + 1900
     << std::setw(2) << t->tm_mon + 1 << std::setw(2) << t->tm_mday;
  auto const backupFolder = base::JoinPath(commonBackupFolder, ss.str());

  // In the case if the folder exists, try to resume.
  if (GetPlatform().IsFileExistsByFullPath(backupFolder))
    return backupFolder;

  // The backup will be in new folder.
  GetPlatform().RmDirRecursively(commonBackupFolder);
  if (!GetPlatform().MkDirChecked(commonBackupFolder))
    return {};
  if (!GetPlatform().MkDirChecked(backupFolder))
    return {};

  return backupFolder;
}

bool BackupBookmarks(std::string const & backupDir,
                     std::vector<std::string> const & files)
{
  for (auto const & f : files)
  {
    std::string fileName = base::GetNameFromFullPathWithoutExt(f);
    auto const kmzPath = base::JoinPath(backupDir, fileName + kKmzExtension);
    if (GetPlatform().IsFileExistsByFullPath(kmzPath))
      continue;

    if (!CreateZipFromPathDeflatedAndDefaultCompression(f, kmzPath))
      return false;
  }
  return true;
}

bool ConvertBookmarks(std::vector<std::string> const & files,
                      size_t & convertedCount)
{
  convertedCount = 0;

  auto const conversionFolder = base::JoinPath(GetBackupFolderName(),
                                             "conversion");
  if (!GetPlatform().IsFileExistsByFullPath(conversionFolder) &&
      !GetPlatform().MkDirChecked(conversionFolder))
  {
    return false;
  }

  // Convert all files to kmb.
  std::vector<std::string> convertedFiles;
  convertedFiles.reserve(files.size());
  for (auto const & f : files)
  {
    std::string fileName = base::GetNameFromFullPathWithoutExt(f);
    auto const kmbPath = base::JoinPath(conversionFolder, fileName + kKmbExtension);
    if (!GetPlatform().IsFileExistsByFullPath(kmbPath))
    {
      auto kmlData = LoadKmlFile(f, KmlFileType::Text);
      if (kmlData == nullptr)
        continue;

      if (!SaveKmlFile(*kmlData, kmbPath, KmlFileType::Binary))
      {
        base::DeleteFileX(kmbPath);
        continue;
      }
    }
    convertedFiles.push_back(kmbPath);
  }
  convertedCount = convertedFiles.size();

  auto const newBookmarksDir = GetBookmarksDirectory();
  if (!GetPlatform().IsFileExistsByFullPath(newBookmarksDir) &&
      !GetPlatform().MkDirChecked(newBookmarksDir))
  {
    return false;
  }

  // Move converted bookmark-files with respect of existing files.
  for (auto const & f : convertedFiles)
  {
    std::string fileName = base::GetNameFromFullPathWithoutExt(f);
    auto kmbPath = base::JoinPath(newBookmarksDir, fileName + kKmbExtension);
    size_t counter = 1;
    while (Platform::IsFileExistsByFullPath(kmbPath))
    {
      kmbPath = base::JoinPath(newBookmarksDir,
        fileName + strings::to_string(counter++) + kKmbExtension);
    }

    if (!base::RenameFileX(f, kmbPath))
      return false;
  }
  GetPlatform().RmDirRecursively(conversionFolder);

  return true;
}

void OnMigrationSuccess(size_t originalCount, size_t convertedCount)
{
  settings::Set(kSettingsParam, true /* is completed */);
  alohalytics::TStringMap details{
    {"original_count", strings::to_string(originalCount)},
    {"converted_count", strings::to_string(convertedCount)}};
  alohalytics::Stats::Instance().LogEvent("Bookmarks_migration_success", details);
}

void OnMigrationFailure(std::string && failedStage)
{
  alohalytics::TStringMap details{
    {"stage", std::move(failedStage)},
    {"free_space", strings::to_string(GetPlatform().GetWritableStorageSpace())}};
  alohalytics::Stats::Instance().LogEvent("Bookmarks_migration_failure", details);
}

bool IsMigrationCompleted()
{
  bool isCompleted;
  if (!settings::Get(kSettingsParam, isCompleted))
    return false;
  return isCompleted;
}

bool MigrateIfNeeded()
{
  if (IsMigrationCompleted())
    return true;

  std::string const dir = GetPlatform().SettingsDir();
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, kKmlExtension, files);
  if (files.empty())
  {
    auto const newBookmarksDir = GetBookmarksDirectory();
    if (!GetPlatform().IsFileExistsByFullPath(newBookmarksDir) &&
        !GetPlatform().MkDirChecked(newBookmarksDir))
    {
      LOG(LWARNING, ("Could not create directory:", newBookmarksDir));
      return false;
    }
    OnMigrationSuccess(0 /* originalCount */, 0 /* convertedCount */);
    return true;
  }

  for (auto & f : files)
    f = base::JoinFoldersToPath(dir, f);

  std::string failedStage;
  auto const backupDir = CheckAndCreateBackupFolder();
  if (backupDir.empty() || !BackupBookmarks(backupDir, files))
  {
    OnMigrationFailure("backup");
    return false;
  }

  size_t convertedCount;
  if (!ConvertBookmarks(files, convertedCount))
  {
    OnMigrationFailure("conversion");
    return false;
  }

  for (auto const & f : files)
    base::DeleteFileX(f);
  OnMigrationSuccess(files.size(), convertedCount);
  return true;
}

// Here we read backup and try to restore old-style #placemark-hotel bookmarks.
void FixUpHotelPlacemarks(BookmarkManager::KMLDataCollectionPtr & collection,
                          bool isMigrationCompleted)
{
  static std::string const kSettingsKey = "HotelPlacemarksExtracted";
  bool isHotelPlacemarksExtracted;
  if (settings::Get(kSettingsKey, isHotelPlacemarksExtracted) && isHotelPlacemarksExtracted)
    return;
  
  if (!isMigrationCompleted)
  {
    settings::Set(kSettingsKey, true);
    return;
  }
  
  // Find all hotel bookmarks in backup.
  Platform::FilesList files;
  Platform::GetFilesRecursively(GetBackupFolderName(), files);
  std::list<kml::BookmarkData> hotelBookmarks;
  for (auto const & f : files)
  {
    if (GetFileExt(f) != kKmzExtension)
      continue;
    
    std::string hash;
    auto kmlData = LoadKmzFile(f, hash);
    if (kmlData == nullptr)
      continue;
    
    for (auto & b : kmlData->m_bookmarksData)
    {
      if (b.m_icon == kml::BookmarkIcon::Hotel)
        hotelBookmarks.push_back(std::move(b));
    }
  }
  if (hotelBookmarks.empty())
  {
    settings::Set(kSettingsKey, true);
    return;
  }
  
  if (!collection)
    collection = std::make_shared<BookmarkManager::KMLDataCollection>();
  
  if (collection->empty())
  {
    auto fileData = std::make_unique<kml::FileData>();
    kml::SetDefaultStr(fileData->m_categoryData.m_name, kHotelsBookmarks);
    fileData->m_bookmarksData.assign(hotelBookmarks.begin(), hotelBookmarks.end());
    collection->emplace_back("", std::move(fileData));
    settings::Set(kSettingsKey, true);
    return;
  }
  
  // Match found hotel bookmarks with existing bookmarks.
  double constexpr kEps = 1e-5;
  for (auto const & p : *collection)
  {
    CHECK(p.second, ());
    bool needSave = false;
    for (auto & b : p.second->m_bookmarksData)
    {
      for (auto it = hotelBookmarks.begin(); it != hotelBookmarks.end(); ++it)
      {
        if (b.m_point.EqualDxDy(it->m_point, kEps))
        {
          needSave = true;
          b.m_color = it->m_color;
          b.m_icon = it->m_icon;
          hotelBookmarks.erase(it);
          break;
        }
      }
    }
    if (needSave)
    {
      CHECK(!p.first.empty(), ());
      SaveKmlFile(*p.second, p.first, KmlFileType::Binary);
    }
  }
  
  // Add not-matched hotel bookmarks.
  auto fileData = std::make_unique<kml::FileData>();
  kml::SetDefaultStr(fileData->m_categoryData.m_name, kHotelsBookmarks);
  fileData->m_bookmarksData.assign(hotelBookmarks.begin(), hotelBookmarks.end());
  if (!fileData->m_bookmarksData.empty())
    collection->emplace_back("", std::move(fileData));

  settings::Set(kSettingsKey, true);
}
}  // namespace migration

using namespace std::placeholders;

BookmarkManager::BookmarkManager(User & user, Callbacks && callbacks)
  : m_user(user)
  , m_callbacks(std::move(callbacks))
  , m_changesTracker(*this)
  , m_needTeardown(false)
  , m_bookmarkCloud(Cloud::CloudParams("bmc.json", "bookmarks", std::string(kBookmarkCloudSettingsParam),
                                       GetBookmarksDirectory(), std::string(kKmbExtension),
                                       std::bind(&ConvertBeforeUploading, _1, _2),
                                       std::bind(&ConvertAfterDownloading, _1, _2)))
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());

  m_userMarkLayers.reserve(UserMark::USER_MARK_TYPES_COUNT - 1);
  for (uint32_t i = 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkLayer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD{});
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});

  using namespace std::placeholders;
  m_bookmarkCloud.SetSynchronizationHandlers(
      std::bind(&BookmarkManager::OnSynchronizationStarted, this, _1),
      std::bind(&BookmarkManager::OnSynchronizationFinished, this, _1, _2, _3),
      std::bind(&BookmarkManager::OnRestoreRequested, this, _1, _2, _3),
      std::bind(&BookmarkManager::OnRestoredFilesPrepared, this));

  m_bookmarkCloud.SetInvalidTokenHandler([this] { m_user.ResetAccessToken(); });
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
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kBookmarksBookmarkAction,
                                                         {{"action", "create"}});

  auto const & c = classif();
  CHECK(c.HasTypesMapping(), ());
  std::stringstream ss;
  for (size_t i = 0; i < bm.m_featureTypes.size(); ++i)
  {
    ss << c.GetReadableObjectName(c.GetTypeForIndex(bm.m_featureTypes[i]));
    if (i + 1 < bm.m_featureTypes.size())
      ss << ",";
  }
  auto const latLon = MercatorBounds::ToLatLon(bm.m_point);
  alohalytics::TStringMap details{
    {"action", "create"},
    {"lat", strings::to_string(latLon.lat)},
    {"lon", strings::to_string(latLon.lon)},
    {"tags", ss.str()}};
  alohalytics::Stats::Instance().LogEvent("Bookmarks_Bookmark_action", details);

  bm.m_timestamp = std::chrono::system_clock::now();
  bm.m_viewportScale = static_cast<uint8_t>(df::GetZoomLevel(m_viewport.GetScale()));

  auto * bookmark = CreateBookmark(std::move(bm));
  bookmark->Attach(groupId);
  auto * group = GetBmCategory(groupId);
  group->AttachUserMark(bookmark->GetId());
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

void BookmarkManager::AttachBookmark(kml::MarkId bmId, kml::MarkGroupId catID)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Attach(catID);
  GetGroup(catID)->AttachUserMark(bmId);
}

void BookmarkManager::DetachBookmark(kml::MarkId bmId, kml::MarkGroupId catID)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Detach();
  GetGroup(catID)->DetachUserMark(bmId);
}

void BookmarkManager::DeleteBookmark(kml::MarkId bmId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmark(bmId), ());
  auto groupIt = m_bookmarks.find(bmId);
  auto const groupId = groupIt->second->GetGroupId();
  if (groupId)
    GetGroup(groupId)->DetachUserMark(bmId);
  m_changesTracker.OnDeleteMark(bmId);
  m_bookmarks.erase(groupIt);
}

Track * BookmarkManager::CreateTrack(kml::TrackData && trackData)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return AddTrack(std::make_unique<Track>(std::move(trackData)));
}

Track const * BookmarkManager::GetTrack(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  return (it != m_tracks.end()) ? it->second.get() : nullptr;
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
  GetBmCategory(groupId)->DetachTrack(trackId);
}

void BookmarkManager::DeleteTrack(kml::TrackId trackId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  auto const groupId = it->second->GetGroupId();
  if (groupId != kml::kInvalidMarkGroupId)
    GetBmCategory(groupId)->DetachTrack(trackId);
  m_changesTracker.OnDeleteLine(trackId);
  m_tracks.erase(it);
}

void BookmarkManager::CollectDirtyGroups(kml::GroupIdSet & dirtyGroups)
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
  if (!m_notificationsEnabled)
    return;

  if (!m_changesTracker.CheckChanges() && !m_firstDrapeNotification)
    return;

  bool hasBookmarks = false;
  kml::GroupIdCollection categoriesToSave;
  for (auto groupId : m_changesTracker.GetDirtyGroupIds())
  {
    if (IsBookmarkCategory(groupId))
    {
      if (GetBmCategory(groupId)->IsAutoSaveEnabled())
        categoriesToSave.push_back(groupId);
      hasBookmarks = true;
    }
  }
  if (hasBookmarks)
  {
    SaveBookmarks(categoriesToSave);
    SendBookmarksChanges();
  }

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    auto engine = lock.Get();
    for (auto groupId : m_changesTracker.GetDirtyGroupIds())
    {
      auto * group = GetGroup(groupId);
      engine->ChangeVisibilityUserMarksGroup(groupId, group->IsVisible());
    }

    for (auto groupId : m_changesTracker.GetRemovedGroupIds())
      engine->ClearUserMarksGroup(groupId);

    engine->UpdateUserMarks(&m_changesTracker, m_firstDrapeNotification);
    m_firstDrapeNotification = false;

    for (auto groupId : m_changesTracker.GetDirtyGroupIds())
    {
      auto * group = GetGroup(groupId);
      group->ResetChanges();
    }

    engine->InvalidateUserMarks();
  }

  for (auto const markId : m_changesTracker.GetUpdatedMarkIds())
    GetMark(markId)->ResetChanges();

  m_changesTracker.ResetChanges();
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

void BookmarkManager::ClearGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto * group = GetGroup(groupId);
  for (auto markId : group->GetUserMarks())
  {
    m_changesTracker.OnDeleteMark(markId);
    if (IsBookmarkCategory(groupId))
      m_bookmarks.erase(markId);
    else
      m_userMarks.erase(markId);
  }
  for (auto trackId : group->GetUserLines())
  {
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

UserMark const * BookmarkManager::FindMarkInRect(kml::MarkGroupId groupId, m2::AnyRectD const & rect, double & d) const
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
      if (mark->IsAvailableForSearch() && rect.IsPointInside(mark->GetPivot()))
        f(mark);
    }
  }
  return resMark;
}

void BookmarkManager::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  GetGroup(groupId)->SetIsVisible(visible);
}

bool BookmarkManager::IsVisible(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->IsVisible();
}

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  m_firstDrapeNotification = true;
}

void BookmarkManager::UpdateViewport(ScreenBase const & screen)
{
  m_viewport = screen;
}

void BookmarkManager::SetAsyncLoadingCallbacks(AsyncLoadingCallbacks && callbacks)
{
  m_asyncLoadingCallbacks = std::move(callbacks);
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

void BookmarkManager::ClearCategories()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto groupId : m_bmGroupsIdList)
  {
    ClearGroup(groupId);
    m_changesTracker.OnDeleteGroup(groupId);
  }

  m_categories.clear();
  UpdateBmGroupIdList();

  m_bookmarks.clear();
  m_tracks.clear();
}

BookmarkManager::KMLDataCollectionPtr BookmarkManager::LoadBookmarks(
    std::string const & dir, std::string const & ext, KmlFileType fileType,
    BookmarksChecker const & checker, std::vector<std::string> & cloudFilePaths)
{
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, ext, files);

  auto collection = std::make_shared<KMLDataCollection>();
  collection->reserve(files.size());
  cloudFilePaths.reserve(files.size());
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
    if (!kmlData->m_bookmarksData.empty() || !kmlData->m_tracksData.empty())
      cloudFilePaths.push_back(filePath);
    collection->emplace_back(filePath, std::move(kmlData));
  }
  return collection;
}

void BookmarkManager::LoadBookmarks()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ClearCategories();
  m_loadBookmarksFinished = false;
  if (!IsMigrated())
    m_migrationInProgress = true;
  NotifyAboutStartAsyncLoading();
  auto const userId = m_user.GetUserId();
  GetPlatform().RunTask(Platform::Thread::File, [this, userId]()
  {
    bool const isMigrationCompleted = migration::IsMigrationCompleted();
    bool const migrated = migration::MigrateIfNeeded();
    std::string const dir = migrated ? GetBookmarksDirectory() : GetPlatform().SettingsDir();
    std::string const filesExt = migrated ? kKmbExtension : kKmlExtension;
    
    std::vector<std::string> cloudFilePaths;
    auto collection = LoadBookmarks(dir, filesExt, migrated ? KmlFileType::Binary : KmlFileType::Text,
      [](kml::FileData const & kmlData)
    {
      return true;  // Allow to load any files from the bookmarks directory.
    }, cloudFilePaths);
    
    migration::FixUpHotelPlacemarks(collection, isMigrationCompleted);

    // Load files downloaded from catalog.
    std::vector<std::string> unusedFilePaths;
    auto catalogCollection = LoadBookmarks(GetPrivateBookmarksDirectory(), kKmbExtension,
                                           KmlFileType::Binary, [](kml::FileData const & kmlData)
    {
      return FromCatalog(kmlData);
    }, unusedFilePaths);
    
    collection->reserve(collection->size() + catalogCollection->size());
    for (auto & p : *catalogCollection)
      collection->emplace_back(p.first, std::move(p.second));

    if (m_needTeardown)
      return;
    NotifyAboutFinishAsyncLoading(std::move(collection));
    if (migrated)
    {
      GetPlatform().RunTask(Platform::Thread::Gui,
                            [this, cloudFilePaths]() { m_bookmarkCloud.Init(cloudFilePaths); });
    }
  });

  LoadState();
}

void BookmarkManager::LoadBookmark(std::string const & filePath, bool isTemporaryFile)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  // Defer bookmark loading in case of another asynchronous process.
  if (!m_loadBookmarksFinished || m_asyncLoadingInProgress || m_conversionInProgress ||
      m_restoreApplying)
  {
    m_bookmarkLoadingQueue.emplace_back(filePath, isTemporaryFile);
    return;
  }

  NotifyAboutStartAsyncLoading();
  LoadBookmarkRoutine(filePath, isTemporaryFile);
}

void BookmarkManager::LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile)
{
  auto const userId = m_user.GetUserId();
  GetPlatform().RunTask(Platform::Thread::File, [this, filePath, isTemporaryFile, userId]()
  {
    if (m_needTeardown)
      return;

    bool const migrated = migration::IsMigrationCompleted();

    auto collection = std::make_shared<KMLDataCollection>();

    auto const savePath = GetKMLPath(filePath);
    if (savePath)
    {
      auto fileSavePath = savePath.get();
      auto kmlData = LoadKmlFile(fileSavePath, KmlFileType::Text);
      if (kmlData && (::IsMyCategory(userId, kmlData->m_categoryData) || !FromCatalog(*kmlData)))
      {
        if (m_needTeardown)
          return;

        if (migrated)
        {
          std::string fileName = base::GetNameFromFullPathWithoutExt(fileSavePath);

          base::DeleteFileX(fileSavePath);
          fileSavePath = GenerateValidAndUniqueFilePathForKMB(fileName);

          if (!SaveKmlFile(*kmlData, fileSavePath, KmlFileType::Binary))
          {
            base::DeleteFileX(fileSavePath);
            fileSavePath.clear();
          }
        }
        if (!fileSavePath.empty())
          collection->emplace_back(fileSavePath, std::move(kmlData));
      }
    }

    if (m_needTeardown)
      return;

    bool const success = !collection->empty();

    NotifyAboutFile(success, filePath, isTemporaryFile);
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
    if (m_migrationInProgress)
    {
      m_migrationInProgress = false;
      SaveBookmarks(m_bmGroupsIdList);
    }
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

boost::optional<std::string> BookmarkManager::GetKMLPath(std::string const & filePath)
{
  std::string const fileExt = GetFileExt(filePath);
  string fileSavePath;
  if (fileExt == kKmlExtension)
  {
    fileSavePath = GenerateValidAndUniqueFilePathForKML(GetFileName(filePath));
    if (!base::CopyFileX(filePath, fileSavePath))
      return {};
  }
  else if (fileExt == kKmzExtension)
  {
    try
    {
      ZipFileReader::FileListT files;
      ZipFileReader::FilesList(filePath, files);
      std::string kmlFileName;
      std::string ext;
      for (size_t i = 0; i < files.size(); ++i)
      {
        ext = GetFileExt(files[i].first);
        if (ext == kKmlExtension)
        {
          kmlFileName = files[i].first;
          break;
        }
      }
      if (kmlFileName.empty())
        return {};

      fileSavePath = GenerateValidAndUniqueFilePathForKML(kmlFileName);
      ZipFileReader::UnzipFile(filePath, kmlFileName, fileSavePath);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Error unzipping file", filePath, e.Msg()));
      return {};
    }
  }
  else
  {
    LOG(LWARNING, ("Unknown file type", filePath));
    return {};
  }
  return fileSavePath;
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

  if (HasBmCategory(m_lastEditedGroupId) && IsEditableCategory(m_lastEditedGroupId))
    return m_lastEditedGroupId;

  for (auto & cat : m_categories)
  {
    if (IsEditableCategory(cat.first) && cat.second->GetFileName() == m_lastCategoryUrl)
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
  if (!IsEditableCategory(groupId))
    return;
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
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

BookmarkCategory * BookmarkManager::GetBmCategory(kml::MarkGroupId categoryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

void BookmarkManager::SendBookmarksChanges()
{
  if (m_callbacks.m_createdBookmarksCallback != nullptr)
  {
    std::vector<std::pair<kml::MarkId, kml::BookmarkData>> marksInfo;
    GetBookmarksData(m_changesTracker.GetCreatedMarkIds(), marksInfo);
    m_callbacks.m_createdBookmarksCallback(marksInfo);
  }
  if (m_callbacks.m_updatedBookmarksCallback != nullptr)
  {
    std::vector<std::pair<kml::MarkId, kml::BookmarkData>> marksInfo;
    GetBookmarksData(m_changesTracker.GetUpdatedMarkIds(), marksInfo);
    m_callbacks.m_updatedBookmarksCallback(marksInfo);
  }
  if (m_callbacks.m_deletedBookmarksCallback != nullptr)
  {
    kml::MarkIdCollection idCollection;
    auto const & removedIds = m_changesTracker.GetRemovedMarkIds();
    idCollection.reserve(removedIds.size());
    for (auto markId : removedIds)
    {
      if (IsBookmark(markId))
        idCollection.push_back(markId);
    }
    m_callbacks.m_deletedBookmarksCallback(idCollection);
  }
}

void BookmarkManager::GetBookmarksData(kml::MarkIdSet const & markIds,
                                       std::vector<std::pair<kml::MarkId, kml::BookmarkData>> & data) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  data.clear();
  data.reserve(markIds.size());
  for (auto markId : markIds)
  {
    auto const * bookmark = GetBookmark(markId);
    if (bookmark)
      data.emplace_back(markId, bookmark->GetData());
  }
}

bool BookmarkManager::HasBmCategory(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_categories.find(groupId) != m_categories.end();
}

void BookmarkManager::UpdateBmGroupIdList()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_bmGroupsIdList.clear();
  m_bmGroupsIdList.reserve(m_categories.size());
  for (auto it = m_categories.crbegin(); it != m_categories.crend(); ++it)
    m_bmGroupsIdList.push_back(it->first);
}

kml::MarkGroupId BookmarkManager::CreateBookmarkCategory(kml::CategoryData && data, bool autoSave)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (data.m_id == kml::kInvalidMarkGroupId)
    data.m_id = UserMarkIdStorage::Instance().GetNextCategoryId();
  auto groupId = data.m_id;
  CHECK_EQUAL(m_categories.count(groupId), 0, ());
  m_categories[groupId] = std::make_unique<BookmarkCategory>(std::move(data), autoSave);
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
  return groupId;
}

kml::MarkGroupId BookmarkManager::CheckAndCreateDefaultCategory()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & cat : m_categories)
  {
    if (IsEditableCategory(cat.first) && !IsCategoryFromCatalog(cat.first))
      return cat.first;
  }
  for (auto const & cat : m_categories)
  {
    if (IsEditableCategory(cat.first))
      return cat.first;
  }
  return CreateBookmarkCategory(m_callbacks.m_getStringsBundle().GetString("core_my_places"));
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
  m_bookmarkCatalog.UnregisterByServerId(it->second->GetServerId());

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
                              BookmarkManager const * manager)
    : m_rectHolder(rectHolder)
    , m_d(numeric_limits<double>::max())
    , m_mark(nullptr)
    , m_manager(manager)
  {}

  void operator()(kml::MarkGroupId groupId)
  {
    m2::AnyRectD const & rect = m_rectHolder(BookmarkManager::GetGroupType(groupId));
    if (UserMark const * p = m_manager->FindMarkInRect(groupId, rect, m_d))
    {
      static double const kEps = 1e-5;
      if (m_mark == nullptr || !p->GetPivot().EqualDxDy(m_mark->GetPivot(), kEps))
        m_mark = p;
    }
  }

  UserMark const * GetFoundMark() const { return m_mark; }

private:
  BookmarkManager::TTouchRectHolder const & m_rectHolder;
  double m_d;
  UserMark const * m_mark;
  BookmarkManager const * m_manager;
};
}  // namespace

UserMark const * BookmarkManager::FindNearestUserMark(m2::AnyRectD const & rect) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  BestUserMarkFinder finder(holder, this);
  finder(UserMark::Type::ROUTING);
  finder(UserMark::Type::SEARCH);
  finder(UserMark::Type::API);
  for (auto & pair : m_categories)
    finder(pair.first);

  return finder.GetFoundMark();
}

UserMarkLayer const * BookmarkManager::GetGroup(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
    return m_userMarkLayers[groupId - 1].get();

  ASSERT(m_categories.find(groupId) != m_categories.end(), ());
  return m_categories.at(groupId).get();
}

UserMarkLayer * BookmarkManager::GetGroup(kml::MarkGroupId groupId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::USER_MARK_TYPES_COUNT)
    return m_userMarkLayers[groupId - 1].get();

  auto const it = m_categories.find(groupId);
  return it != m_categories.end() ? it->second.get() : nullptr;
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

    if (FromCatalog(fileData))
      m_bookmarkCatalog.RegisterByServerId(fileData.m_serverId);

    if (!UserMarkIdStorage::Instance().CheckIds(fileData) || HasDuplicatedIds(fileData))
    {
      //TODO: notify subscribers(like search subsystem). This KML could have been indexed.
      ResetIds(fileData);
    }

    auto originalName = kml::GetDefaultStr(categoryData.m_name);
    if (originalName.empty())
    {
      originalName = kDefaultBookmarksFileName;
      kml::SetDefaultStr(categoryData.m_name, originalName);
    }

    auto uniqueName = originalName;
    int counter = 0;
    while (IsUsedCategoryName(uniqueName))
      uniqueName = originalName + strings::to_string(++counter);

    if (counter > 0)
    {
      auto const sameCategoryId = GetCategoryId(originalName);
      if (categoryData.m_id != kml::kInvalidMarkGroupId && categoryData.m_id < sameCategoryId)
        SetCategoryName(sameCategoryId, uniqueName);
      else
        kml::SetDefaultStr(categoryData.m_name, uniqueName);
    }

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

    for (auto & bmData : fileData.m_bookmarksData)
    {
      auto * bm = CreateBookmark(std::move(bmData));
      bm->Attach(groupId);
      group->AttachUserMark(bm->GetId());
    }
    for (auto & trackData : fileData.m_tracksData)
    {
      auto track = make_unique<Track>(std::move(trackData));
      auto * t = AddTrack(std::move(track));
      t->Attach(groupId);
      group->AttachTrack(t->GetId());
    }

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
  if (fileData.m_categoryData.m_id == kml::kInvalidMarkGroupId)
    return false;

  if (m_categories.find(fileData.m_categoryData.m_id) != m_categories.cend())
    return true;

  for (auto const & b : fileData.m_bookmarksData)
  {
    if (m_bookmarks.count(b.m_id) > 0)
      return true;
  }

  for (auto const & t : fileData.m_tracksData)
  {
    if (m_tracks.count(t.m_id) > 0)
      return true;
  }
  return false;
}

std::unique_ptr<kml::FileData> BookmarkManager::CollectBmGroupKMLData(BookmarkCategory const * group) const
{
  auto kmlData = std::make_unique<kml::FileData>();
  kmlData->m_deviceId = GetPlatform().UniqueClientId();
  kmlData->m_serverId = group->GetServerId();
  kmlData->m_categoryData = group->GetCategoryData();
  auto const & markIds = group->GetUserMarks();
  kmlData->m_bookmarksData.reserve(markIds.size());
  for (auto it = markIds.rbegin(); it != markIds.rend(); ++it)
  {
    Bookmark const * bm = GetBookmark(*it);
    kmlData->m_bookmarksData.emplace_back(bm->GetData());
  }
  auto const & lineIds = group->GetUserLines();
  kmlData->m_tracksData.reserve(lineIds.size());
  for (auto trackId : lineIds)
  {
    Track const *track = GetTrack(trackId);
    kmlData->m_tracksData.emplace_back(track->GetData());
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
  return SaveKmlFileSafe(kmlData, file);
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

  bool migrated = migration::IsMigrationCompleted();
  std::string const fileDir = migrated ? GetBookmarksDirectory() : GetPlatform().SettingsDir();
  std::string const fileExt = migrated ? kKmbExtension : kKmlExtension;

  if (migrated && !GetPlatform().IsFileExistsByFullPath(fileDir) && !GetPlatform().MkDirChecked(fileDir))
    return nullptr;

  auto collection = std::make_shared<KMLDataCollection>();
  for (auto const groupId : groupIdCollection)
  {
    auto * group = GetBmCategory(groupId);

    if (group->IsCategoryFromCatalog() && !::IsMyCategory(m_user, group->GetCategoryData()))
    {
      auto const privateFileDir = GetPrivateBookmarksDirectory();
      if (!GetPlatform().IsFileExistsByFullPath(privateFileDir) &&
          !GetPlatform().MkDirChecked(privateFileDir))
      {
        return nullptr;
      }
      auto const fn = base::JoinPath(privateFileDir, group->GetServerId() + kKmbExtension);
      group->SetFileName(fn);
      collection->emplace_back(fn, CollectBmGroupKMLData(group));
      continue;
    }

    // Get valid file name from category name
    std::string const name = RemoveInvalidSymbols(group->GetName(), kDefaultBookmarksFileName);
    std::string file = group->GetFileName();

    if (file.empty())
    {
      file = GenerateUniqueFileName(fileDir, name, fileExt);
      group->SetFileName(file);
    }

    collection->emplace_back(file, CollectBmGroupKMLData(group));
  }
  return collection;
}

bool BookmarkManager::SaveKmlFileSafe(kml::FileData & kmlData, std::string const & file)
{
  auto const ext = base::GetFileExtension(file);
  auto const fileTmp = file + ".tmp";
  if (SaveKmlFile(kmlData, fileTmp, ext == kKmbExtension ?
                                    KmlFileType::Binary : KmlFileType::Text))
  {
    // Only after successful save we replace original file.
    base::DeleteFileX(file);
    VERIFY(base::RenameFileX(fileTmp, file), (fileTmp, file));
    return true;
  }
  base::DeleteFileX(fileTmp);
  return false;
}

void BookmarkManager::SaveBookmarks(kml::GroupIdCollection const & groupIdCollection)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_migrationInProgress)
    return;

  auto kmlDataCollection = PrepareToSaveBookmarks(groupIdCollection);
  if (!kmlDataCollection)
    return;

  if (m_testModeEnabled)
  {
    // Save bookmarks synchronously.
    for (auto const & kmlItem : *kmlDataCollection)
      SaveKmlFileSafe(*kmlItem.second, kmlItem.first);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::File,
                        [this, kmlDataCollection = std::move(kmlDataCollection)]()
  {
    for (auto const & kmlItem : *kmlDataCollection)
      SaveKmlFileSafe(*kmlItem.second, kmlItem.first);
  });
}

void BookmarkManager::SetCloudEnabled(bool enabled)
{
  m_bookmarkCloud.SetState(enabled ? Cloud::State::Enabled : Cloud::State::Disabled);
}

bool BookmarkManager::IsCloudEnabled() const
{
  return m_bookmarkCloud.GetState() == Cloud::State::Enabled;
}

uint64_t BookmarkManager::GetLastSynchronizationTimestampInMs() const
{
  return m_bookmarkCloud.GetLastSynchronizationTimestampInMs();
}

std::unique_ptr<User::Subscriber> BookmarkManager::GetUserSubscriber()
{
  return m_bookmarkCloud.GetUserSubscriber();
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

bool BookmarkManager::IsEditableBookmark(kml::MarkId bmId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const * mark = GetBookmark(bmId);
  if (mark->GetGroupId() != kml::kInvalidMarkGroupId)
    return IsEditableCategory(mark->GetGroupId());
  return true;
}

bool BookmarkManager::IsEditableTrack(kml::TrackId trackId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const * track = GetTrack(trackId);
  if (track->GetGroupId() != kml::kInvalidMarkGroupId)
    return IsEditableCategory(track->GetGroupId());
  return true;
}

bool BookmarkManager::IsEditableCategory(kml::MarkGroupId groupId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return IsMyCategory(groupId) || !IsCategoryFromCatalog(groupId);
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

bool BookmarkManager::AreAllCategoriesVisible(CategoryFilterType const filter) const
{
  return CheckVisibility(filter, true /* isVisible */);
}

bool BookmarkManager::AreAllCategoriesInvisible(CategoryFilterType const filter) const
{
  return CheckVisibility(filter, false /* isVisible */);
}

bool BookmarkManager::CheckVisibility(BookmarkManager::CategoryFilterType const filter,
                                      bool isVisible) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & category : m_categories)
  {
    auto const fromCatalog = IsCategoryFromCatalog(category.first) && !IsMyCategory(category.first);
    if (!IsValidFilterType(filter, fromCatalog))
      continue;
    if (category.second->IsVisible() != isVisible)
      return false;
  }

  return true;
}

void BookmarkManager::SetAllCategoriesVisibility(CategoryFilterType const filter, bool visible)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto session = GetEditSession();
  for (auto const & category : m_categories)
  {
    auto const fromCatalog = IsCategoryFromCatalog(category.first) && !IsMyCategory(category.first);
    if (!IsValidFilterType(filter, fromCatalog))
      continue;
    category.second->SetIsVisible(visible);
  }
}

bool BookmarkManager::CanConvert() const
{
  // The conversion available only after successful migration.
  // Also we cannot convert during asynchronous loading or another conversion.
  return migration::IsMigrationCompleted() && m_loadBookmarksFinished &&
         !m_asyncLoadingInProgress && !m_conversionInProgress;
}

void BookmarkManager::FinishConversion(ConversionHandler const & handler, bool result)
{
  handler(result);

  // Run deferred asynchronous loading if possible.
  GetPlatform().RunTask(Platform::Thread::Gui, [this]()
  {
    m_conversionInProgress = false;
    if (!m_bookmarkLoadingQueue.empty())
    {
      NotifyAboutStartAsyncLoading();
      LoadBookmarkRoutine(m_bookmarkLoadingQueue.front().m_filename,
                          m_bookmarkLoadingQueue.front().m_isTemporaryFile);
      m_bookmarkLoadingQueue.pop_front();
    }
  });
}

size_t BookmarkManager::GetKmlFilesCountForConversion() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (!CanConvert())
    return 0;

  Platform::FilesList files;
  Platform::GetFilesByExt(GetPlatform().SettingsDir(),
                          kKmlExtension, files);
  return files.size();
}

void BookmarkManager::ConvertAllKmlFiles(ConversionHandler && handler)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (!CanConvert())
    return;

  m_conversionInProgress = true;
  auto const userId = m_user.GetUserId();
  GetPlatform().RunTask(Platform::Thread::File, [this, userId, handler = std::move(handler)]()
  {
    auto const oldDir = GetPlatform().SettingsDir();
    Platform::FilesList files;
    Platform::GetFilesByExt(oldDir, kKmlExtension, files);
    for (auto & f : files)
      f = base::JoinPath(oldDir, f);

    auto const newDir = GetBookmarksDirectory();
    if (!GetPlatform().IsFileExistsByFullPath(newDir) && !GetPlatform().MkDirChecked(newDir))
    {
      FinishConversion(handler, false /* success */);
      return;
    }

    auto fileData = std::make_shared<KMLDataCollection>();

    bool allConverted = true;
    for (auto const & f : files)
    {
      std::unique_ptr<kml::FileData> kmlData = LoadKmlFile(f, KmlFileType::Text);
      if (kmlData == nullptr)
      {
        allConverted = false;
        continue;
      }

      // Skip KML files from the catalog which are not belonged to the user.
      if (FromCatalog(*kmlData) && !::IsMyCategory(userId, kmlData->m_categoryData))
        continue;

      std::string fileName = base::GetNameFromFullPathWithoutExt(f);
      auto kmbPath = base::JoinPath(newDir, fileName + kKmbExtension);
      size_t counter = 1;
      while (Platform::IsFileExistsByFullPath(kmbPath))
        kmbPath = base::JoinPath(newDir, fileName + strings::to_string(counter++) + kKmbExtension);

      if (!SaveKmlFile(*kmlData, kmbPath, KmlFileType::Binary))
      {
        allConverted = false;
        continue;
      }

      fileData->emplace_back(kmbPath, std::move(kmlData));
      base::DeleteFileX(f);
    }

    if (!fileData->empty())
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [this, fileData = std::move(fileData)]() mutable
      {
        CreateCategories(std::move(*fileData), true /* autoSave */);
      });
    }

    FinishConversion(handler, allConverted);
  });
}

void BookmarkManager::SetCloudHandlers(
    Cloud::SynchronizationStartedHandler && onSynchronizationStarted,
    Cloud::SynchronizationFinishedHandler && onSynchronizationFinished,
    Cloud::RestoreRequestedHandler && onRestoreRequested,
    Cloud::RestoredFilesPreparedHandler && onRestoredFilesPrepared)
{
  m_onSynchronizationStarted = std::move(onSynchronizationStarted);
  m_onSynchronizationFinished = std::move(onSynchronizationFinished);
  m_onRestoreRequested = std::move(onRestoreRequested);
  m_onRestoredFilesPrepared = std::move(onRestoredFilesPrepared);
}

void BookmarkManager::OnSynchronizationStarted(Cloud::SynchronizationType type)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, type]()
  {
    if (type == Cloud::SynchronizationType::Restore)
      m_restoreApplying = true;

    if (m_onSynchronizationStarted)
      m_onSynchronizationStarted(type);
  });

  LOG(LINFO, ("Cloud Synchronization Started:", type));
}

void BookmarkManager::OnSynchronizationFinished(Cloud::SynchronizationType type,
                                                Cloud::SynchronizationResult result,
                                                std::string const & errorStr)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, type, result, errorStr]()
  {
    if (m_onSynchronizationFinished)
      m_onSynchronizationFinished(type, result, errorStr);

    if (type == Cloud::SynchronizationType::Restore)
    {
      m_restoreApplying = false;
      // Reload bookmarks after restoring.
      if (result == Cloud::SynchronizationResult::Success)
        LoadBookmarks();
    }
  });

  LOG(LINFO, ("Cloud Synchronization Finished:", type, result, errorStr));
}

void BookmarkManager::OnRestoreRequested(Cloud::RestoringRequestResult result,
                                         std::string const & deviceName,
                                         uint64_t backupTimestampInMs)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, result, deviceName, backupTimestampInMs]()
  {
    if (m_onRestoreRequested)
      m_onRestoreRequested(result, deviceName, backupTimestampInMs);
  });

  using namespace std::chrono;
  LOG(LINFO, ("Cloud Restore Requested:", result, deviceName,
              time_point<system_clock>(milliseconds(backupTimestampInMs))));
}

void BookmarkManager::OnRestoredFilesPrepared()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Here we save some sensitive info, which must not be lost after restoring.
  for (auto groupId : m_bmGroupsIdList)
  {
    auto * group = GetBmCategory(groupId);
    auto const & data = group->GetCategoryData();
    if (m_user.GetUserId() == data.m_authorId && !group->GetServerId().empty() &&
        data.m_accessRules != kml::AccessRules::Local)
    {
      RestoringCache cache;
      cache.m_serverId = group->GetServerId();
      cache.m_accessRules = data.m_accessRules;
      m_restoringCache.insert({group->GetFileName(), std::move(cache)});
    }
  }

  ClearCategories();
  CheckAndResetLastIds();

  // Force notify about changes, because new groups can get the same ids.
  auto const notificationEnabled = AreNotificationsEnabled();
  SetNotificationsEnabled(true);
  NotifyChanges();
  SetNotificationsEnabled(notificationEnabled);

  if (m_onRestoredFilesPrepared)
    m_onRestoredFilesPrepared();

  LOG(LINFO, ("Cloud Restore Files: Prepared"));
}

void BookmarkManager::RequestCloudRestoring()
{
  m_bookmarkCloud.RequestRestoring();
}

void BookmarkManager::ApplyCloudRestoring()
{
  m_bookmarkCloud.ApplyRestoring();
}

void BookmarkManager::CancelCloudRestoring()
{
  m_bookmarkCloud.CancelRestoring();
}

void BookmarkManager::SetNotificationsEnabled(bool enabled)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_notificationsEnabled == enabled)
    return;

  m_notificationsEnabled = enabled;
  if (m_openedEditSessionsCount == 0)
    NotifyChanges();
}

bool BookmarkManager::AreNotificationsEnabled() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_notificationsEnabled;
}

void BookmarkManager::SetCatalogHandlers(OnCatalogDownloadStartedHandler && onCatalogDownloadStarted,
                                         OnCatalogDownloadFinishedHandler && onCatalogDownloadFinished,
                                         OnCatalogImportStartedHandler && onCatalogImportStarted,
                                         OnCatalogImportFinishedHandler && onCatalogImportFinished,
                                         OnCatalogUploadStartedHandler && onCatalogUploadStartedHandler,
                                         OnCatalogUploadFinishedHandler && onCatalogUploadFinishedHandler)
{
  m_onCatalogDownloadStarted = std::move(onCatalogDownloadStarted);
  m_onCatalogDownloadFinished = std::move(onCatalogDownloadFinished);
  m_onCatalogImportStarted = std::move(onCatalogImportStarted);
  m_onCatalogImportFinished = std::move(onCatalogImportFinished);
  m_onCatalogUploadStartedHandler = std::move(onCatalogUploadStartedHandler);
  m_onCatalogUploadFinishedHandler = std::move(onCatalogUploadFinishedHandler);
}

void BookmarkManager::DownloadFromCatalogAndImport(std::string const & id, std::string const & name)
{
  m_bookmarkCatalog.Download(id, name, m_user.GetAccessToken(), [this, id]()
  {
    if (m_onCatalogDownloadStarted)
      m_onCatalogDownloadStarted(id);
  },
    [this, id](BookmarkCatalog::DownloadResult result, std::string const & desc,
               std::string const & filePath)
  {
    UNUSED_VALUE(desc);

    if (m_onCatalogDownloadFinished)
      m_onCatalogDownloadFinished(id, result);

    if (result == BookmarkCatalog::DownloadResult::Success)
      ImportDownloadedFromCatalog(id, filePath);
  });
}

void BookmarkManager::ImportDownloadedFromCatalog(std::string const & id, std::string const & filePath)
{
  if (m_onCatalogImportStarted)
    m_onCatalogImportStarted(id);

  auto const userId = m_user.GetUserId();
  GetPlatform().RunTask(Platform::Thread::File, [this, id, filePath, userId]()
  {
    SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, filePath));
    std::string hash;
    auto kmlData = LoadKmzFile(filePath, hash);
    if (kmlData && FromCatalog(*kmlData) && kmlData->m_serverId == id)
    {
      bool const isMyCategory = ::IsMyCategory(userId, kmlData->m_categoryData);
      auto const p = base::JoinPath(isMyCategory ? GetBookmarksDirectory() : GetPrivateBookmarksDirectory(),
                                    id + kKmbExtension);
      auto collection = std::make_shared<KMLDataCollection>();
      collection->emplace_back(p, std::move(kmlData));

      GetPlatform().RunTask(Platform::Thread::Gui,
                            [this, id, collection = std::move(collection)]() mutable
      {
        std::vector<kml::MarkGroupId> idsToDelete;
        for (auto const & group : m_categories)
        {
          if (id == group.second->GetServerId())
            idsToDelete.push_back(group.first);
        }
        for (auto const & categoryId : idsToDelete)
        {
          ClearGroup(categoryId);
          m_changesTracker.OnDeleteGroup(categoryId);
          auto const it = m_categories.find(categoryId);
          FileWriter::DeleteFileX(it->second->GetFileName());
          m_categories.erase(categoryId);
        }
        UpdateBmGroupIdList();

        CreateCategories(std::move(*collection));

        kml::MarkGroupId newCategoryId = kml::kInvalidMarkGroupId;
        for (auto const & group : m_categories)
        {
          if (id == group.second->GetServerId())
          {
            newCategoryId = group.first;
            break;
          }
        }

        if (m_onCatalogImportFinished)
          m_onCatalogImportFinished(id, newCategoryId, true /* successful */);
      });
    }
    else
    {
      if (!kmlData)
      {
        LOG(LERROR, ("Malformed KML from the catalog"));
      }
      else
      {
        LOG(LERROR, ("KML from the catalog is invalid. Server ID =", kmlData->m_serverId,
            "Expected server ID =", id, "Access Rules =", kmlData->m_categoryData.m_accessRules));
      }

      GetPlatform().RunTask(Platform::Thread::Gui, [this, id]
      {
        if (m_onCatalogImportFinished)
          m_onCatalogImportFinished(id, kml::kInvalidMarkGroupId, false /* successful */);
      });
    }
  });
}

void BookmarkManager::UploadToCatalog(kml::MarkGroupId categoryId, kml::AccessRules accessRules)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_onCatalogUploadStartedHandler)
    m_onCatalogUploadStartedHandler(categoryId);

  if (!migration::IsMigrationCompleted())
  {
    if (m_onCatalogUploadFinishedHandler)
    {
      m_onCatalogUploadFinishedHandler(BookmarkCatalog::UploadResult::InvalidCall,
                                       "Bookmarks migration is not completed. Uploading is prohibited.",
                                       categoryId, categoryId);
    }
    return;
  }

  auto cat = GetBmCategory(categoryId);
  if (cat == nullptr)
  {
    if (m_onCatalogUploadFinishedHandler)
    {
      m_onCatalogUploadFinishedHandler(BookmarkCatalog::UploadResult::InvalidCall,
                                       "Unknown category.", categoryId, categoryId);
    }
    return;
  }

  // Force save bookmarks before uploading.
  SaveBookmarks({categoryId});

  BookmarkCatalog::UploadData uploadData;
  uploadData.m_accessRules = accessRules;
  uploadData.m_userId = m_user.GetUserId();
  uploadData.m_userName = m_user.GetUserName();
  uploadData.m_serverId = cat->GetServerId();

  auto kmlDataCollection = PrepareToSaveBookmarks({categoryId});
  if (!kmlDataCollection || kmlDataCollection->empty())
  {
    if (m_onCatalogUploadFinishedHandler)
    {
      m_onCatalogUploadFinishedHandler(BookmarkCatalog::UploadResult::InvalidCall,
                                       "Try to upload unsaved bookmarks.", categoryId, categoryId);
    }
    return;
  }

  auto onUploadSuccess = [this, categoryId](BookmarkCatalog::UploadResult result,
                                            std::shared_ptr<kml::FileData> fileData, bool originalFileExists,
                                            bool originalFileUnmodified) mutable
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    CHECK(fileData != nullptr, ());

    auto cat = GetBmCategory(categoryId);

    if (cat != nullptr && (!originalFileExists || originalFileUnmodified || cat->GetServerId() == fileData->m_serverId))
    {
      // Update just the bookmarks category properties in case when the category was unmodified or the category from the
      // catalog was changed after the start of the uploading.
      {
        auto session = GetEditSession();
        cat->SetServerId(fileData->m_serverId);
        cat->SetAccessRules(fileData->m_categoryData.m_accessRules);
        cat->SetAuthor(fileData->m_categoryData.m_authorName, fileData->m_categoryData.m_authorId);
      }

      if (cat->GetID() == m_lastEditedGroupId)
        SetLastEditedBmCategory(CheckAndCreateDefaultCategory());

      if (m_onCatalogUploadFinishedHandler)
        m_onCatalogUploadFinishedHandler(result, {}, categoryId, categoryId);
      return;
    }

    // Until we cannot block UI to prevent bookmarks modification during uploading,
    // we have to create a copy in this case.
    for (auto & n : fileData->m_categoryData.m_name)
      n.second += " (uploaded copy)";

    CHECK(migration::IsMigrationCompleted(), ());
    auto fileDataPtr = std::make_unique<kml::FileData>();
    auto const serverId = fileData->m_serverId;
    *fileDataPtr = std::move(*fileData);
    ResetIds(*fileDataPtr);
    KMLDataCollection collection;
    collection.emplace_back(std::make_pair("", std::move(fileDataPtr)));
    CreateCategories(std::move(collection), true /* autoSave */);

    kml::MarkGroupId newCategoryId = categoryId;
    for (auto const & category : m_categories)
    {
      if (category.second->GetServerId() == serverId)
      {
        newCategoryId = category.first;
        break;
      }
    }
    CHECK_NOT_EQUAL(newCategoryId, categoryId, ());

    if (m_onCatalogUploadFinishedHandler)
      m_onCatalogUploadFinishedHandler(result, {}, categoryId, newCategoryId);
  };

  auto onUploadError = [this, categoryId](BookmarkCatalog::UploadResult result,
                                          std::string const & description)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    if (m_onCatalogUploadFinishedHandler)
      m_onCatalogUploadFinishedHandler(result, description, categoryId, categoryId);
  };

  auto & kmlData = kmlDataCollection->front();
  std::shared_ptr<kml::FileData> fileData = std::move(kmlData.second);
  m_bookmarkCatalog.Upload(uploadData, m_user.GetAccessToken(), fileData,
                           kmlData.first, std::move(onUploadSuccess), std::move(onUploadError));
}

bool BookmarkManager::IsCategoryFromCatalog(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto cat = GetBmCategory(categoryId);
  if (cat == nullptr)
    return false;

  return cat->IsCategoryFromCatalog();
}

std::string BookmarkManager::GetCategoryCatalogDeeplink(kml::MarkGroupId categoryId) const
{
  auto cat = GetBmCategory(categoryId);
  if (cat == nullptr)
    return {};
  return cat->GetCatalogDeeplink();
}

BookmarkCatalog const & BookmarkManager::GetCatalog() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_bookmarkCatalog;
}

bool BookmarkManager::IsMyCategory(kml::MarkGroupId categoryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto cat = GetBmCategory(categoryId);
  if (cat == nullptr)
    return false;

  return ::IsMyCategory(m_user, cat->GetCategoryData());
}

void BookmarkManager::EnableTestMode(bool enable)
{
  UserMarkIdStorage::Instance().EnableSaving(!enable);
  m_testModeEnabled = enable;
}

kml::GroupIdSet BookmarkManager::MarksChangesTracker::GetAllGroupIds() const
{
  auto const & groupIds = m_bmManager.GetBmGroupsIdList();
  kml::GroupIdSet resultingSet(groupIds.begin(), groupIds.end());
  for (uint32_t i = 1; i < UserMark::USER_MARK_TYPES_COUNT; ++i)
    resultingSet.insert(static_cast<kml::MarkGroupId>(i));
  return resultingSet;
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisible(kml::MarkGroupId groupId) const
{
  return m_bmManager.IsVisible(groupId);
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisibilityChanged(kml::MarkGroupId groupId) const
{
  return m_bmManager.GetGroup(groupId)->IsVisibilityChanged();
}

kml::MarkIdSet const & BookmarkManager::MarksChangesTracker::GetGroupPointIds(kml::MarkGroupId groupId) const
{
  return m_bmManager.GetUserMarkIds(groupId);
}

kml::TrackIdSet const & BookmarkManager::MarksChangesTracker::GetGroupLineIds(kml::MarkGroupId groupId) const
{
  return m_bmManager.GetTrackIds(groupId);
}

df::UserPointMark const * BookmarkManager::MarksChangesTracker::GetUserPointMark(kml::MarkId markId) const
{
  return m_bmManager.GetMark(markId);
}

df::UserLineMark const * BookmarkManager::MarksChangesTracker::GetUserLineMark(kml::TrackId lineId) const
{
  return m_bmManager.GetTrack(lineId);
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
  auto const it = m_createdGroups.find(groupId);
  if (it != m_createdGroups.end())
    m_createdGroups.erase(it);
  else
    m_removedGroups.insert(groupId);
}

bool BookmarkManager::MarksChangesTracker::CheckChanges()
{
  m_bmManager.CollectDirtyGroups(m_dirtyGroups);
  for (auto const markId : m_updatedMarks)
  {
    auto const * mark = m_bmManager.GetMark(markId);
    if (mark->IsDirty())
      m_dirtyGroups.insert(mark->GetGroupId());
  }
  return !m_dirtyGroups.empty() || !m_removedGroups.empty();
}

void BookmarkManager::MarksChangesTracker::ResetChanges()
{
  m_dirtyGroups.clear();
  m_createdGroups.clear();
  m_removedGroups.clear();

  m_createdMarks.clear();
  m_removedMarks.clear();
  m_updatedMarks.clear();

  m_createdLines.clear();
  m_removedLines.clear();
}

// static
std::string BookmarkManager::RemoveInvalidSymbols(std::string const & name, std::string const & defaultName)
{
  // Remove not allowed symbols
  strings::UniString uniName = strings::MakeUniString(name);
  uniName.erase_if(&IsBadCharForPath);
  return (uniName.empty() ? defaultName : strings::ToUtf8(uniName));
}

// static
std::string BookmarkManager::GenerateUniqueFileName(const std::string & path, std::string name, std::string const & kmlExt)
{
  // check if file name already contains .kml extension
  size_t const extPos = name.rfind(kmlExt);
  if (extPos != std::string::npos)
  {
    // remove extension
    ASSERT_GREATER_OR_EQUAL(name.size(), kmlExt.size(), ());
    size_t const expectedPos = name.size() - kmlExt.size();
    if (extPos == expectedPos)
      name.resize(expectedPos);
  }

  size_t counter = 1;
  std::string suffix;
  while (Platform::IsFileExistsByFullPath(base::JoinPath(path, name + suffix + kmlExt)))
    suffix = strings::to_string(counter++);
  return base::JoinPath(path, name + suffix + kmlExt);
}

// static
std::string BookmarkManager::GenerateValidAndUniqueFilePathForKML(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName, kDefaultBookmarksFileName);
  return GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath, kKmlExtension);
}

// static
std::string BookmarkManager::GenerateValidAndUniqueFilePathForKMB(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName, kDefaultBookmarksFileName);
  return GenerateUniqueFileName(GetBookmarksDirectory(), filePath, kKmbExtension);
}

// static
bool BookmarkManager::IsMigrated()
{
  return migration::IsMigrationCompleted();
}

// static
std::string BookmarkManager::GetActualBookmarksDirectory()
{
  return IsMigrated() ? GetBookmarksDirectory() : GetPlatform().SettingsDir();
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
  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  return m_bmManager.CreateBookmark(std::move(bmData), groupId);
}

Track * BookmarkManager::EditSession::CreateTrack(kml::TrackData && trackData)
{
  return m_bmManager.CreateTrack(std::move(trackData));
}

Bookmark * BookmarkManager::EditSession::GetBookmarkForEdit(kml::MarkId bmId)
{
  CHECK(m_bmManager.IsEditableBookmark(bmId), ());
  return m_bmManager.GetBookmarkForEdit(bmId);
}

void BookmarkManager::EditSession::DeleteUserMark(kml::MarkId markId)
{
  m_bmManager.DeleteUserMark(markId);
}

void BookmarkManager::EditSession::DeleteBookmark(kml::MarkId bmId)
{
  CHECK(m_bmManager.IsEditableBookmark(bmId), ());
  m_bmManager.DeleteBookmark(bmId);
}

void BookmarkManager::EditSession::DeleteTrack(kml::TrackId trackId)
{
  CHECK(m_bmManager.IsEditableTrack(trackId), ());
  m_bmManager.DeleteTrack(trackId);
}

void BookmarkManager::EditSession::ClearGroup(kml::MarkGroupId groupId)
{
  if (m_bmManager.IsBookmarkCategory(groupId))
    CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.ClearGroup(groupId);
}

void BookmarkManager::EditSession::SetIsVisible(kml::MarkGroupId groupId, bool visible)
{
  m_bmManager.SetIsVisible(groupId, visible);
}

void BookmarkManager::EditSession::MoveBookmark(
  kml::MarkId bmID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID)
{
  CHECK(m_bmManager.IsEditableCategory(curGroupID), ());
  CHECK(m_bmManager.IsEditableCategory(newGroupID), ());

  m_bmManager.MoveBookmark(bmID, curGroupID, newGroupID);
}

void BookmarkManager::EditSession::UpdateBookmark(kml::MarkId bmId, kml::BookmarkData const & bm)
{
  CHECK(m_bmManager.IsEditableBookmark(bmId), ());
  return m_bmManager.UpdateBookmark(bmId, bm);
}

void BookmarkManager::EditSession::AttachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId)
{
  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.AttachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::DetachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId)
{
  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.DetachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.AttachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId)
{
  CHECK(m_bmManager.IsEditableCategory(groupId), ());
  m_bmManager.DetachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::SetCategoryName(kml::MarkGroupId categoryId, std::string const & name)
{
  CHECK(m_bmManager.IsEditableCategory(categoryId), ());
  m_bmManager.SetCategoryName(categoryId, name);
}

void BookmarkManager::EditSession::SetCategoryDescription(kml::MarkGroupId categoryId,
                                                          std::string const & desc)
{
  CHECK(m_bmManager.IsEditableCategory(categoryId), ());
  m_bmManager.SetCategoryDescription(categoryId, desc);
}

void BookmarkManager::EditSession::SetCategoryTags(kml::MarkGroupId categoryId, std::vector<std::string> const & tags)
{
  CHECK(m_bmManager.IsEditableCategory(categoryId), ());
  m_bmManager.SetCategoryTags(categoryId, tags);
}

void BookmarkManager::EditSession::SetCategoryAccessRules(kml::MarkGroupId categoryId, kml::AccessRules accessRules)
{
  CHECK(m_bmManager.IsEditableCategory(categoryId), ());
  m_bmManager.SetCategoryAccessRules(categoryId, accessRules);
}

void BookmarkManager::EditSession::SetCategoryCustomProperty(kml::MarkGroupId categoryId, std::string const & key,
                                                             std::string const & value)
{
  CHECK(m_bmManager.IsEditableCategory(categoryId), ());
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

namespace lightweight
{
namespace impl
{
bool IsBookmarksCloudEnabled()
{
  return Cloud::GetCloudState(kBookmarkCloudSettingsParam) == Cloud::State::Enabled;
}
}  // namespace impl
}  // namespace lightweight
