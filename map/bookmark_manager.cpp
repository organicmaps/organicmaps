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

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <algorithm>
#include <fstream>

using namespace std::placeholders;

namespace
{
char const * BOOKMARK_CATEGORY = "LastBookmarkCategory";
char const * BOOKMARK_TYPE = "LastBookmarkType";
char const * KMZ_EXTENSION = ".kmz";

// Returns extension with a dot in a lower case.
std::string const GetFileExt(std::string const & filePath)
{
  std::string ext = my::GetFileExtension(filePath);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

std::string const GetFileName(std::string const & filePath)
{
  std::string ret = filePath;
  my::GetNameFromFullPath(ret);
  return ret;
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
}  // namespace

BookmarkManager::BookmarkManager(Callbacks && callbacks)
  : m_callbacks(std::move(callbacks))
  , m_changesTracker(*this)
  , m_needTeardown(false)
  , m_nextGroupID(UserMark::BOOKMARK)
  , m_bookmarkCloud(Cloud::CloudParams("bmc.json", "bookmarks", "BookmarkCloudParam",
                                       std::string(KMZ_EXTENSION)))
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());
  m_userMarkLayers.reserve(UserMark::BOOKMARK);
  for (uint32_t i = 0; i < UserMark::BOOKMARK; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkLayer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD{});
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});

  m_bookmarkCloud.SetSynchronizationHandlers([]()
  {
    alohalytics::Stats::Instance().LogEvent("Bookmarks_sync_started");
  }, [](Cloud::SynchronizationResult result, std::string const & errorStr)
  {
    if (result == Cloud::SynchronizationResult::Success)
    {
      alohalytics::Stats::Instance().LogEvent("Bookmarks_sync_success");
    }
    else
    {
      std::string const typeStr = (result == Cloud::SynchronizationResult::DiskError) ? "disk" : "network";
      alohalytics::TStringMap details {{"type", typeStr}, {"error", errorStr}};
      alohalytics::Stats::Instance().LogEvent("Bookmarks_sync_error", details);
    }
  });
}

BookmarkManager::~BookmarkManager()
{
  ClearCategories();
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

Bookmark * BookmarkManager::CreateBookmark(m2::PointD const & ptOrg, BookmarkData const & bmData)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return AddBookmark(std::make_unique<Bookmark>(bmData, ptOrg));
}

Bookmark * BookmarkManager::CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bm, df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kBookmarksBookmarkAction,
                                                         {{"action", "create"}});

  bm.SetTimeStamp(time(0));
  bm.SetScale(df::GetDrawTileScale(m_viewport));

  auto * bookmark = CreateBookmark(ptOrg, bm);
  bookmark->Attach(groupId);
  auto * group = GetBmCategory(groupId);
  group->AttachUserMark(bookmark->GetId());
  group->SetIsVisible(true);

  m_lastCategoryUrl = group->GetFileName();
  m_lastType = bm.GetType();
  SaveState();

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

Track * BookmarkManager::CreateTrack(m2::PolylineD const & polyline, Track::Params const & p)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return AddTrack(std::make_unique<Track>(polyline, p));
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

  bool isBookmarks = false;
  for (auto groupId : m_changesTracker.GetDirtyGroupIds())
  {
    if (IsBookmarkCategory(groupId))
    {
      if (GetBmCategory(groupId)->IsAutoSaveEnabled())
        SaveToKMLFile(groupId);
      isBookmarks = true;
    }
  }
  if (isBookmarks)
    SendBookmarksChanges();

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

std::string const & BookmarkManager::GetCategoryName(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->GetName();
}

void BookmarkManager::SetCategoryName(df::MarkGroupID categoryId, std::string const & name)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  GetBmCategory(categoryId)->SetName(name);
}

std::string const & BookmarkManager::GetCategoryFileName(df::MarkGroupID categoryId) const
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
  return GetGroup(groupId)->SetIsVisible(visible);
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
  settings::Set(BOOKMARK_CATEGORY, m_lastCategoryUrl);
  settings::Set(BOOKMARK_TYPE, m_lastType);
}

