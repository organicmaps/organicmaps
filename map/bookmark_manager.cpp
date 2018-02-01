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
#include "coding/zip_reader.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

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
    if (c < ' ' || illegalChars[i] == c)
      return true;

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

std::string RemoveInvalidSymbols(std::string const & name)
{
  // Remove not allowed symbols
  strings::UniString uniName = strings::MakeUniString(name);
  uniName.erase_if(&IsBadCharForPath);
  return (uniName.empty() ? "Bookmarks" : strings::ToUtf8(uniName));
}

std::string GenerateUniqueFileName(const std::string & path, std::string name)
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

std::string const GenerateValidAndUniqueFilePathForKML(std::string const & fileName)
{
  std::string filePath = RemoveInvalidSymbols(fileName);
  filePath = GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath);
  return filePath;
}

}  // namespace

BookmarkManager::BookmarkManager(Callbacks && callbacks)
  : m_callbacks(std::move(callbacks))
  , m_bookmarksListeners(std::bind(&BookmarkManager::OnCreateUserMarks, this, _1, _2),
                         std::bind(&BookmarkManager::OnUpdateUserMarks, this, _1, _2),
                         std::bind(&BookmarkManager::OnDeleteUserMarks, this, _1, _2))
  , m_needTeardown(false)
  , m_nextGroupID(UserMark::BOOKMARK)
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());
  m_userMarkLayers.reserve(UserMark::BOOKMARK);
  for (size_t i = 0; i < UserMark::BOOKMARK; ++i)
    m_userMarkLayers.emplace_back(std::make_unique<UserMarkContainer>(static_cast<UserMark::Type>(i)));

  m_selectionMark = CreateUserMark<StaticMarkPoint>(m2::PointD{});
  m_myPositionMark = CreateUserMark<MyPositionMarkPoint>(m2::PointD{});
}

BookmarkManager::~BookmarkManager()
{
  ClearCategories();
}

////////////////////////////

UserMark const * BookmarkManager::GetUserMark(df::MarkID markID) const
{
  auto it = m_userMarks.find(markID);
  return (it != m_userMarks.end()) ? it->second.get() : nullptr;
}

UserMark * BookmarkManager::GetUserMarkForEdit(df::MarkID markID)
{
  auto it = m_userMarks.find(markID);
  if (it == m_userMarks.end())
    return nullptr;
  auto const groupId = static_cast<df::MarkGroupID>(it->second->GetMarkType());
  m_userMarkLayers[groupId]->EditUserMark(markID);
  return it->second.get();
}

void BookmarkManager::DeleteUserMark(df::MarkID markId)
{
  auto it = m_userMarks.find(markId);
  auto const groupId = it->second->GetGroupId();
  FindContainer(groupId)->DetachUserMark(markId);
  m_userMarks.erase(it);
}

Bookmark * BookmarkManager::CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bmData)
{
  return AddBookmark(std::make_unique<Bookmark>(bmData, ptOrg));
}

Bookmark * BookmarkManager::CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bm, df::MarkGroupID groupID)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(df::GetDrawTileScale(m_viewport));

  auto * bookmark = CreateBookmark(ptOrg, bm);
  bookmark->Attach(groupID);
  auto * group = GetBmCategory(groupID);
  group->AttachUserMark(bookmark->GetId());
  group->SetIsVisible(true);
  SaveToKMLFile(groupID);
  NotifyChanges(groupID);

  m_lastCategoryUrl = group->GetFileName();
  m_lastType = bm.GetType();
  SaveState();

  return bookmark;
}

Bookmark const * BookmarkManager::GetBookmark(df::MarkID markID) const
{
  auto it = m_bookmarks.find(markID);
  return (it != m_bookmarks.end()) ? it->second.get() : nullptr;
}

