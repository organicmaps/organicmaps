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
#include "coding/internal/file_data.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

#include <algorithm>

using namespace std::placeholders;

namespace
{
char const * BOOKMARK_CATEGORY = "LastBookmarkCategory";
char const * BOOKMARK_TYPE = "LastBookmarkType";
char const * KMZ_EXTENSION = ".kmz";

using SearchUserMarkContainer = SpecifiedUserMarkContainer<SearchMarkPoint, UserMark::Type::SEARCH>;
using ApiUserMarkContainer = SpecifiedUserMarkContainer<ApiMarkPoint, UserMark::Type::API>;
using DebugUserMarkContainer = SpecifiedUserMarkContainer<DebugMarkPoint, UserMark::Type::DEBUG_MARK>;
using RouteUserMarkContainer = SpecifiedUserMarkContainer<RouteMarkPoint, UserMark::Type::ROUTING>;
using LocalAdsMarkContainer = SpecifiedUserMarkContainer<LocalAdsMark, UserMark::Type::LOCAL_ADS>;
using TransitMarkContainer = SpecifiedUserMarkContainer<TransitMark, UserMark::Type::TRANSIT>;
using StaticUserMarkContainer = SpecifiedUserMarkContainer<SearchMarkPoint, UserMark::Type::STATIC>;

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

std::string const GenerateValidAndUniqueFilePathForKML(std::string const & fileName)
{
  std::string filePath = BookmarkManager::RemoveInvalidSymbols(fileName);
  filePath = BookmarkManager::GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath);
  return filePath;
}

bool IsBadCharForPath(strings::UniChar const & c)
{
  static strings::UniChar const illegalChars[] = {':', '/', '\\', '<', '>', '\"', '|', '?', '*'};

  for (size_t i = 0; i < ARRAY_SIZE(illegalChars); ++i)
    if (c < ' ' || illegalChars[i] == c)
      return true;

  return false;
}
}  // namespace

std::string BookmarkManager::RemoveInvalidSymbols(std::string const & name)
{
  // Remove not allowed symbols
  strings::UniString uniName = strings::MakeUniString(name);
  uniName.erase_if(&IsBadCharForPath);
  return (uniName.empty() ? "Bookmarks" : strings::ToUtf8(uniName));
}

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

BookmarkManager::BookmarkManager(Callbacks && callbacks)
  : m_callbacks(std::move(callbacks))
  , m_bookmarksListeners(std::bind(&BookmarkManager::OnCreateUserMarks, this, _1, _2),
                         std::bind(&BookmarkManager::OnUpdateUserMarks, this, _1, _2),
                         std::bind(&BookmarkManager::OnDeleteUserMarks, this, _1, _2))
  , m_needTeardown(false)
  , m_nextCategoryId(UserMark::BOOKMARK)
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());
  m_userMarkLayers.resize(UserMark::PREDEFINED_COUNT);
  m_userMarkLayers[UserMark::API] = my::make_unique<ApiUserMarkContainer>();
  m_userMarkLayers[UserMark::SEARCH] = my::make_unique<SearchUserMarkContainer>();
  m_userMarkLayers[UserMark::STATIC] = my::make_unique<StaticUserMarkContainer>();
  m_userMarkLayers[UserMark::ROUTING] = my::make_unique<RouteUserMarkContainer>();
  m_userMarkLayers[UserMark::TRANSIT] = my::make_unique<TransitMarkContainer>();
  m_userMarkLayers[UserMark::LOCAL_ADS] = my::make_unique<LocalAdsMarkContainer>();
  m_userMarkLayers[UserMark::DEBUG_MARK] = my::make_unique<DebugUserMarkContainer>();

  m_selectionMark = my::make_unique<StaticMarkPoint>();
  m_myPositionMark = my::make_unique<MyPositionMarkPoint>();
}

BookmarkManager::~BookmarkManager()
{
  ClearCategories();
}

////////////////////////////
void BookmarkManager::NotifyChanges(size_t categoryId)
{
  FindContainer(categoryId)->NotifyChanges();
}

size_t BookmarkManager::GetUserMarkCount(size_t categoryId) const
{
  return FindContainer(categoryId)->GetUserMarkCount();
}

UserMark const * BookmarkManager::GetUserMark(size_t categoryId, size_t index) const
{
  return FindContainer(categoryId)->GetUserMark(index);
}

