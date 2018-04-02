#include "map/bookmark_manager.hpp"
#include "map/api_mark_point.hpp"
#include "map/local_ads_mark.hpp"
#include "map/routing_mark.hpp"
#include "map/search_mark.hpp"
#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/scales.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/multilang_utf8_string.hpp"
#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std::placeholders;

namespace
{
std::string const kLastBookmarkCategoryId = "LastBookmarkCategoryId";
std::string const kLastBookmarkCategory = "LastBookmarkCategory";
std::string const kLastBookmarkType = "LastBookmarkType";
std::string const kLastBookmarkColor = "LastBookmarkColor";
std::string const kKmzExtension = ".kmz";
std::string const kBookmarksExt = ".kmb";

uint64_t LoadLastBmCategoryId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkCategoryId, val) && strings::to_uint64(val, lastId))
    return max(static_cast<uint64_t>(UserMark::COUNT), lastId);
  return static_cast<uint64_t>(UserMark::COUNT);
}

void SaveLastBmCategoryId(uint64_t lastId)
{
  GetPlatform().GetSecureStorage().Save(kLastBookmarkCategoryId, strings::to_string(lastId));
}

uint64_t ResetLastBmCategoryId()
{
  auto const lastId = static_cast<uint64_t>(UserMark::COUNT);
  SaveLastBmCategoryId(lastId);
  return lastId;
}

// Returns extension with a dot in a lower case.
std::string GetFileExt(std::string const & filePath)
{
  std::string ext = my::GetFileExtension(filePath);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

std::string GetFileName(std::string const & filePath)
{
  std::string ret = filePath;
  my::GetNameFromFullPath(ret);
  return ret;
}

std::string GetBookmarksDirectory()
{
  return my::JoinPath(GetPlatform().SettingsDir(), "bookmarks");
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
      double minDCandidate = m_globalCenter.SquareLength(org);
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

BookmarkManager::SharingResult GetFileForSharing(df::MarkGroupID categoryId, std::string const & filePath)
{
  if (!GetPlatform().IsFileExistsByFullPath(filePath))
  {
    return BookmarkManager::SharingResult(categoryId, BookmarkManager::SharingResult::Code::FileError,
                                          "Bookmarks file does not exist.");
  }

  auto ext = my::GetFileExtension(filePath);
  strings::AsciiToLower(ext);
  std::string fileName = filePath;
  my::GetNameFromFullPath(fileName);
  my::GetNameWithoutExt(fileName);
  auto const tmpFilePath = my::JoinFoldersToPath(GetPlatform().TmpDir(), fileName + kKmzExtension);
  if (ext == kKmzExtension)
  {
    if (my::CopyFileX(filePath, tmpFilePath))
      return BookmarkManager::SharingResult(categoryId, tmpFilePath);

    return BookmarkManager::SharingResult(categoryId, BookmarkManager::SharingResult::Code::FileError,
                                          "Could not copy file.");
  }

  if (!CreateZipFromPathDeflatedAndDefaultCompression(filePath, tmpFilePath))
  {
    return BookmarkManager::SharingResult(categoryId, BookmarkManager::SharingResult::Code::ArchiveError,
                                          "Could not create archive.");
  }

  return BookmarkManager::SharingResult(categoryId, tmpFilePath);
}

bool ConvertBeforeUploading(std::string const & filePath, std::string const & convertedFilePath)
{
  //TODO: convert from kmb to kmz.
  return CreateZipFromPathDeflatedAndDefaultCompression(filePath, convertedFilePath);
}

bool ConvertAfterDownloading(std::string const & filePath, std::string const & convertedFilePath)
{
  ZipFileReader::FileListT files;
  ZipFileReader::FilesList(filePath, files);
  if (files.empty())
    return false;

  std::string const unarchievedPath = filePath + ".raw";
  MY_SCOPE_GUARD(fileGuard, bind(&FileWriter::DeleteFileX, unarchievedPath));
  ZipFileReader::UnzipFile(filePath, files.front().first, unarchievedPath);
  if (!GetPlatform().IsFileExistsByFullPath(unarchievedPath))
    return false;

  kml::FileData kmlData;
  try
  {
    kml::DeserializerKml des(kmlData);
    FileReader reader(unarchievedPath);
    des.Deserialize(reader);
  }
  catch (FileReader::Exception const & exc)
  {
    LOG(LWARNING, ("KML text deserialization failure: ", exc.what(), "file:", unarchievedPath));
    return false;
  }

  try
  {
    kml::binary::SerializerKml ser(kmlData);
    FileWriter writer(convertedFilePath);
    ser.Serialize(writer);
  }
  catch (FileWriter::Exception const & exc)
  {
    LOG(LWARNING, ("KML binary serialization failure: ", exc.what(), "file:", convertedFilePath));
    return false;
  }

  return true;
}
}  // namespace

namespace migration
{
std::string const kSettingsParam = "BookmarksMigrationCompleted";

std::string GetBackupFolderName()
{
  return my::JoinPath(GetPlatform().SettingsDir(), "bookmarks_backup");
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
  auto const backupFolder = my::JoinPath(commonBackupFolder, ss.str());

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
    std::string fileName = f;
    my::GetNameFromFullPath(fileName);
    my::GetNameWithoutExt(fileName);
    auto const kmzPath = my::JoinPath(backupDir, fileName + kKmzExtension);
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

  auto const conversionFolder = my::JoinPath(GetBackupFolderName(),
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
    std::string fileName = f;
    my::GetNameFromFullPath(fileName);
    my::GetNameWithoutExt(fileName);
    auto const kmbPath = my::JoinPath(conversionFolder, fileName + kBookmarksExt);
    if (!GetPlatform().IsFileExistsByFullPath(kmbPath))
    {
      kml::FileData kmlData;
      try
      {
        kml::DeserializerKml des(kmlData);
        FileReader reader(f);
        des.Deserialize(reader);
      }
      catch (FileReader::Exception const &exc)
      {
        LOG(LDEBUG, ("KML text deserialization failure: ", exc.what(), "file", f));
        continue;
      }

      try
      {
        kml::binary::SerializerKml ser(kmlData);
        FileWriter writer(kmbPath);
        ser.Serialize(writer);
      }
      catch (FileWriter::Exception const &exc)
      {
        my::DeleteFileX(kmbPath);
        LOG(LDEBUG, ("KML binary serialization failure: ", exc.what(), "file", f));
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
    std::string fileName = f;
    my::GetNameFromFullPath(fileName);
    my::GetNameWithoutExt(fileName);
    auto kmbPath = my::JoinPath(newBookmarksDir, fileName + kBookmarksExt);
    size_t counter = 1;
    while (Platform::IsFileExistsByFullPath(kmbPath))
    {
      kmbPath = my::JoinPath(newBookmarksDir,
        fileName + strings::to_string(counter++) + kBookmarksExt);
    }

    if (!my::RenameFileX(f, kmbPath))
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
  Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);
  if (files.empty())
  {
    auto const newBookmarksDir = GetBookmarksDirectory();
    if (!GetPlatform().IsFileExistsByFullPath(newBookmarksDir))
      UNUSED_VALUE(GetPlatform().MkDirChecked(newBookmarksDir));
    OnMigrationSuccess(0 /* originalCount */, 0 /* convertedCount */);
    return true;
  }

  for (auto & f : files)
    f = my::JoinFoldersToPath(dir, f);

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

  //TODO(@darina): Uncomment after KMB integration.
  //for (auto const & f : files)
  //  my::DeleteFileX(f);
  OnMigrationSuccess(files.size(), convertedCount);
  return true;
}
}  // namespace migration

using namespace std::placeholders;

BookmarkManager::BookmarkManager(Callbacks && callbacks)
  : m_callbacks(std::move(callbacks))
  , m_changesTracker(*this)
  , m_needTeardown(false)
  , m_lastGroupID(LoadLastBmCategoryId())
  , m_bookmarkCloud(Cloud::CloudParams("bmc.json", "bookmarks", "BookmarkCloudParam",
                                       GetBookmarksDirectory(), std::string(kBookmarksExt),
                                       std::bind(&ConvertBeforeUploading, _1, _2),
                                       std::bind(&ConvertAfterDownloading, _1, _2)))
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());
  ASSERT_GREATER_OR_EQUAL(m_lastGroupID, UserMark::COUNT, ());
  m_userMarkLayers.reserve(UserMark::COUNT - 1);
  for (uint32_t i = 1; i < UserMark::COUNT; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkLayer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD{});
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});

  using namespace std::placeholders;
  m_bookmarkCloud.SetSynchronizationHandlers(
      std::bind(&BookmarkManager::OnSynchronizationStarted, this, _1),
      std::bind(&BookmarkManager::OnSynchronizationFinished, this, _1, _2, _3),
      std::bind(&BookmarkManager::OnRestoreRequested, this, _1, _2),
      std::bind(&BookmarkManager::OnRestoredFilesPrepared, this));
}