void BookmarkManager::LoadState()
{
  UNUSED_VALUE(settings::Get(BOOKMARK_CATEGORY, m_lastCategoryUrl));
  UNUSED_VALUE(settings::Get(BOOKMARK_TYPE, m_lastType));
}

void BookmarkManager::ClearCategories()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  m_categories.clear();
  m_bmGroupsIdList.clear();
}

void BookmarkManager::LoadBookmarks()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ClearCategories();
  m_loadBookmarksFinished = false;

  NotifyAboutStartAsyncLoading();
  GetPlatform().RunTask(Platform::Thread::File, [this]()
  {
    std::string const dir = GetPlatform().SettingsDir();
    Platform::FilesList files;
    Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);

    auto collection = std::make_shared<KMLDataCollection>();
    collection->reserve(files.size());
    std::vector<std::string> filePaths;
    filePaths.reserve(files.size());
    for (auto const & file : files)
    {
      auto const filePath = dir + file;
      auto kmlData = LoadKMLFile(filePath);
      if (m_needTeardown)
        return;

      if (kmlData != nullptr)
      {
        filePaths.push_back(filePath);
        collection->emplace_back(std::move(kmlData));
      }
    }
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
    auto collection = std::make_shared<KMLDataCollection >();
    auto const fileSavePath = GetKMLPath(filePath);
    if (m_needTeardown)
      return;

    if (!fileSavePath)
    {
      NotifyAboutFile(false /* success */, filePath, isTemporaryFile);
    }
    else
    {
      auto kmlData = LoadKMLFile(fileSavePath.get());
      if (m_needTeardown)
        return;

      bool const dataExists = (kmlData != nullptr);
      if (dataExists)
        collection->emplace_back(std::move(kmlData));

      NotifyAboutFile(dataExists, filePath, isTemporaryFile);
    }
    NotifyAboutFinishAsyncLoading(std::move(collection));

    if (fileSavePath)
    {
      GetPlatform().RunTask(Platform::Thread::Gui,
                            [this, fileSavePath]() { m_bookmarkCloud.Init({fileSavePath.get()}); });
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
      CreateCategories(std::move(*collection));
    else
      CheckAndCreateDefaultCategory();

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
  else if (fileExt == KMZ_EXTENSION)
  {
    try
    {
      ZipFileReader::FileListT files;
      ZipFileReader::FilesList(filePath, files);
      std::string kmlFileName;
      for (size_t i = 0; i < files.size(); ++i)
      {
        if (GetFileExt(files[i].first) == BOOKMARKS_FILE_EXTENSION)
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
}

void BookmarkManager::UpdateBookmark(df::MarkID bmID, BookmarkData const & bm)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto * bookmark = GetBookmarkForEdit(bmID);
  bookmark->SetData(bm);
  ASSERT(bookmark->GetGroupId() != df::kInvalidMarkGroupId, ());

  m_lastType = bm.GetType();
  SaveState();
}

df::MarkGroupID BookmarkManager::LastEditedBMCategory()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (auto & cat : m_categories)
  {
    if (cat.second->GetFileName() == m_lastCategoryUrl)
      return cat.first;
  }
  CheckAndCreateDefaultCategory();
  return m_bmGroupsIdList.front();
}

std::string BookmarkManager::LastEditedBMType() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(IsBookmarkCategory(categoryId), ());
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : 0);
}

void BookmarkManager::SendBookmarksChanges()
{
  if (m_callbacks.m_createdBookmarksCallback != nullptr)
  {
    std::vector<std::pair<df::MarkID, BookmarkData>> marksInfo;
    GetBookmarksData(m_changesTracker.GetCreatedMarkIds(), marksInfo);
    m_callbacks.m_createdBookmarksCallback(marksInfo);
  }
  if (m_callbacks.m_updatedBookmarksCallback != nullptr)
  {
    std::vector<std::pair<df::MarkID, BookmarkData>> marksInfo;
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
                                       std::vector<std::pair<df::MarkID, BookmarkData>> & data) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  data.clear();
  data.reserve(markIds.size());
  for (auto markId : markIds)
  {
    auto const * bookmark = GetBookmark(markId);
    if (bookmark)
      data.push_back(std::make_pair(markId, bookmark->GetData()));
  }
}