UserMark * BookmarkManager::GetUserMarkForEdit(size_t categoryId, size_t index)
{
  return FindContainer(categoryId)->GetUserMarkForEdit(index);
}

void BookmarkManager::DeleteUserMark(size_t categoryId, size_t index)
{
  FindContainer(categoryId)->DeleteUserMark(index);
}

void BookmarkManager::ClearUserMarks(size_t categoryId)
{
  FindContainer(categoryId)->Clear();
}

Bookmark const * BookmarkManager::GetBookmark(size_t categoryId, size_t bmIndex) const
{
  ASSERT(categoryId >= UserMark::BOOKMARK, ());
  return static_cast<Bookmark const *>(FindContainer(categoryId)->GetUserMark(bmIndex));
}

Bookmark * BookmarkManager::GetBookmarkForEdit(size_t categoryId, size_t bmIndex)
{
  ASSERT(categoryId >= UserMark::BOOKMARK, ());
  return static_cast<Bookmark *>(FindContainer(categoryId)->GetUserMarkForEdit(bmIndex));
}

size_t BookmarkManager::GetTracksCount(size_t categoryId) const
{
  return GetBmCategory(categoryId)->GetTracksCount();
}

Track const * BookmarkManager::GetTrack(size_t categoryId, size_t index) const
{
  return GetBmCategory(categoryId)->GetTrack(index);
}

void BookmarkManager::DeleteTrack(size_t categoryId, size_t index)
{
  return GetBmCategory(categoryId)->DeleteTrack(index);
}

bool BookmarkManager::SaveToKMLFile(size_t categoryId)
{
  return GetBmCategory(categoryId)->SaveToKMLFile();
}

std::string const & BookmarkManager::GetCategoryName(size_t categoryId) const
{
  return GetBmCategory(categoryId)->GetName();
}

void BookmarkManager::SetCategoryName(size_t categoryId, std::string const & name)
{
  GetBmCategory(categoryId)->SetName(name);
}

std::string const & BookmarkManager::GetCategoryFileName(size_t categoryId) const
{
  return GetBmCategory(categoryId)->GetFileName();
}

UserMark const * BookmarkManager::FindMarkInRect(size_t categoryId, m2::AnyRectD const & rect, double & d) const
{
  return FindContainer(categoryId)->FindMarkInRect(rect, d);
}

UserMark * BookmarkManager::CreateUserMark(size_t categoryId, m2::PointD const & ptOrg)
{
  return FindContainer(categoryId)->CreateUserMark(ptOrg);
}

void BookmarkManager::SetIsVisible(size_t categoryId, bool visible)
{
  return FindContainer(categoryId)->SetIsVisible(visible);
}

bool BookmarkManager::IsVisible(size_t categoryId) const
{
  return FindContainer(categoryId)->IsVisible();
}