BookmarkManager::EditSession BookmarkManager::GetEditSession()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return EditSession(*this);
}

UserMark const * BookmarkManager::GetMark(df::MarkID markId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (IsBookmark(markId))
    return GetBookmark(markId);
  return GetUserMark(markId);
}

UserMark const * BookmarkManager::GetUserMark(df::MarkID markId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  return (it != m_userMarks.end()) ? it->second.get() : nullptr;
}

UserMark * BookmarkManager::GetUserMarkForEdit(df::MarkID markId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_userMarks.find(markId);
  if (it != m_userMarks.end())
  {
    m_changesTracker.OnUpdateMark(markId);
    return it->second.get();
  }
  return nullptr;
}

void BookmarkManager::DeleteUserMark(df::MarkID markId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(!IsBookmark(markId), ());
  auto it = m_userMarks.find(markId);
  auto const groupId = it->second->GetGroupId();
  GetGroup(groupId)->DetachUserMark(markId);
  m_changesTracker.OnDeleteMark(markId);
  m_userMarks.erase(it);
}

Bookmark * BookmarkManager::CreateBookmark(kml::BookmarkData const & bmData)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return AddBookmark(std::make_unique<Bookmark>(bmData));
}

Bookmark * BookmarkManager::CreateBookmark(kml::BookmarkData & bm, df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kBookmarksBookmarkAction,
                                                         {{"action", "create"}});

  bm.m_timestamp = std::chrono::system_clock::now();
  bm.m_viewportScale = static_cast<uint8_t>(df::GetZoomLevel(m_viewport.GetScale()));

  auto * bookmark = CreateBookmark(bm);
  bookmark->Attach(groupId);
  auto * group = GetBmCategory(groupId);
  group->AttachUserMark(bookmark->GetId());
  group->SetIsVisible(true);

  SetLastEditedBmCategory(groupId);
  SetLastEditedBmColor(bookmark->GetData().m_color.m_predefinedColor);

  return bookmark;
}

Bookmark const * BookmarkManager::GetBookmark(df::MarkID markId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_bookmarks.find(markId);
  return (it != m_bookmarks.end()) ? it->second.get() : nullptr;
}

Bookmark * BookmarkManager::GetBookmarkForEdit(df::MarkID markId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_bookmarks.find(markId);
  if (it == m_bookmarks.end())
    return nullptr;

  auto const groupId = it->second->GetGroupId();
  if (groupId != df::kInvalidMarkGroupId)
    m_changesTracker.OnUpdateMark(markId);

  return it->second.get();
}

void BookmarkManager::AttachBookmark(df::MarkID bmId, df::MarkGroupID catID)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Attach(catID);
  GetGroup(catID)->AttachUserMark(bmId);
}