bool BookmarkManager::HasBmCategory(df::MarkGroupID groupId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return m_categories.find(groupId) != m_categories.end();
}

df::MarkGroupID BookmarkManager::CreateBookmarkCategory(std::string const & name, bool autoSave)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto const groupId = m_nextGroupID++;
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
    m2::AnyRectD const & rect = m_rectHolder(min((UserMark::Type)groupId, UserMark::BOOKMARK));
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
  if (groupId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[groupId].get();

  ASSERT(m_categories.find(groupId) != m_categories.end(), ());
  return m_categories.at(groupId).get();
}

UserMarkLayer * BookmarkManager::GetGroup(df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (groupId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[groupId].get();

  auto const it = m_categories.find(groupId);
  return it != m_categories.end() ? it->second.get() : nullptr;
}

void BookmarkManager::CreateCategories(KMLDataCollection && dataCollection, bool autoSave)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  df::GroupIDSet loadedGroups;
  for (auto & data : dataCollection)
  {
    df::MarkGroupID groupId;
    BookmarkCategory * group = nullptr;

    auto const it = std::find_if(m_categories.begin(), m_categories.end(),
      [&data](CategoriesCollection::value_type const & v)
    {
      return v.second->GetName() == data->m_name;
    });
    bool merge = it != m_categories.end();
    if (merge)
    {
      groupId = it->first;
      group = it->second.get();
    }
    else
    {
      groupId = CreateBookmarkCategory(data->m_name, false /* autoSave */);
      loadedGroups.insert(groupId);

      group = GetBmCategory(groupId);
      group->SetFileName(data->m_file);
      group->SetIsVisible(data->m_visible);
    }
    for (auto & bookmark : data->m_bookmarks)
    {
      auto * bm = AddBookmark(std::move(bookmark));
      bm->Attach(groupId);
      group->AttachUserMark(bm->GetId());
    }
    for (auto & track : data->m_tracks)
    {
      auto * t = AddTrack(std::move(track));
      t->Attach(groupId);
      group->AttachTrack(t->GetId());
    }
    if (merge)
    {
      // Delete file since it will be merged.
      my::DeleteFileX(data->m_file);
      SaveToKMLFile(groupId);
    }
  }

  NotifyChanges();

  for (auto const & groupId : loadedGroups)
  {
    auto * group = GetBmCategory(groupId);
    group->EnableAutoSave(autoSave);
  }
}

namespace
{
char const * kmlHeader =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
    "<Document>\n"
    "  <Style id=\"placemark-blue\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-blue.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-brown\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-brown.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-green\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-green.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-orange\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-orange.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-pink\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-pink.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-purple\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-purple.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-red\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-red.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-yellow\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-yellow.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
;

char const * kmlFooter =
  "</Document>\n"
    "</kml>\n";
}

namespace
{
inline void SaveStringWithCDATA(std::ostream & stream, std::string const & s)
{
  // According to kml/xml spec, we need to escape special symbols with CDATA
  if (s.find_first_of("<&") != std::string::npos)
    stream << "<![CDATA[" << s << "]]>";
  else
    stream << s;
}

std::string PointToString(m2::PointD const & org)
{
  double const lon = MercatorBounds::XToLon(org.x);
  double const lat = MercatorBounds::YToLat(org.y);

  ostringstream ss;
  ss.precision(8);

  ss << lon << "," << lat;
  return ss.str();
}
}

void BookmarkManager::SaveToKML(df::MarkGroupID groupId, std::ostream & s)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  SaveToKML(GetBmCategory(groupId), s);
}

