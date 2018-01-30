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
{
  ASSERT(m_callbacks.m_getStringsBundle != nullptr, ());

  m_userMarkLayers.emplace_back(my::make_unique<SearchUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<ApiUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<DebugUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<RouteUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<LocalAdsMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<TransitMarkContainer>());

  auto staticMarksContainer = my::make_unique<StaticUserMarkContainer>();
  m_selectionMark = my::make_unique<StaticMarkPoint>(this);
  m_myPositionMark = my::make_unique<MyPositionMarkPoint>(this);

  m_userMarkLayers.emplace_back(std::move(staticMarksContainer));
}

BookmarkManager::~BookmarkManager()
{
  m_userMarkLayers.clear();

  ClearCategories();
}

////////////////////////////
float BookmarkManager::GetPointDepth(UserMark::Type type, size_t index) const
{
  if (type == UserMark::Type::BOOKMARK)
  {
    return GetBmCategory(index)->GetPointDepth();
  }
  else
  {
    return FindUserMarksContainer(type)->GetPointDepth();
  }
}

void BookmarkManager::NotifyChanges(UserMark::Type type, size_t index)
{
  if (type == UserMark::Type::BOOKMARK)
  {
    GetBmCategory(index)->NotifyChanges();
  }
  else
  {
    GetUserMarksController(type).NotifyChanges();
  }
}

size_t BookmarkManager::GetUserMarkCount(UserMark::Type type) const
{
  return GetUserMarksController(type).GetUserMarkCount();
}

UserMark const * BookmarkManager::GetUserMark(UserMark::Type type, size_t index) const
{
  return GetUserMarksController(type).GetUserMark(index);
}

UserMark * BookmarkManager::GetUserMarkForEdit(UserMark::Type type, size_t index)
{
  return GetUserMarksController(type).GetUserMarkForEdit(index);
}

void BookmarkManager::DeleteUserMark(UserMark::Type type, size_t index)
{
  GetUserMarksController(type).DeleteUserMark(index);
}

void BookmarkManager::ClearUserMarks(UserMark::Type type)
{
  GetUserMarksController(type).Clear();
}

size_t BookmarkManager::GetBookmarksCount(size_t categoryIndex) const
{
  return GetBmCategory(categoryIndex)->GetUserMarkCount();
}

Bookmark const * BookmarkManager::GetBookmarkTmp(size_t categoryIndex, size_t bmIndex) const
{
  return static_cast<Bookmark const *>(GetBmCategory(categoryIndex)->GetUserMark(bmIndex));
}

Bookmark * BookmarkManager::GetBookmarkForEditTmp(size_t categoryIndex, size_t bmIndex)
{
  return static_cast<Bookmark *>(GetBmCategory(categoryIndex)->GetUserMarkForEdit(bmIndex));
}

void BookmarkManager::DeleteBookmark(size_t categoryIndex, size_t bmIndex)
{
  GetBmCategory(categoryIndex)->DeleteUserMark(bmIndex);
}

size_t BookmarkManager::GetTracksCount(size_t categoryIndex) const
{
  return GetBmCategory(categoryIndex)->GetTracksCount();
}

Track const * BookmarkManager::GetTrack(size_t categoryIndex, size_t index) const
{
  return GetBmCategory(categoryIndex)->GetTrack(index);
}

void BookmarkManager::DeleteTrack(size_t categoryIndex, size_t index)
{
  return GetBmCategory(categoryIndex)->DeleteTrack(index);
}

bool BookmarkManager::SaveToKMLFile(size_t categoryIndex)
{
  return GetBmCategory(categoryIndex)->SaveToKMLFile();
}

std::string const & BookmarkManager::GetCategoryName(size_t categoryIndex) const
{
  return GetBmCategory(categoryIndex)->GetName();
}

void BookmarkManager::SetCategoryName(size_t categoryIndex, std::string const & name)
{
  GetBmCategory(categoryIndex)->SetName(name);
}

std::string const & BookmarkManager::GetCategoryFileName(size_t categoryIndex) const
{
  return GetBmCategory(categoryIndex)->GetFileName();
}