void BookmarkManager::DetachBookmark(df::MarkID bmId, df::MarkGroupID catID)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetBookmarkForEdit(bmId)->Detach();
  GetGroup(catID)->DetachUserMark(bmId);
}

void BookmarkManager::DeleteBookmark(df::MarkID bmId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmark(bmId), ());
  auto groupIt = m_bookmarks.find(bmId);
  auto const groupId = groupIt->second->GetGroupId();
  if (groupId)
    GetGroup(groupId)->DetachUserMark(bmId);
  m_changesTracker.OnDeleteMark(bmId);
  m_bookmarks.erase(groupIt);
}

Track * BookmarkManager::CreateTrack(kml::TrackData const & trackData)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return AddTrack(std::make_unique<Track>(trackData));
}

Track const * BookmarkManager::GetTrack(df::LineID trackId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  return (it != m_tracks.end()) ? it->second.get() : nullptr;
}

void BookmarkManager::AttachTrack(df::LineID trackId, df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  it->second->Attach(groupId);
  GetBmCategory(groupId)->AttachTrack(trackId);
}

void BookmarkManager::DetachTrack(df::LineID trackId, df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetBmCategory(groupId)->DetachTrack(trackId);
}

void BookmarkManager::DeleteTrack(df::LineID trackId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_tracks.find(trackId);
  auto const groupId = it->second->GetGroupId();
  if (groupId != df::kInvalidMarkGroupId)
    GetBmCategory(groupId)->DetachTrack(trackId);
  m_changesTracker.OnDeleteLine(trackId);
  m_tracks.erase(it);
}

void BookmarkManager::CollectDirtyGroups(df::GroupIDSet & dirtyGroups)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & group : m_userMarkLayers)
  {
    if (!group->IsDirty())
      continue;
    auto const groupId = static_cast<df::MarkGroupID>(group->GetType());
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
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (!m_changesTracker.CheckChanges() && !m_firstDrapeNotification)
    return;

  bool hasBookmarks = false;
  df::GroupIDCollection categoriesToSave;
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

df::MarkIDSet const & BookmarkManager::GetUserMarkIds(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserMarks();
}

df::LineIDSet const & BookmarkManager::GetTrackIds(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetGroup(groupId)->GetUserLines();
}

void BookmarkManager::ClearGroup(df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
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

std::string BookmarkManager::GetCategoryName(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->GetName();
}

void BookmarkManager::SetCategoryName(df::MarkGroupID categoryId, std::string const & name)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetBmCategory(categoryId)->SetName(name);
}

std::string BookmarkManager::GetCategoryFileName(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->GetFileName();
}

UserMark const * BookmarkManager::FindMarkInRect(df::MarkGroupID groupId, m2::AnyRectD const & rect, double & d) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
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

void BookmarkManager::SetIsVisible(df::MarkGroupID groupId, bool visible)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetGroup(groupId)->SetIsVisible(visible);
}

bool BookmarkManager::IsVisible(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
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
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto * bm = bookmark.get();
  auto const markId = bm->GetId();
  ASSERT(m_bookmarks.count(markId) == 0, ());
  m_bookmarks.emplace(markId, std::move(bookmark));
  m_changesTracker.OnAddMark(markId);
  return bm;
}

Track * BookmarkManager::AddTrack(std::unique_ptr<Track> && track)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto * t = track.get();
  auto const trackId = t->GetId();
  ASSERT(m_tracks.count(trackId) == 0, ());
  m_tracks.emplace(trackId, std::move(track));
  m_changesTracker.OnAddLine(trackId);
  return t;
}

void BookmarkManager::SaveState() const
{
  settings::Set(kLastBookmarkCategory, m_lastCategoryUrl);
  settings::Set(kLastBookmarkColor, static_cast<uint32_t>(m_lastColor));
}

void BookmarkManager::LoadState()
{
  UNUSED_VALUE(settings::Get(kLastBookmarkCategory, m_lastCategoryUrl));
  uint32_t color;
  if (settings::Get(kLastBookmarkColor, color) &&
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
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto groupId : m_bmGroupsIdList)
  {
    ClearGroup(groupId);
    m_changesTracker.OnDeleteGroup(groupId);
  }

  m_categories.clear();
  m_bmGroupsIdList.clear();
  m_bookmarks.clear();
  m_tracks.clear();
}

std::shared_ptr<BookmarkManager::KMLDataCollection> BookmarkManager::LoadBookmarksKML(std::vector<std::string> & filePaths)
{
  std::string const dir = GetPlatform().SettingsDir();
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);

  auto collection = std::make_shared<KMLDataCollection>();
  collection->reserve(files.size());
  filePaths.reserve(files.size());
  for (auto const & file : files)
  {
    auto const filePath = dir + file;
    auto kmlData = std::make_unique<kml::FileData>();
    try
    {
      kml::DeserializerKml des(*kmlData);
      FileReader reader(filePath);
      des.Deserialize(reader);
    }
    catch (FileReader::Exception const &exc)
    {
      LOG(LDEBUG, ("KML deserialization failure: ", exc.what(), "file", filePath));
      continue;
    }
    if (m_needTeardown)
      break;
    filePaths.push_back(filePath);
    collection->emplace_back(filePath, std::move(kmlData));

    /*auto kmlData = LoadKMLFile(filePath);

    if (m_needTeardown)
      break;

    if (kmlData)
    {
      filePaths.push_back(filePath);
      collection->emplace_back(filePath, std::move(kmlData));
    }*/
  }
  return collection;
}