void BookmarkManager::SaveToKML(BookmarkCategory * group, std::ostream & s)
{
  s << kmlHeader;

  // Use CDATA if we have special symbols in the name
  s << "  <name>";
  SaveStringWithCDATA(s, group->GetName());
  s << "</name>\n";

  s << "  <visibility>" << (group->IsVisible() ? "1" : "0") <<"</visibility>\n";

  // Bookmarks are stored to KML file in reverse order, so, least recently added bookmark
  // will be stored last. The reason is that when bookmarks will be loaded from the KML file,
  // most recently added bookmark will be loaded last and in accordance with current logic
  // will be added to the beginning of the bookmarks list.
  // Thus, this method preserves LRU bookmarks order after store -> load actions.
  auto const & markIds = group->GetUserMarks();
  for (auto it = markIds.rbegin(); it != markIds.rend(); ++it)
  {
    Bookmark const * bm = GetBookmark(*it);
    s << "  <Placemark>\n";
    s << "    <name>";
    SaveStringWithCDATA(s, bm->GetName());
    s << "</name>\n";

    if (!bm->GetDescription().empty())
    {
      s << "    <description>";
      SaveStringWithCDATA(s, bm->GetDescription());
      s << "</description>\n";
    }

    time_t const timeStamp = bm->GetTimeStamp();
    if (timeStamp != my::INVALID_TIME_STAMP)
    {
      std::string const strTimeStamp = my::TimestampToString(timeStamp);
      ASSERT_EQUAL(strTimeStamp.size(), 20, ("We always generate fixed length UTC-format timestamp"));
      s << "    <TimeStamp><when>" << strTimeStamp << "</when></TimeStamp>\n";
    }

    s << "    <styleUrl>#" << bm->GetType() << "</styleUrl>\n"
      << "    <Point><coordinates>" << PointToString(bm->GetPivot()) << "</coordinates></Point>\n";

    double const scale = bm->GetScale();
    if (scale != -1.0)
    {
      /// @todo Factor out to separate function to use for other custom params.
      s << "    <ExtendedData xmlns:mwm=\"http://mapswith.me\">\n"
        << "      <mwm:scale>" << bm->GetScale() << "</mwm:scale>\n"
        << "    </ExtendedData>\n";
    }

    s << "  </Placemark>\n";
  }

  // Saving tracks
  for (auto trackId : group->GetUserLines())
  {
    Track const * track = GetTrack(trackId);

    s << "  <Placemark>\n";
    s << "    <name>";
    SaveStringWithCDATA(s, track->GetName());
    s << "</name>\n";

    ASSERT_GREATER(track->GetLayerCount(), 0, ());

    s << "<Style><LineStyle>";
    dp::Color const & col = track->GetColor(0);
    s << "<color>"
      << NumToHex(col.GetAlpha())
      << NumToHex(col.GetBlue())
      << NumToHex(col.GetGreen())
      << NumToHex(col.GetRed());
    s << "</color>\n";

    s << "<width>"
      << track->GetWidth(0);
    s << "</width>\n";

    s << "</LineStyle></Style>\n";
    // stop style saving

    s << "    <LineString><coordinates>";

    Track::PolylineD const & poly = track->GetPolyline();
    for (auto pt = poly.Begin(); pt != poly.End(); ++pt)
      s << PointToString(*pt) << " ";

    s << "    </coordinates></LineString>\n"
      << "  </Placemark>\n";
  }

  s << kmlFooter;
}