UserMark const * BookmarkManager::FindMarkInRect(UserMark::Type type, size_t index, m2::AnyRectD const & rect, double & d) const
{
  return type == UserMark::Type::BOOKMARK ? GetBmCategory(index)->FindMarkInRect(rect, d)
    : FindUserMarksContainer(type)->FindMarkInRect(rect, d);
}

UserMark * BookmarkManager::CreateUserMark(UserMark::Type type, m2::PointD const & ptOrg)
{
  return FindUserMarksContainer(type)->CreateUserMark(this, ptOrg);
}

UserMark * BookmarkManager::CreateBookmark(size_t index, m2::PointD const & ptOrg)
{
  return GetBmCategory(index)->CreateUserMark(this, ptOrg);
}

void BookmarkManager::SetContainerIsVisible(UserMark::Type type, bool visible)
{
  return FindUserMarksContainer(type)->SetIsVisible(visible);
}

void BookmarkManager::SetCategoryIsVisible(size_t categoryId, bool visible)
{
  return GetBmCategory(categoryId)->SetIsVisible(visible);
}

void BookmarkManager::SetContainerIsDrawable(UserMark::Type type, bool drawable)
{
  return FindUserMarksContainer(type)->SetIsDrawable(drawable);
}

bool BookmarkManager::IsVisible(size_t categoryId) const
{
  return GetBmCategory(categoryId)->IsVisible();
}

////////////////////////////

void BookmarkManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);

  for (auto & userMarkLayer : m_userMarkLayers)
    userMarkLayer->SetDrapeEngine(engine);

  for (auto & category : m_categories)
    category->SetDrapeEngine(engine);
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
    size_t index = 0;
    for (auto const & file : files)
    {
      auto cat = BookmarkCategory::CreateFromKMLFile(this, dir + file, index, m_bookmarksListeners);
      if (m_needTeardown)
        return;

      if (cat != nullptr)
      {
        collection->emplace_back(std::move(cat));
        ++index;
      }
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
      // XXX: will not work in case of several task work at the same time
      auto const index = collection->size();
      auto cat = BookmarkCategory::CreateFromKMLFile(this, fileSavePath.get(), index, m_bookmarksListeners);
      if (m_needTeardown)
        return;

      bool const categoryExists = (cat != nullptr);
      if (categoryExists)
        collection->emplace_back(std::move(cat));

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
    cat->NotifyChanges();
}

size_t BookmarkManager::AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(df::GetDrawTileScale(m_viewport));

  BookmarkCategory & cat = *m_categories[categoryIndex];

  auto bookmark = static_cast<Bookmark *>(cat.CreateUserMark(this, ptOrg));
  bookmark->SetData(bm);
  cat.SetIsVisible(true);
  cat.SaveToKMLFile();
  cat.NotifyChanges();

  m_lastCategoryUrl = cat.GetFileName();
  m_lastType = bm.GetType();
  SaveState();

  // Bookmark always is pushed front.
  return 0;
}

size_t BookmarkManager::MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex)
{
  BookmarkData data;
  m2::PointD ptOrg;

  BookmarkCategory * cat = GetBmCategory(curCatIndex);
  auto bm = static_cast<Bookmark const *>(cat->GetUserMark(bmIndex));
  data = bm->GetData();
  ptOrg = bm->GetPivot();

  cat->DeleteUserMark(bmIndex);
  cat->SaveToKMLFile();
  cat->NotifyChanges();

  return AddBookmark(newCatIndex, ptOrg, data);
}

void BookmarkManager::ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm)
{
  BookmarkCategory & cat = *m_categories[catIndex];
  static_cast<Bookmark *>(cat.GetUserMarkForEdit(bmIndex))->SetData(bm);
  cat.SaveToKMLFile();
  cat.NotifyChanges();

  m_lastType = bm.GetType();
  SaveState();
}

size_t BookmarkManager::LastEditedBMCategory()
{
  for (size_t i = 0; i < m_categories.size(); ++i)
  {
    if (m_categories[i]->GetFileName() == m_lastCategoryUrl)
      return i;
  }

  if (m_categories.empty())
    CreateBmCategory(m_callbacks.m_getStringsBundle().GetString("my_places"));

  return 0;
}