std::shared_ptr<BookmarkManager::KMLDataCollection> BookmarkManager::LoadBookmarksKMB(std::vector<std::string> & filePaths)
{
  std::string const dir = GetBookmarksDirectory();
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, kBookmarksExt, files);

  auto collection = std::make_shared<KMLDataCollection>();
  collection->reserve(files.size());
  filePaths.reserve(files.size());

  for (auto const & file : files)
  {
    auto const filePath = my::JoinPath(dir, file);
    auto kmlData = std::make_unique<kml::FileData>();
    try
    {
      kml::binary::DeserializerKml des(*kmlData);
      FileReader reader(filePath);
      des.Deserialize(reader);
    }
    catch (FileReader::Exception const &exc)
    {
      LOG(LDEBUG, ("KML binary deserialization failure: ", exc.what(), "file", filePath));
      continue;
    }
    if (m_needTeardown)
      break;
    filePaths.push_back(filePath);
    collection->emplace_back(filePath, std::move(kmlData));
  }
  return collection;
}

void BookmarkManager::LoadBookmarks()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ClearCategories();
  m_loadBookmarksFinished = false;

  NotifyAboutStartAsyncLoading();
  GetPlatform().RunTask(Platform::Thread::File, [this]()
  {
    bool const migrated = migration::MigrateIfNeeded();
    std::vector<std::string> filePaths;
    auto collection = migrated ? LoadBookmarksKMB(filePaths) : LoadBookmarksKML(filePaths);
    if (m_needTeardown)
      return;
    NotifyAboutFinishAsyncLoading(std::move(collection));
    GetPlatform().RunTask(Platform::Thread::Gui,
                          [this, filePaths]() { m_bookmarkCloud.Init(filePaths); });
  });

  LoadState();
}

void BookmarkManager::LoadBookmark(std::string const & filePath, bool isTemporaryFile)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (!m_loadBookmarksFinished || m_asyncLoadingInProgress)
  {
    m_bookmarkLoadingQueue.emplace_back(filePath, isTemporaryFile);
    return;
  }
  LoadBookmarkRoutine(filePath, isTemporaryFile);
}