Bookmark * BookmarkManager::GetBookmarkForEdit(df::MarkID markID)
{
  auto it = m_bookmarks.find(markID);
  if (it == m_bookmarks.end())
    return nullptr;
  auto const groupId = it->second->GetGroupId();
  if (groupId)
    GetBmCategory(groupId)->EditUserMark(markID);
  return it->second.get();
}

void BookmarkManager::AttachBookmark(df::MarkID bmId, df::MarkGroupID catID)
{
  GetBookmarkForEdit(bmId)->Attach(catID);
  FindContainer(catID)->AttachUserMark(bmId);
}

void BookmarkManager::DetachBookmark(df::MarkID bmId, df::MarkGroupID catID)
{
  GetBookmarkForEdit(bmId)->Detach();
  FindContainer(catID)->DetachUserMark(bmId);
}

void BookmarkManager::DeleteBookmark(df::MarkID bmId)
{
  auto it = m_bookmarks.find(bmId);
  auto const groupID = it->second->GetGroupId();
  if (groupID)
    FindContainer(groupID)->DetachUserMark(bmId);
  m_bookmarks.erase(it);
}

Track * BookmarkManager::CreateTrack(m2::PolylineD const & polyline, Track::Params const & p)
{
  return AddTrack(std::make_unique<Track>(polyline, p));
}

Track const * BookmarkManager::GetTrack(df::MarkID trackID) const
{
  auto it = m_tracks.find(trackID);
  return (it != m_tracks.end()) ? it->second.get() : nullptr;
}

void BookmarkManager::AttachTrack(df::MarkID trackID, df::MarkGroupID groupID)
{
  auto it = m_tracks.find(trackID);
  it->second->Attach(groupID);
  GetBmCategory(groupID)->AttachTrack(trackID);
}

void BookmarkManager::DetachTrack(df::MarkID trackID, df::MarkGroupID groupID)
{
  GetBmCategory(groupID)->DetachTrack(trackID);
}

void BookmarkManager::DeleteTrack(df::MarkID trackID)
{
  auto it = m_tracks.find(trackID);
  auto const groupID = it->second->GetGroupId();
  if (groupID)
    GetBmCategory(groupID)->DetachTrack(trackID);
  m_tracks.erase(it);
}

void BookmarkManager::NotifyChanges(df::MarkGroupID groupId)
{
  auto * group = FindContainer(groupId);
  if (!group->IsDirty())
    return;

  group->NotifyListeners();

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return;

  auto engine = lock.Get();

  engine->ChangeVisibilityUserMarksGroup(groupId, group->IsVisible());

  if (group->GetUserPointCount() == 0 && group->GetUserLineCount() == 0)
  {
    engine->UpdateUserMarksGroup(groupId, this);
    engine->ClearUserMarksGroup(groupId);
  }
  else if (group->IsVisible())
  {
    engine->UpdateUserMarksGroup(groupId, this);
  }

  engine->InvalidateUserMarks();
}

BookmarkManager::MarkIDSet const & BookmarkManager::GetUserMarkIds(df::MarkGroupID groupID) const
{
  return FindContainer(groupID)->GetUserMarks();
}

BookmarkManager::MarkIDSet const & BookmarkManager::GetTrackIds(df::MarkGroupID groupID) const
{
  return GetBmCategory(groupID)->GetTracks();
}

void BookmarkManager::ClearUserMarks(df::MarkGroupID groupId)
{
  auto * group = FindContainer(groupId);
  for (auto markId : group->GetUserMarks())
  {
    if (IsBookmark(groupId))
      m_bookmarks.erase(markId);
    else
      m_userMarks.erase(markId);
  }
  group->Clear();
}

std::string const & BookmarkManager::GetCategoryName(df::MarkGroupID categoryId) const
{
  return GetBmCategory(categoryId)->GetName();
}

void BookmarkManager::SetCategoryName(df::MarkGroupID categoryId, std::string const & name)
{
  GetBmCategory(categoryId)->SetName(name);
}