std::string BookmarkManager::LastEditedBMType() const
{
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(size_t index) const
{
  return (index < m_categories.size() ? m_categories[index].get() : 0);
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

size_t BookmarkManager::CreateBmCategory(std::string const & name)
{
  m_categories.emplace_back(new BookmarkCategory(name, m_categories.size(), m_bookmarksListeners));

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
    m_categories.back()->SetDrapeEngine(lock.Get());

  return (m_categories.size() - 1);
}

void BookmarkManager::DeleteBmCategory(CategoryIter it)
{
  BookmarkCategory & cat = *it->get();
  cat.DeleteLater();
  FileWriter::DeleteFileX(cat.GetFileName());
  m_categories.erase(it);
}

bool BookmarkManager::DeleteBmCategory(size_t index)
{
  if (index >= m_categories.size())
    return false;

  DeleteBmCategory(m_categories.begin() + index);
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

  void operator()(UserMark::Type type, size_t index)
  {
    m2::AnyRectD const & rect = m_rectHolder(type);
    if (UserMark const * p = m_manager->FindMarkInRect(type, index, rect, m_d))
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
    auto const mark = category->GetUserMarkById(id);
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
  for (size_t i = 0; i < m_categories.size(); ++i)
  {
    mark = m_categories[i]->GetUserMarkById(id, index);
    if (mark != nullptr)
    {
      catIndex = i;
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
  finder(UserMark::Type::ROUTING, 0);
  finder(UserMark::Type::SEARCH, 0);
  finder(UserMark::Type::API, 0);
  for (unsigned i = 0; i < m_categories.size(); ++i)
    finder(UserMark::Type::BOOKMARK, i);

  return finder.GetFoundMark();
}

bool BookmarkManager::UserMarksIsVisible(UserMark::Type type) const
{
  return FindUserMarksContainer(type)->IsVisible();
}

UserMarksController & BookmarkManager::GetUserMarksController(UserMark::Type type)
{
  return *FindUserMarksContainer(type);
}

UserMarksController const & BookmarkManager::GetUserMarksController(UserMark::Type type) const
{
  return *FindUserMarksContainer(type);
}

UserMarkContainer const * BookmarkManager::FindUserMarksContainer(UserMark::Type type) const
{
  auto const iter = std::find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(),
                                 [&type](std::unique_ptr<UserMarkContainer> const & cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return iter->get();
}

UserMarkContainer * BookmarkManager::FindUserMarksContainer(UserMark::Type type)
{
  auto iter = std::find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(),
                           [&type](std::unique_ptr<UserMarkContainer> const & cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return iter->get();
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
    std::string const categoryName = category->GetName();
    auto const it = std::find_if(newCategories.begin(), newCategories.end(),
                                 [&categoryName](std::unique_ptr<BookmarkCategory> const & c)
    {
      return c->GetName() == categoryName;
    });
    if (it == newCategories.end())
      continue;

    // Copy bookmarks and tracks to the existing category.
    for (size_t i = 0; i < (*it)->GetUserMarkCount(); ++i)
    {
      auto srcBookmark = static_cast<Bookmark const *>((*it)->GetUserMark(i));
      auto bookmark = static_cast<Bookmark *>(category->CreateUserMark(this, srcBookmark->GetPivot()));
      bookmark->SetData(srcBookmark->GetData());
    }
    category->AppendTracks((*it)->StealTracks());
    category->SaveToKMLFile();

    // Delete file since it has been merged.
    my::DeleteFileX((*it)->GetFileName());

    newCategories.erase(it);
  }

  std::move(newCategories.begin(), newCategories.end(), std::back_inserter(m_categories));

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (lock)
  {
    for (auto & cat : m_categories)
    {
      cat->SetDrapeEngine(lock.Get());
      cat->NotifyChanges();
    }
  }
}

UserMarkNotificationGuard::UserMarkNotificationGuard(BookmarkManager & mng, UserMark::Type type)
  : m_manager(mng),
    m_type(type)
{}

UserMarkNotificationGuard::~UserMarkNotificationGuard()
{
  ASSERT(m_type != UserMark::Type::BOOKMARK, ());
  m_manager.NotifyChanges(m_type, 0);
}