void BookmarkManager::LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile)
{
  ASSERT(!m_asyncLoadingInProgress, ());
  NotifyAboutStartAsyncLoading();
  GetPlatform().RunTask(Platform::Thread::File, [this, filePath, isTemporaryFile]()
  {
    bool const migrated = migration::IsMigrationCompleted();
    auto collection = std::make_shared<KMLDataCollection>();
    auto kmlData = std::make_unique<kml::FileData>();
    std::string fileSavePath;

    try
    {
      auto const savePath = GetKMLPath(filePath);
      if (m_needTeardown)
        return;
      if (savePath)
      {
        kml::DeserializerKml des(*kmlData);
        FileReader reader(fileSavePath);
        des.Deserialize(reader);
        fileSavePath = savePath.get();
      }
    }
    catch (FileReader::Exception const & exc)
    {
      LOG(LDEBUG, ("KML deserialization failure: ", exc.what(), "file", filePath));
    }
    if (m_needTeardown)
      return;

    if (migrated)
    {
      std::string fileName = fileSavePath;
      my::DeleteFileX(fileSavePath);
      my::GetNameFromFullPath(fileName);
      my::GetNameWithoutExt(fileName);
      fileSavePath = GenerateValidAndUniqueFilePathForKMB(fileName);

      try
      {
        kml::binary::SerializerKml ser(*kmlData);
        FileWriter writer(fileSavePath);
        ser.Serialize(writer);
      }
      catch (FileWriter::Exception const & exc)
      {
        my::DeleteFileX(fileSavePath);
        LOG(LDEBUG, ("KML binary serialization failure: ", exc.what(), "file", fileSavePath));
        fileSavePath.clear();
      }
    }
    if (m_needTeardown)
      return;

    if (!fileSavePath.empty())
      collection->emplace_back(fileSavePath, std::move(kmlData));
    NotifyAboutFile(!fileSavePath.empty() /* success */, filePath, isTemporaryFile);
    NotifyAboutFinishAsyncLoading(std::move(collection));

    if (!fileSavePath.empty())
    {
      // TODO(darina): should we use the cloud only for KMB?
      GetPlatform().RunTask(Platform::Thread::Gui,
                            [this, fileSavePath]() { m_bookmarkCloud.Init({fileSavePath}); });
    }
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

void BookmarkManager::NotifyAboutFinishAsyncLoading(std::shared_ptr<KMLDataCollection> && collection)
{
  if (m_needTeardown)
    return;
  
  GetPlatform().RunTask(Platform::Thread::Gui, [this, collection]()
  {
    m_asyncLoadingInProgress = false;
    m_loadBookmarksFinished = true;
    if (!collection->empty())
    {
      CreateCategories(std::move(*collection));
    }
    else
    {
      CheckAndResetLastIds();
      CheckAndCreateDefaultCategory();
    }

    if (m_asyncLoadingCallbacks.m_onFinished != nullptr)
      m_asyncLoadingCallbacks.m_onFinished();

    if (!m_bookmarkLoadingQueue.empty())
    {
      LoadBookmarkRoutine(m_bookmarkLoadingQueue.front().m_filename,
                          m_bookmarkLoadingQueue.front().m_isTemporaryFile);
      m_bookmarkLoadingQueue.pop_front();
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
  if (fileExt == BOOKMARKS_FILE_EXTENSION)
  {
    fileSavePath = GenerateValidAndUniqueFilePathForKML(GetFileName(filePath));
    if (!my::CopyFileX(filePath, fileSavePath))
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
        if (ext == BOOKMARKS_FILE_EXTENSION)
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

void BookmarkManager::MoveBookmark(df::MarkID bmID, df::MarkGroupID curGroupID, df::MarkGroupID newGroupID)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  DetachBookmark(bmID, curGroupID);
  AttachBookmark(bmID, newGroupID);

  SetLastEditedBmCategory(newGroupID);
}

void BookmarkManager::UpdateBookmark(df::MarkID bmID, kml::BookmarkData const & bm)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto * bookmark = GetBookmarkForEdit(bmID);

  auto const prevColor = bookmark->GetColor();
  bookmark->SetData(bm);
  ASSERT(bookmark->GetGroupId() != df::kInvalidMarkGroupId, ());

  if (prevColor != bookmark->GetColor())
    SetLastEditedBmColor(bookmark->GetColor());
}

df::MarkGroupID BookmarkManager::LastEditedBMCategory()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

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
  CheckAndCreateDefaultCategory();
  return m_bmGroupsIdList.front();
}

kml::PredefinedColor BookmarkManager::LastEditedBMColor() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return (m_lastColor != kml::PredefinedColor::None ? m_lastColor : BookmarkCategory::GetDefaultColor());
}

void BookmarkManager::SetLastEditedBmCategory(df::MarkGroupID groupId)
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

BookmarkCategory const * BookmarkManager::GetBmCategory(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

BookmarkCategory * BookmarkManager::GetBmCategory(df::MarkGroupID categoryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : nullptr);
}

void BookmarkManager::SendBookmarksChanges()
{
  if (m_callbacks.m_createdBookmarksCallback != nullptr)
  {
    std::vector<std::pair<df::MarkID, kml::BookmarkData>> marksInfo;
    GetBookmarksData(m_changesTracker.GetCreatedMarkIds(), marksInfo);
    m_callbacks.m_createdBookmarksCallback(marksInfo);
  }
  if (m_callbacks.m_updatedBookmarksCallback != nullptr)
  {
    std::vector<std::pair<df::MarkID, kml::BookmarkData>> marksInfo;
    GetBookmarksData(m_changesTracker.GetUpdatedMarkIds(), marksInfo);
    m_callbacks.m_updatedBookmarksCallback(marksInfo);
  }
  if (m_callbacks.m_deletedBookmarksCallback != nullptr)
  {
    df::MarkIDCollection idCollection;
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

void BookmarkManager::GetBookmarksData(df::MarkIDSet const & markIds,
                                       std::vector<std::pair<df::MarkID, kml::BookmarkData>> & data) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  data.clear();
  data.reserve(markIds.size());
  for (auto markId : markIds)
  {
    auto const * bookmark = GetBookmark(markId);
    if (bookmark)
      data.emplace_back(markId, bookmark->GetData());
  }
}

bool BookmarkManager::HasBmCategory(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return m_categories.find(groupId) != m_categories.end();
}

df::MarkGroupID BookmarkManager::CreateBookmarkCategory(kml::CategoryData const & data, bool autoSave)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto groupId = data.m_id;
  if (groupId == kml::kInvalidCategoryId)
  {
    groupId = ++m_lastGroupID;
    SaveLastBmCategoryId(m_lastGroupID);
  }
  ASSERT_EQUAL(m_categories.count(groupId), 0, ());
  auto & cat = m_categories[groupId];
  cat = my::make_unique<BookmarkCategory>(data, groupId, autoSave);
  m_bmGroupsIdList.push_back(groupId);
  m_changesTracker.OnAddGroup(groupId);
  return groupId;
}

df::MarkGroupID BookmarkManager::CreateBookmarkCategory(std::string const & name, bool autoSave)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto const groupId = ++m_lastGroupID;
  SaveLastBmCategoryId(m_lastGroupID);
  auto & cat = m_categories[groupId];
  cat = my::make_unique<BookmarkCategory>(name, groupId, autoSave);
  m_bmGroupsIdList.push_back(groupId);
  m_changesTracker.OnAddGroup(groupId);
  return groupId;
}

void BookmarkManager::CheckAndCreateDefaultCategory()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (m_categories.empty())
    CreateBookmarkCategory(m_callbacks.m_getStringsBundle().GetString("core_my_places"));
}

void BookmarkManager::CheckAndResetLastIds()
{
  if (m_categories.empty())
    m_lastGroupID = ResetLastBmCategoryId();
  if (m_bookmarks.empty())
    UserMark::ResetLastId(UserMark::BOOKMARK);
  if (m_tracks.empty())
    Track::ResetLastId();
}

bool BookmarkManager::DeleteBmCategory(df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto it = m_categories.find(groupId);
  if (it == m_categories.end())
    return false;

  ClearGroup(groupId);
  m_changesTracker.OnDeleteGroup(groupId);

  FileWriter::DeleteFileX(it->second->GetFileName());

  m_categories.erase(it);
  m_bmGroupsIdList.erase(std::remove(m_bmGroupsIdList.begin(), m_bmGroupsIdList.end(), groupId),
                         m_bmGroupsIdList.end());
  return true;
}

namespace
{
class BestUserMarkFinder
{
public:
  explicit BestUserMarkFinder(BookmarkManager::TTouchRectHolder const & rectHolder, BookmarkManager const * manager)
    : m_rectHolder(rectHolder)
    , m_d(numeric_limits<double>::max())
    , m_mark(nullptr)
    , m_manager(manager)
  {}

  void operator()(df::MarkGroupID groupId)
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
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  BestUserMarkFinder finder(holder, this);
  finder(UserMark::Type::ROUTING);
  finder(UserMark::Type::SEARCH);
  finder(UserMark::Type::API);
  for (auto & pair : m_categories)
    finder(pair.first);

  return finder.GetFoundMark();
}

UserMarkLayer const * BookmarkManager::GetGroup(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::COUNT)
    return m_userMarkLayers[groupId - 1].get();

  ASSERT(m_categories.find(groupId) != m_categories.end(), ());
  return m_categories.at(groupId).get();
}

UserMarkLayer * BookmarkManager::GetGroup(df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::COUNT)
    return m_userMarkLayers[groupId - 1].get();

  auto const it = m_categories.find(groupId);
  return it != m_categories.end() ? it->second.get() : nullptr;
}

void BookmarkManager::CreateCategories(KMLDataCollection && dataCollection, bool autoSave)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  df::GroupIDSet loadedGroups;

  std::vector<std::pair<df::MarkGroupID, BookmarkCategory *>> categoriesForMerge;
  categoriesForMerge.reserve(m_categories.size());
  for (auto const & c : m_categories)
    categoriesForMerge.emplace_back(c.first, c.second.get());

  for (auto const & data : dataCollection)
  {
    df::MarkGroupID groupId;
    BookmarkCategory * group = nullptr;

    auto const & fileName = data.first;
    auto const * fileData = data.second.get();
    auto const & categoryData = fileData->m_categoryData;

    auto const it = std::find_if(categoriesForMerge.cbegin(), categoriesForMerge.cend(),
      [categoryData](auto const & v)
      {
        return v.second->GetName() == kml::GetDefaultStr(categoryData.m_name);
      });
    bool const merge = it != categoriesForMerge.cend();
    if (merge)
    {
      groupId = it->first;
      group = it->second;
    }
    else
    {
      bool const saveAfterCreation = categoryData.m_id == df::kInvalidMarkGroupId;
      groupId = CreateBookmarkCategory(categoryData, saveAfterCreation);
      loadedGroups.insert(groupId);
      group = GetBmCategory(groupId);
      group->SetFileName(fileName);
    }

    for (auto & bmData : fileData->m_bookmarksData)
    {
      auto * bm = CreateBookmark(bmData);
      bm->Attach(groupId);
      group->AttachUserMark(bm->GetId());
    }
    for (auto & trackData : fileData->m_tracksData)
    {
      auto track = make_unique<Track>(trackData);
      auto * t = AddTrack(std::move(track));
      t->Attach(groupId);
      group->AttachTrack(t->GetId());
    }
    if (merge)
    {
      // Delete file since it will be merged.
      my::DeleteFileX(fileName);
      SaveBookmarks({groupId});
    }
  }

  NotifyChanges();

  for (auto const & groupId : loadedGroups)
  {
    auto * group = GetBmCategory(groupId);
    group->EnableAutoSave(autoSave);
  }
}