std::string const & BookmarkManager::GetCategoryFileName(df::MarkGroupID categoryId) const
{
  return GetBmCategory(categoryId)->GetFileName();
}

UserMark const * BookmarkManager::FindMarkInRect(df::MarkGroupID groupID, m2::AnyRectD const & rect, double & d) const
{
  auto const * group = FindContainer(groupID);

  UserMark const * mark = nullptr;
  if (group->IsVisible())
  {
    FindMarkFunctor f(&mark, d, rect);
    for (auto markId : group->GetUserMarks())
    {
      auto const * mark = IsBookmark(groupID)
        ? static_cast<UserMark const *>(GetBookmark(markId))
        : GetUserMark(markId);
      if (mark->IsAvailableForSearch() && rect.IsPointInside(mark->GetPivot()))
        f(mark);
    }
  }
  return mark;
}

void BookmarkManager::SetIsVisible(df::MarkGroupID categoryId, bool visible)
{
  return FindContainer(categoryId)->SetIsVisible(visible);
}

bool BookmarkManager::IsVisible(df::MarkGroupID categoryId) const
{
  return FindContainer(categoryId)->IsVisible();
}

////////////////////////////

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
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
  auto * bm = bookmark.get();
  auto const markId = bm->GetId();
  ASSERT(m_bookmarks.count(markId) == 0, ());
  m_bookmarks.emplace(markId, std::move(bookmark));
  return bm;
}

Track * BookmarkManager::AddTrack(std::unique_ptr<Track> && track)
{
  auto * t = track.get();
  auto const trackId = t->GetId();
  ASSERT(m_tracks.count(trackId) == 0, ());
  m_tracks.emplace(trackId, std::move(track));
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
  m_categories.clear();
  m_bmGroupsIdList.clear();
}

void BookmarkManager::LoadBookmarks()
{
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
    for (auto const & file : files)
    {
      auto kmlData = LoadKMLFile(dir + file);
      if (m_needTeardown)
        return;

      if (kmlData != nullptr)
        collection->emplace_back(std::move(kmlData));
    }
    NotifyAboutFinishAsyncLoading(std::move(collection));
  });

  LoadState();
}

void BookmarkManager::LoadBookmark(std::string const & filePath, bool isTemporaryFile)
{
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

void BookmarkManager::InitBookmarks()
{
  for (auto & cat : m_categories)
    NotifyChanges(cat.first);
}

void BookmarkManager::MoveBookmark(df::MarkID bmID, df::MarkGroupID curGroupID, df::MarkGroupID newGroupID)
{
  DetachBookmark(bmID, curGroupID);
  SaveToKMLFile(curGroupID);
  NotifyChanges(curGroupID);
  AttachBookmark(bmID, newGroupID);
}

void BookmarkManager::UpdateBookmark(df::MarkID bmID, BookmarkData const & bm)
{
  auto * bookmark = GetBookmarkForEdit(bmID);
  bookmark->SetData(bm);
  ASSERT(bookmark->GetGroupId(), ());
  SaveToKMLFile(bookmark->GetGroupId());
  NotifyChanges(bookmark->GetGroupId());

  m_lastType = bm.GetType();
  SaveState();
}

size_t BookmarkManager::LastEditedBMCategory()
{
  for (auto & cat : m_categories)
  {
    if (cat.second->GetFileName() == m_lastCategoryUrl)
      return cat.first;
  }

  if (m_categories.empty())
    CreateBmCategory(m_callbacks.m_getStringsBundle().GetString("my_places"));

  return m_bmGroupsIdList.front();
}

std::string BookmarkManager::LastEditedBMType() const
{
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(df::MarkGroupID categoryId) const
{
  ASSERT(categoryId >= UserMark::BOOKMARK, ());
  auto const it = m_categories.find(categoryId);
  return (it != m_categories.end() ? it->second.get() : 0);
}

void BookmarkManager::OnCreateUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds)
{
  if (container.GetType() != UserMark::Type::BOOKMARK)
    return;

  if (m_callbacks.m_createdBookmarksCallback == nullptr)
    return;

  std::vector<std::pair<df::MarkID, BookmarkData>> marksInfo;
  GetBookmarksData(markIds, marksInfo);

  m_callbacks.m_createdBookmarksCallback(marksInfo);
}

void BookmarkManager::OnUpdateUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds)
{
  if (container.GetType() != UserMark::Type::BOOKMARK)
    return;

  if (m_callbacks.m_updatedBookmarksCallback == nullptr)
    return;

  std::vector<std::pair<df::MarkID, BookmarkData>> marksInfo;
  GetBookmarksData(markIds, marksInfo);

  m_callbacks.m_updatedBookmarksCallback(marksInfo);
}