bool BookmarkManager::SaveToKMLFile(df::MarkGroupID groupId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  std::string oldFile;

  auto * group = GetBmCategory(groupId);

  // Get valid file name from category name
  std::string const name = RemoveInvalidSymbols(group->GetName());
  std::string file = group->GetFileName();

  if (file.empty())
  {
    file = GenerateUniqueFileName(GetPlatform().SettingsDir(), name);
    group->SetFileName(file);
  }

  std::string const fileTmp = file + ".tmp";
  try
  {
    // First, we save to the temporary file
    /// @todo On Windows UTF-8 file names are not supported.
    std::ofstream of(fileTmp.c_str(), std::ios_base::out | std::ios_base::trunc);
    SaveToKML(group, of);
    of.flush();

    if (!of.fail())
    {
      // Only after successful save we replace original file
      my::DeleteFileX(file);
      VERIFY(my::RenameFileX(fileTmp, file), (fileTmp, file));

      return true;
    }
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Exception while saving bookmarks:", e.what()));
  }

  LOG(LWARNING, ("Can't save bookmarks category", name, "to file", file));

  // remove possibly left tmp file
  my::DeleteFileX(fileTmp);

  return false;
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

BookmarkManager::SharingResult BookmarkManager::BeginSharing(df::MarkGroupID categoryId)
{
  auto const it = m_activeSharing.find(categoryId);
  if (it != m_activeSharing.end())
  {
    // In case if the sharing has already begun.
    if (GetPlatform().IsFileExistsByFullPath(it->second))
      return SharingResult(it->second);
  }

  auto const result = GetFileForSharing(categoryId);
  if (result.m_code != SharingResult::Code::Success)
  {
    m_activeSharing.erase(categoryId);
    return result;
  }

  m_activeSharing[categoryId] = result.m_sharingPath;
  return result;
}

void BookmarkManager::EndSharing(df::MarkGroupID categoryId)
{
  auto const it = m_activeSharing.find(categoryId);
  if (it == m_activeSharing.end())
    return;

  if (GetPlatform().IsFileExistsByFullPath(it->second))
    my::DeleteFileX(it->second);

  m_activeSharing.erase(categoryId);
}

bool BookmarkManager::IsCategoryEmpty(df::MarkGroupID categoryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  return GetBmCategory(categoryId)->IsEmpty();
}

BookmarkManager::SharingResult BookmarkManager::GetFileForSharing(df::MarkGroupID categoryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (IsCategoryEmpty(categoryId))
    return SharingResult(SharingResult::Code::EmptyCategory);

  auto const filePath = GetCategoryFileName(categoryId);
  if (!GetPlatform().IsFileExistsByFullPath(filePath))
    return SharingResult(SharingResult::Code::FileError, "Bookmarks file does not exist.");

  auto ext = my::GetFileExtension(filePath);
  strings::AsciiToLower(ext);
  std::string fileName = filePath;
  my::GetNameFromFullPath(fileName);
  my::GetNameWithoutExt(fileName);
  auto const tmpFilePath = my::JoinFoldersToPath(GetPlatform().TmpDir(), fileName + KMZ_EXTENSION);
  if (ext == KMZ_EXTENSION)
  {
    if (my::CopyFileX(filePath, tmpFilePath))
      return SharingResult(tmpFilePath);

    return SharingResult(SharingResult::Code::FileError, "Could not copy file.");
  }

  if (!CreateZipFromPathDeflatedAndDefaultCompression(filePath, tmpFilePath))
    return SharingResult(SharingResult::Code::ArchiveError, "Could not create archive.");

  return SharingResult(tmpFilePath);
}

df::GroupIDSet BookmarkManager::MarksChangesTracker::GetAllGroupIds() const
{
  auto const & groupIds = m_bmManager.GetBmGroupsIdList();
  df::GroupIDSet resultingSet(groupIds.begin(), groupIds.end());
  for (uint32_t i = 0; i < UserMark::BOOKMARK; ++i)
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
  return !m_dirtyGroups.empty();
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
std::string BookmarkManager::GenerateUniqueFileName(const std::string & path, std::string name)
{
  std::string const kmlExt(BOOKMARKS_FILE_EXTENSION);

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
  while (Platform::IsFileExistsByFullPath(path + name + suffix + kmlExt))
    suffix = strings::to_string(counter++);
  return (path + name + suffix + kmlExt);
}

// static
std::string BookmarkManager::GenerateValidAndUniqueFilePathForKML(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName);
  filePath = GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath);
  return filePath;
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

Bookmark * BookmarkManager::EditSession::CreateBookmark(m2::PointD const & ptOrg, BookmarkData const & bm)
{
  return m_bmManager.CreateBookmark(ptOrg, bm);
}

Bookmark * BookmarkManager::EditSession::CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bm, df::MarkGroupID groupId)
{
  return m_bmManager.CreateBookmark(ptOrg, bm, groupId);
}

Track * BookmarkManager::EditSession::CreateTrack(m2::PolylineD const & polyline, Track::Params const & p)
{
  return m_bmManager.CreateTrack(polyline, p);
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

void BookmarkManager::EditSession::UpdateBookmark(df::MarkID bmId, BookmarkData const & bm)
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