std::unique_ptr<kml::FileData> BookmarkManager::CollectBmGroupKMLData(BookmarkCategory const * group) const
{
  auto kmlData = std::make_unique<kml::FileData>();
  kmlData->m_categoryData = group->GetCategoryData();
  auto const & markIds = group->GetUserMarks();
  kmlData->m_bookmarksData.reserve(markIds.size());
  for (auto it = markIds.rbegin(); it != markIds.rend(); ++it)
  {
    Bookmark const *bm = GetBookmark(*it);
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

bool BookmarkManager::SaveBookmarkCategory(df::MarkGroupID groupId)
{
  auto collection = PrepareToSaveBookmarks({groupId});
  if (!collection || collection->empty())
    return false;
  auto const & file = collection->front().first;
  auto & kmlData = *collection->front().second;
  return SaveKMLData(file, kmlData, migration::IsMigrationCompleted());
}

void BookmarkManager::SaveToFile(df::MarkGroupID groupId, Writer & writer, bool useBinary) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto * group = GetBmCategory(groupId);
  auto kmlData = CollectBmGroupKMLData(group);
  SaveToFile(*kmlData, writer, useBinary);
}

void BookmarkManager::SaveToFile(kml::FileData & kmlData, Writer & writer, bool useBinary) const
{
  if (useBinary)
  {
    kml::binary::SerializerKml ser(kmlData);
    ser.Serialize(writer);
  }
  else
  {
    kml::SerializerKml ser(kmlData);
    ser.Serialize(writer);
  }
}

std::shared_ptr<BookmarkManager::KMLDataCollection> BookmarkManager::PrepareToSaveBookmarks(
  df::GroupIDCollection const & groupIdCollection)
{
  bool migrated = migration::IsMigrationCompleted();

  std::string const fileDir = migrated ? GetBookmarksDirectory() : GetPlatform().SettingsDir();
  std::string const fileExt = migrated ? kBookmarksExt : BOOKMARKS_FILE_EXTENSION;

  if (migrated && !GetPlatform().IsFileExistsByFullPath(fileDir) && !GetPlatform().MkDirChecked(fileDir))
    return std::shared_ptr<KMLDataCollection>();

  auto collection = std::make_shared<KMLDataCollection>();

  for (auto const groupId : groupIdCollection)
  {
    auto * group = GetBmCategory(groupId);

    // Get valid file name from category name
    std::string const name = RemoveInvalidSymbols(group->GetName());
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

bool BookmarkManager::SaveKMLData(std::string const & file, kml::FileData & kmlData, bool useBinary)
{
  auto const fileTmp = file + ".tmp";
  try
  {
    FileWriter writer(fileTmp);
    SaveToFile(kmlData, writer, useBinary);

    // Only after successful save we replace original file
    my::DeleteFileX(file);
    VERIFY(my::RenameFileX(fileTmp, file), (fileTmp, file));
    return true;
  }
  catch (FileWriter::Exception const & exc)
  {
    LOG(LDEBUG, ("KML serialization failure:", exc.what(), "file", fileTmp));
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Exception while saving bookmarks:", e.what(), "file", file));
  }

  // remove possibly left tmp file
  my::DeleteFileX(fileTmp);
  return false;
}

void BookmarkManager::SaveBookmarks(df::GroupIDCollection const & groupIdCollection)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  auto kmlDataCollection = PrepareToSaveBookmarks(groupIdCollection);
  if (!kmlDataCollection)
    return;

  bool const migrated = migration::IsMigrationCompleted();
  GetPlatform().RunTask(Platform::Thread::File, [this, migrated, kmlDataCollection]()
  {
    for (auto const & kmlItem : *kmlDataCollection)
      SaveKMLData(kmlItem.first, *kmlItem.second, migrated);
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

void BookmarkManager::SetInvalidTokenHandler(Cloud::InvalidTokenHandler && onInvalidToken)
{
  m_bookmarkCloud.SetInvalidTokenHandler(std::move(onInvalidToken));
}

void BookmarkManager::PrepareFileForSharing(df::MarkGroupID categoryId, SharingHandler && handler)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(handler, ());
  if (IsCategoryEmpty(categoryId))
  {
    handler(SharingResult(categoryId, SharingResult::Code::EmptyCategory));
    return;
  }

  auto const filePath = GetCategoryFileName(categoryId);
  GetPlatform().RunTask(Platform::Thread::File, [categoryId, filePath, handler = std::move(handler)]()
  {
    handler(GetFileForSharing(categoryId, filePath));
  });
}

bool BookmarkManager::IsCategoryEmpty(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->IsEmpty();
}

bool BookmarkManager::IsUsedCategoryName(std::string const & name) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & c : m_categories)
  {
    if (c.second->GetName() == name)
      return true;
  }
  return false;
}

bool BookmarkManager::AreAllCategoriesVisible() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & c : m_categories)
  {
    if (!c.second->IsVisible())
      return false;
  }
  return true;
}