void BookmarkManager::OnDeleteUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds)
{
  if (container.GetType() != UserMark::Type::BOOKMARK)
    return;

  if (m_callbacks.m_deletedBookmarksCallback == nullptr)
    return;

  m_callbacks.m_deletedBookmarksCallback(markIds);
}

void BookmarkManager::GetBookmarksData(df::IDCollection const & markIds,
                                       std::vector<std::pair<df::MarkID, BookmarkData>> & data) const
{
  data.clear();
  data.reserve(markIds.size());
  for (auto markId : markIds)
  {
    auto const * bookmark = GetBookmark(markId);
    ASSERT(bookmark != nullptr, ());
    data.push_back(std::make_pair(markId, bookmark->GetData()));
  }
}

bool BookmarkManager::HasBmCategory(df::MarkGroupID groupID) const
{
  return m_categories.find(groupID) != m_categories.end();
}

df::MarkGroupID BookmarkManager::CreateBmCategory(std::string const & name)
{
  auto const groupId = m_nextGroupID++;
  auto & cat = m_categories[groupId];
  cat = my::make_unique<BookmarkCategory>(name, groupId, m_bookmarksListeners);
  m_bmGroupsIdList.push_back(groupId);
  return groupId;
}

bool BookmarkManager::DeleteBmCategory(df::MarkGroupID groupID)
{
  auto it = m_categories.find(groupID);
  if (it == m_categories.end())
    return false;

  BookmarkCategory & group = *it->second.get();
  // TODO(darina): check the necessity of DeleteLater
  //  cat.DeleteLater();
  FileWriter::DeleteFileX(group.GetFileName());
  for (auto markId : group.GetUserMarks())
    m_bookmarks.erase(markId);
  for (auto trackId : group.GetTracks())
    m_tracks.erase(trackId);
  it->second->Clear();
  NotifyChanges(groupID);
  m_categories.erase(it);
  m_bmGroupsIdList.erase(std::remove(m_bmGroupsIdList.begin(), m_bmGroupsIdList.end(), groupID),
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

  void operator()(df::MarkGroupID groupID)
  {
    m2::AnyRectD const & rect = m_rectHolder(min((UserMark::Type)groupID, UserMark::BOOKMARK));
    if (UserMark const * p = m_manager->FindMarkInRect(groupID, rect, m_d))
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
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder) const
{
  BestUserMarkFinder finder(holder, this);
  finder(UserMark::Type::ROUTING);
  finder(UserMark::Type::SEARCH);
  finder(UserMark::Type::API);
  for (auto & pair : m_categories)
    finder(pair.first);

  return finder.GetFoundMark();
}

UserMarkContainer const * BookmarkManager::FindContainer(df::MarkGroupID containerId) const
{
  if (containerId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[containerId].get();
  else
  {
    ASSERT(m_categories.find(containerId) != m_categories.end(), ());
    return m_categories.at(containerId).get();
  }
}

UserMarkContainer * BookmarkManager::FindContainer(df::MarkGroupID containerId)
{
  if (containerId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[containerId].get();
  else
  {
    auto const it = m_categories.find(containerId);
    return it != m_categories.end() ? it->second.get() : 0;
  }
}

void BookmarkManager::AcceptChanges(df::MarkGroupID groupID,
                                    df::MarkIDCollection & groupMarks,
                                    df::MarkIDCollection & createdMarks,
                                    df::MarkIDCollection & removedMarks)
{
  FindContainer(groupID)->AcceptChanges(groupMarks, createdMarks, removedMarks);
}

df::UserPointMark const * BookmarkManager::GetUserPointMark(df::MarkID markID) const
{
  df::UserPointMark const * mark = GetUserMark(markID);
  if (!mark)
    mark = GetBookmark(markID);
  return mark;
}

df::UserLineMark const * BookmarkManager::GetUserLineMark(df::MarkID markID) const
{
  return GetTrack(markID);
}

void BookmarkManager::CreateCategories(KMLDataCollection && dataCollection)
{
  for (auto & data : dataCollection)
  {
    df::MarkGroupID groupID;
    BookmarkCategory * group = nullptr;

    auto const it = std::find_if(m_categories.begin(), m_categories.end(),
      [&data](CategoriesCollection::value_type const & v)
    {
      return v.second->GetName() == data->m_name;
    });
    bool merge = it != m_categories.end();
    if (merge)
    {
      groupID = it->first;
      group = it->second.get();
    }
    else
    {
      groupID = CreateBmCategory(data->m_name);
      group = GetBmCategory(groupID);
      group->SetFileName(data->m_file);
      group->SetIsVisible(data->m_visible);
    }
    for (auto & bookmark : data->m_bookmarks)
    {
      auto * bm = AddBookmark(std::move(bookmark));
      bm->Attach(groupID);
      group->AttachUserMark(bm->GetId());
    }
    for (auto & track : data->m_tracks)
    {
      auto * t = AddTrack(std::move(track));
      t->Attach(groupID);
      group->AttachTrack(t->GetId());
    }
    if (merge)
    {
      SaveToKMLFile(groupID);
      // Delete file since it has been merged.
      // TODO(darina): why not delete the file before saving it?
      my::DeleteFileX(data->m_file);
    }
  }

  for (auto & cat : m_categories)
    NotifyChanges(cat.first);
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

void BookmarkManager::SaveToKML(BookmarkCategory * group, std::ostream & s)
{
  s << kmlHeader;

  // Use CDATA if we have special symbols in the name
  s << "  <name>";
  SaveStringWithCDATA(s, group->GetName());
  s << "</name>\n";

  s << "  <visibility>" << (group->IsVisible() ? "1" : "0") <<"</visibility>\n";

  for (auto markId : group->GetUserMarks())
  {
    Bookmark const * bm = GetBookmark(markId);
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
  for (auto trackId : group->GetTracks())
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

bool BookmarkManager::SaveToKMLFile(df::MarkGroupID groupID)
{
  std::string oldFile;

  auto * group = GetBmCategory(groupID);

  // Get valid file name from category name
  std::string const name = RemoveInvalidSymbols(group->GetName());
  std::string file = group->GetFileName();

  if (!file.empty())
  {
    size_t i2 = file.find_last_of('.');
    if (i2 == std::string::npos)
      i2 = file.size();
    size_t i1 = file.find_last_of("\\/");
    if (i1 == std::string::npos)
      i1 = 0;
    else
      ++i1;

    // If m_file doesn't match name, assign new m_file for this category and save old file name.
    if (file.substr(i1, i2 - i1).find(name) != 0)
    {
      file.swap(oldFile);
    }
  }
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
      // Only after successfull save we replace original file
      my::DeleteFileX(file);
      VERIFY(my::RenameFileX(fileTmp, file), (fileTmp, file));
      // delete old file
      if (!oldFile.empty())
        VERIFY(my::DeleteFileX(oldFile), (oldFile, file));

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

  // return old file name in case of error
  if (!oldFile.empty())
    group->SetFileName(oldFile);

  return false;
}