////////////////////////////

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);

  for (auto & userMarkLayer : m_userMarkLayers)
    userMarkLayer->SetDrapeEngine(engine);

  for (auto & category : m_categories)
    category.second->SetDrapeEngine(engine);
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
  m_categoriesIdList.clear();
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

    auto collection = std::make_shared<CategoriesCollection>();
    for (auto const & file : files)
    {
      size_t const id = m_nextCategoryId++;
      auto cat = BookmarkCategory::CreateFromKMLFile(dir + file, id, m_bookmarksListeners);
      if (m_needTeardown)
        return;

      if (cat != nullptr)
        collection->emplace(id, std::move(cat));
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
    auto collection = std::make_shared<CategoriesCollection>();
    auto const fileSavePath = GetKMLPath(filePath);
    if (m_needTeardown)
      return;

    if (!fileSavePath)
    {
      NotifyAboutFile(false /* success */, filePath, isTemporaryFile);
    }
    else
    {
      auto const id = m_nextCategoryId++;
      auto cat = BookmarkCategory::CreateFromKMLFile(fileSavePath.get(), id, m_bookmarksListeners);
      if (m_needTeardown)
        return;

      bool const categoryExists = (cat != nullptr);
      if (categoryExists)
        collection->emplace(id, std::move(cat));

      NotifyAboutFile(categoryExists, filePath, isTemporaryFile);
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

void BookmarkManager::NotifyAboutFinishAsyncLoading(std::shared_ptr<CategoriesCollection> && collection)
{
  if (m_needTeardown)
    return;
  
  GetPlatform().RunTask(Platform::Thread::Gui, [this, collection]()
  {
    m_asyncLoadingInProgress = false;
    m_loadBookmarksFinished = true;
    if (!collection->empty())
      MergeCategories(std::move(*collection));

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
    cat.second->NotifyChanges();
}

size_t BookmarkManager::AddBookmark(size_t categoryId, m2::PointD const & ptOrg, BookmarkData & bm)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(df::GetDrawTileScale(m_viewport));

  BookmarkCategory * cat = GetBmCategory(categoryId);

  auto bookmark = static_cast<Bookmark *>(cat->CreateUserMark(ptOrg));
  bookmark->SetData(bm);
  cat->SetIsVisible(true);
  cat->SaveToKMLFile();
  cat->NotifyChanges();

  m_lastCategoryUrl = cat->GetFileName();
  m_lastType = bm.GetType();
  SaveState();

  // Bookmark always is pushed front.
  return 0;
}

size_t BookmarkManager::MoveBookmark(size_t bmIndex, size_t curCatId, size_t newCatId)
{
  BookmarkData data;
  m2::PointD ptOrg;

  BookmarkCategory * cat = GetBmCategory(curCatId);
  auto bm = static_cast<Bookmark const *>(cat->GetUserMark(bmIndex));
  data = bm->GetData();
  ptOrg = bm->GetPivot();

  cat->DeleteUserMark(bmIndex);
  cat->SaveToKMLFile();
  cat->NotifyChanges();

  return AddBookmark(newCatId, ptOrg, data);
}

void BookmarkManager::ReplaceBookmark(size_t categoryId, size_t bmIndex, BookmarkData const & bm)
{
  BookmarkCategory * cat = GetBmCategory(categoryId);
  static_cast<Bookmark *>(cat->GetUserMarkForEdit(bmIndex))->SetData(bm);
  cat->SaveToKMLFile();
  cat->NotifyChanges();

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

  return m_categoriesIdList.front();
}

std::string BookmarkManager::LastEditedBMType() const
{
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(size_t categoryId) const
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
  GetBookmarksData(container, markIds, marksInfo);

  m_callbacks.m_createdBookmarksCallback(marksInfo);
}

void BookmarkManager::OnUpdateUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds)
{
  if (container.GetType() != UserMark::Type::BOOKMARK)
    return;

  if (m_callbacks.m_updatedBookmarksCallback == nullptr)
    return;

  std::vector<std::pair<df::MarkID, BookmarkData>> marksInfo;
  GetBookmarksData(container, markIds, marksInfo);

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

void BookmarkManager::GetBookmarksData(UserMarkContainer const & container, df::IDCollection const & markIds,
                                       std::vector<std::pair<df::MarkID, BookmarkData>> & data) const
{
  data.clear();
  data.reserve(markIds.size());
  for (auto markId : markIds)
  {
    auto const userMark = container.GetUserMarkById(markId);
    ASSERT(userMark != nullptr, ());
    ASSERT(dynamic_cast<Bookmark const *>(userMark) != nullptr, ());

    auto const bookmark = static_cast<Bookmark const *>(userMark);
    data.push_back(std::make_pair(markId, bookmark->GetData()));
  }
}

bool BookmarkManager::HasBmCategory(size_t categoryId) const
{
  return m_categories.find(categoryId) != m_categories.end();
}

size_t BookmarkManager::CreateBmCategory(std::string const & name)
{
  size_t const id = m_nextCategoryId++;

  auto & cat = m_categories[id];
  cat = my::make_unique<BookmarkCategory>(name, id, m_bookmarksListeners);
  m_categoriesIdList.push_back(id);

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
    cat->SetDrapeEngine(lock.Get());

  return id;
}

bool BookmarkManager::DeleteBmCategory(size_t categoryId)
{
  auto it = m_categories.find(categoryId);
  if (it == m_categories.end())
    return false;

  BookmarkCategory & cat = *it->second.get();
  cat.DeleteLater();
  FileWriter::DeleteFileX(cat.GetFileName());
  m_categories.erase(it);
  m_categoriesIdList.erase(std::remove(m_categoriesIdList.begin(), m_categoriesIdList.end(), categoryId),
    m_categoriesIdList.end());
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

  void operator()(size_t categoryId)
  {
    m2::AnyRectD const & rect = m_rectHolder(min((UserMark::Type)categoryId, UserMark::BOOKMARK));
    if (UserMark const * p = m_manager->FindMarkInRect(categoryId, rect, m_d))
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

Bookmark const * BookmarkManager::GetBookmark(df::MarkID id) const
{
  for (auto const & category : m_categories)
  {
    auto const mark = category.second->GetUserMarkById(id);
    if (mark != nullptr)
    {
      ASSERT(dynamic_cast<Bookmark const *>(mark) != nullptr, ());
      return static_cast<Bookmark const *>(mark);
    }
  }
  return nullptr;
}

Bookmark const * BookmarkManager::GetBookmark(df::MarkID id, size_t & catIndex, size_t & bmIndex) const
{
  size_t index = 0;
  UserMark const * mark = nullptr;
  for (auto & it : m_categories)
  {
    mark = it.second->GetUserMarkById(id, index);
    if (mark != nullptr)
    {
      catIndex = it.first;
      bmIndex = index;
      ASSERT(dynamic_cast<Bookmark const *>(mark) != nullptr, ());
      return static_cast<Bookmark const *>(mark);
    }
  }
  return nullptr;
}

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
  for (auto & it : m_categories)
    finder(it.first);

  return finder.GetFoundMark();
}

UserMarkContainer const * BookmarkManager::FindContainer(size_t containerId) const
{
  if (containerId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[containerId].get();
  else
  {
    ASSERT(m_categories.find(containerId) != m_categories.end(), ());
    return m_categories.at(containerId).get();
  }
}

UserMarkContainer * BookmarkManager::FindContainer(size_t containerId)
{
  if (containerId < UserMark::Type::BOOKMARK)
    return m_userMarkLayers[containerId].get();
  else
  {
    auto const it = m_categories.find(containerId);
    return it != m_categories.end() ? it->second.get() : 0;
  }
}

std::unique_ptr<StaticMarkPoint> & BookmarkManager::SelectionMark()
{
  ASSERT(m_selectionMark != nullptr, ());
  return m_selectionMark;
}

std::unique_ptr<MyPositionMarkPoint> & BookmarkManager::MyPositionMark()
{
  ASSERT(m_myPositionMark != nullptr, ());
  return m_myPositionMark;
}

std::unique_ptr<StaticMarkPoint> const & BookmarkManager::SelectionMark() const
{
  ASSERT(m_selectionMark != nullptr, ());
  return m_selectionMark;
}

std::unique_ptr<MyPositionMarkPoint> const & BookmarkManager::MyPositionMark() const
{
  ASSERT(m_myPositionMark != nullptr, ());
  return m_myPositionMark;
}

void BookmarkManager::MergeCategories(CategoriesCollection && newCategories)
{
  for (auto & category : m_categories)
  {
    // Since all KML-files are being loaded asynchronously user can create
    // new category during loading. So we have to merge categories after loading.
    std::string const categoryName = category.second->GetName();
    auto const it = std::find_if(newCategories.begin(), newCategories.end(),
                                 [&categoryName](CategoriesCollection::value_type const & v)
    {
      return v.second->GetName() == categoryName;
    });
    if (it == newCategories.end())
      continue;
    auto * existingCat = category.second.get();
    auto * newCat = it->second.get();

    // Copy bookmarks and tracks to the existing category.
    for (size_t i = 0, sz = newCat->GetUserMarkCount(); i < sz; ++i)
    {
      auto srcBookmark = static_cast<Bookmark const *>(newCat->GetUserMark(i));
      auto bookmark = static_cast<Bookmark *>(existingCat->CreateUserMark(srcBookmark->GetPivot()));
      bookmark->SetData(srcBookmark->GetData());
    }
    existingCat->AppendTracks(newCat->StealTracks());
    existingCat->SaveToKMLFile();

    // Delete file since it has been merged.
    my::DeleteFileX(newCat->GetFileName());

    newCategories.erase(it);
  }

  for (auto & category : newCategories)
    m_categoriesIdList.push_back(category.first);
  m_categories.insert(make_move_iterator(newCategories.begin()),
                      make_move_iterator(newCategories.end()));

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    for (auto & cat : m_categories)
    {
      cat.second->SetDrapeEngine(lock.Get());
      cat.second->NotifyChanges();
    }
  }
}