bool BookmarkManager::AreAllCategoriesInvisible() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto const & c : m_categories)
  {
    if (c.second->IsVisible())
      return false;
  }
  return true;
}

void BookmarkManager::SetAllCategoriesVisibility(bool visible)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto & c : m_categories)
    c.second->SetIsVisible(visible);
}

size_t BookmarkManager::GetKmlFilesCountForConversion() const
{
  // The conversion available only after successful migration.
  if (!migration::IsMigrationCompleted())
    return 0;

  Platform::FilesList files;
  Platform::GetFilesByExt(GetPlatform().SettingsDir(),
                          BOOKMARKS_FILE_EXTENSION, files);
  return files.size();
}

void BookmarkManager::ConvertAllKmlFiles(ConversionHandler && handler) const
{
  // The conversion available only after successful migration.
  if (!migration::IsMigrationCompleted())
    return;

  GetPlatform().RunTask(Platform::Thread::File, [handler = std::move(handler)]()
  {
    auto const oldDir = GetPlatform().SettingsDir();
    Platform::FilesList files;
    Platform::GetFilesByExt(oldDir, BOOKMARKS_FILE_EXTENSION, files);
    for (auto & f : files)
      f = my::JoinFoldersToPath(oldDir, f);

    auto const newDir = GetBookmarksDirectory();
    if (!GetPlatform().IsFileExistsByFullPath(newDir) && !GetPlatform().MkDirChecked(newDir))
    {
      handler(false /* success */);
      return;
    }

    std::vector<std::pair<std::string, kml::FileData>> fileData;
    fileData.reserve(files.size());
    for (auto const & f : files)
    {
      std::string fileName = f;
      my::GetNameFromFullPath(fileName);
      my::GetNameWithoutExt(fileName);
      auto kmbPath = my::JoinPath(newDir, fileName + kBookmarksExt);
      size_t counter = 1;
      while (Platform::IsFileExistsByFullPath(kmbPath))
        kmbPath = my::JoinPath(newDir, fileName + strings::to_string(counter++) + kBookmarksExt);

      kml::FileData kmlData;
      try
      {
        kml::DeserializerKml des(kmlData);
        FileReader reader(f);
        des.Deserialize(reader);
      }
      catch (FileReader::Exception const & exc)
      {
        LOG(LDEBUG, ("KML text deserialization failure: ", exc.what(), "file", f));
        handler(false /* success */);
        return;
      }

      try
      {
        kml::binary::SerializerKml ser(kmlData);
        FileWriter writer(kmbPath);
        ser.Serialize(writer);
      }
      catch (FileWriter::Exception const & exc)
      {
        my::DeleteFileX(kmbPath);
        LOG(LDEBUG, ("KML binary serialization failure: ", exc.what(), "file", f));
        handler(false /* success */);
        return;
      }

      fileData.emplace_back(kmbPath, std::move(kmlData));
    }

    for (auto const & f : files)
      my::DeleteFileX(f);

    //TODO(@darina): add fileData to m_categories.

    handler(true /* success */);
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

    if (type == Cloud::SynchronizationType::Restore &&
        result == Cloud::SynchronizationResult::Success)
    {
      // Reload bookmarks after restoring.
      LoadBookmarks();
    }
  });

  LOG(LINFO, ("Cloud Synchronization Finished:", type, result, errorStr));
}

void BookmarkManager::OnRestoreRequested(Cloud::RestoringRequestResult result,
                                         uint64_t backupTimestampInMs)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, result, backupTimestampInMs]()
  {
    if (m_onRestoreRequested)
      m_onRestoreRequested(result, backupTimestampInMs);
  });

  using namespace std::chrono;
  LOG(LINFO, ("Cloud Restore Requested:", result,
              time_point<system_clock>(milliseconds(backupTimestampInMs))));
}

void BookmarkManager::OnRestoredFilesPrepared()
{
  // This method is always called from UI-thread.
  ClearCategories();

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

df::GroupIDSet BookmarkManager::MarksChangesTracker::GetAllGroupIds() const
{
  auto const & groupIds = m_bmManager.GetBmGroupsIdList();
  df::GroupIDSet resultingSet(groupIds.begin(), groupIds.end());
  for (uint32_t i = 1; i < UserMark::COUNT; ++i)
    resultingSet.insert(static_cast<df::MarkGroupID>(i));
  return resultingSet;
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisible(df::MarkGroupID groupId) const
{
  return m_bmManager.IsVisible(groupId);
}

bool BookmarkManager::MarksChangesTracker::IsGroupVisibilityChanged(df::MarkGroupID groupId) const
{
  return m_bmManager.GetGroup(groupId)->IsVisibilityChanged();
}

df::MarkIDSet const & BookmarkManager::MarksChangesTracker::GetGroupPointIds(df::MarkGroupID groupId) const
{
  return m_bmManager.GetUserMarkIds(groupId);
}

df::LineIDSet const & BookmarkManager::MarksChangesTracker::GetGroupLineIds(df::MarkGroupID groupId) const
{
  return m_bmManager.GetTrackIds(groupId);
}

df::UserPointMark const * BookmarkManager::MarksChangesTracker::GetUserPointMark(df::MarkID markId) const
{
  return m_bmManager.GetMark(markId);
}

df::UserLineMark const * BookmarkManager::MarksChangesTracker::GetUserLineMark(df::LineID lineId) const
{
  return m_bmManager.GetTrack(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddMark(df::MarkID markId)
{
  m_createdMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteMark(df::MarkID markId)
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

void BookmarkManager::MarksChangesTracker::OnUpdateMark(df::MarkID markId)
{
  if (m_createdMarks.find(markId) == m_createdMarks.end())
    m_updatedMarks.insert(markId);
}

void BookmarkManager::MarksChangesTracker::OnAddLine(df::LineID lineId)
{
  m_createdLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteLine(df::LineID lineId)
{
  auto const it = m_createdLines.find(lineId);
  if (it != m_createdLines.end())
    m_createdLines.erase(it);
  else
    m_removedLines.insert(lineId);
}

void BookmarkManager::MarksChangesTracker::OnAddGroup(df::MarkGroupID groupId)
{
  m_createdGroups.insert(groupId);
}

void BookmarkManager::MarksChangesTracker::OnDeleteGroup(df::MarkGroupID groupId)
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
std::string BookmarkManager::RemoveInvalidSymbols(std::string const & name)
{
  // Remove not allowed symbols
  strings::UniString uniName = strings::MakeUniString(name);
  uniName.erase_if(&IsBadCharForPath);
  return (uniName.empty() ? "Bookmarks" : strings::ToUtf8(uniName));
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
  while (Platform::IsFileExistsByFullPath(my::JoinPath(path, name + suffix + kmlExt)))
    suffix = strings::to_string(counter++);
  return my::JoinPath(path, name + suffix + kmlExt);
}

// static
std::string BookmarkManager::GenerateValidAndUniqueFilePathForKML(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName);
  return GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath, BOOKMARKS_FILE_EXTENSION);
}

// static
std::string BookmarkManager::GenerateValidAndUniqueFilePathForKMB(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName);
  return GenerateUniqueFileName(GetBookmarksDirectory(), filePath, kBookmarksExt);
}

// static
bool BookmarkManager::IsMigrated()
{
  return migration::IsMigrationCompleted();
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

Bookmark * BookmarkManager::EditSession::CreateBookmark(kml::BookmarkData const & bm)
{
  return m_bmManager.CreateBookmark(bm);
}

Bookmark * BookmarkManager::EditSession::CreateBookmark(kml::BookmarkData & bm, df::MarkGroupID groupId)
{
  return m_bmManager.CreateBookmark(bm, groupId);
}

Track * BookmarkManager::EditSession::CreateTrack(kml::TrackData const & trackData)
{
  return m_bmManager.CreateTrack(trackData);
}

Bookmark * BookmarkManager::EditSession::GetBookmarkForEdit(df::MarkID markId)
{
  return m_bmManager.GetBookmarkForEdit(markId);
}

void BookmarkManager::EditSession::DeleteUserMark(df::MarkID markId)
{
  m_bmManager.DeleteUserMark(markId);
}

void BookmarkManager::EditSession::DeleteBookmark(df::MarkID bmId)
{
  m_bmManager.DeleteBookmark(bmId);
}

void BookmarkManager::EditSession::DeleteTrack(df::LineID trackId)
{
  m_bmManager.DeleteTrack(trackId);
}

void BookmarkManager::EditSession::ClearGroup(df::MarkGroupID groupId)
{
  m_bmManager.ClearGroup(groupId);
}

void BookmarkManager::EditSession::SetIsVisible(df::MarkGroupID groupId, bool visible)
{
  m_bmManager.SetIsVisible(groupId, visible);
}

void BookmarkManager::EditSession::MoveBookmark(
  df::MarkID bmID, df::MarkGroupID curGroupID, df::MarkGroupID newGroupID)
{
  m_bmManager.MoveBookmark(bmID, curGroupID, newGroupID);
}

void BookmarkManager::EditSession::UpdateBookmark(df::MarkID bmId, kml::BookmarkData const & bm)
{
  return m_bmManager.UpdateBookmark(bmId, bm);
}

void BookmarkManager::EditSession::AttachBookmark(df::MarkID bmId, df::MarkGroupID groupId)
{
  m_bmManager.AttachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::DetachBookmark(df::MarkID bmId, df::MarkGroupID groupId)
{
  m_bmManager.DetachBookmark(bmId, groupId);
}

void BookmarkManager::EditSession::AttachTrack(df::LineID trackId, df::MarkGroupID groupId)
{
  m_bmManager.AttachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::DetachTrack(df::LineID trackId, df::MarkGroupID groupId)
{
  m_bmManager.DetachTrack(trackId, groupId);
}

void BookmarkManager::EditSession::SetCategoryName(df::MarkGroupID categoryId, std::string const & name)
{
  m_bmManager.SetCategoryName(categoryId, name);
}

bool BookmarkManager::EditSession::DeleteBmCategory(df::MarkGroupID groupId)
{
  return m_bmManager.DeleteBmCategory(groupId);
}

void BookmarkManager::EditSession::NotifyChanges()
{
  m_bmManager.NotifyChanges();
}
