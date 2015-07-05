#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/user_mark.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/scales.hpp"

#include "geometry/transformations.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

BookmarkManager::BookmarkManager(Framework & f)
  : m_framework(f)
{
  m_userMarkLayers.reserve(3);
  ///@TODO UVR
  m_userMarkLayers.push_back(new SearchUserMarkContainer(0.0/*graphics::activePinDepth*/, m_framework));
  m_userMarkLayers.push_back(new ApiUserMarkContainer(0.0/*graphics::activePinDepth*/, m_framework));
  //m_userMarkLayers.push_back(new DebugUserMarkContainer(graphics::debugDepth, m_framework));
  UserMarkContainer::InitStaticMarks(FindUserMarksContainer(UserMarkType::SEARCH_MARK));
}

BookmarkManager::~BookmarkManager()
{
  for_each(m_userMarkLayers.begin(), m_userMarkLayers.end(), DeleteFunctor());
  m_userMarkLayers.clear();

  ClearItems();
}

namespace
{
char const * BOOKMARK_CATEGORY = "LastBookmarkCategory";
char const * BOOKMARK_TYPE = "LastBookmarkType";
}

void BookmarkManager::SaveState() const
{
  Settings::Set(BOOKMARK_CATEGORY, m_lastCategoryUrl);
  Settings::Set(BOOKMARK_TYPE, m_lastType);
}

void BookmarkManager::LoadState()
{
  Settings::Get(BOOKMARK_CATEGORY, m_lastCategoryUrl);
  Settings::Get(BOOKMARK_TYPE, m_lastType);
}

void BookmarkManager::ClearItems()
{
  for_each(m_categories.begin(), m_categories.end(), DeleteFunctor());
  m_categories.clear();
}

void BookmarkManager::LoadBookmarks()
{
  ClearItems();

  string const dir = GetPlatform().SettingsDir();

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);
  for (size_t i = 0; i < files.size(); ++i)
    LoadBookmark(dir + files[i]);

  LoadState();
}

void BookmarkManager::LoadBookmark(string const & filePath)
{
  BookmarkCategory * cat = BookmarkCategory::CreateFromKMLFile(filePath, m_framework);
  if (cat)
    m_categories.push_back(cat);
}

size_t BookmarkManager::AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(m_framework.GetDrawScale());

  BookmarkCategory * pCat = m_categories[categoryIndex];

  BookmarkCategory::Guard guard(*pCat);
  static_cast<Bookmark *>(guard.m_controller.CreateUserMark(ptOrg))->SetData(bm);
  guard.m_controller.SetIsVisible(true);
  pCat->SaveToKMLFile();

  m_lastCategoryUrl = pCat->GetFileName();
  m_lastType = bm.GetType();
  SaveState();

  return (pCat->GetUserMarkCount() - 1);
}

size_t BookmarkManager::MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex)
{
  BookmarkCategory * cat = m_framework.GetBmCategory(curCatIndex);
  BookmarkCategory::Guard guard(*cat);
  Bookmark const * bm = static_cast<Bookmark const *>(guard.m_controller.GetUserMark(bmIndex));
  BookmarkData data = bm->GetData();
  m2::PointD ptOrg = bm->GetPivot();

  guard.m_controller.DeleteUserMark(bmIndex);
  cat->SaveToKMLFile();

  return AddBookmark(newCatIndex, ptOrg, data);
}

void BookmarkManager::ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm)
{
  BookmarkCategory * cat = m_categories[catIndex];
  BookmarkCategory::Guard guard(*cat);
  static_cast<Bookmark *>(guard.m_controller.GetUserMarkForEdit(bmIndex))->SetData(bm);
  cat->SaveToKMLFile();

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
    m_categories.push_back(new BookmarkCategory(m_framework.GetStringsBundle().GetString("my_places"), m_framework));

  return 0;
}

string BookmarkManager::LastEditedBMType() const
{
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(size_t index) const
{
  return (index < m_categories.size() ? m_categories[index] : 0);
}

size_t BookmarkManager::CreateBmCategory(string const & name)
{
  m_categories.push_back(new BookmarkCategory(name, m_framework));
  return (m_categories.size()-1);
}

void BookmarkManager::DeleteBmCategory(CategoryIter i)
{
  BookmarkCategory * cat = *i;
  m_categories.erase(i);
  cat->DeleteLater();
  FileWriter::DeleteFileX(cat->GetFileName());

  if (cat->CanBeDeleted())
    delete cat;
}

bool BookmarkManager::DeleteBmCategory(size_t index)
{
  if (index < m_categories.size())
  {
    DeleteBmCategory(m_categories.begin() + index);
    return true;
  }
  else
    return false;
}

namespace
{
  class BestUserMarkFinder
  {
  public:
    BestUserMarkFinder(BookmarkManager::TTouchRectHolder const & rectHolder)
      : m_rectHolder(rectHolder)
      , m_d(numeric_limits<double>::max())
      , m_mark(NULL) {}

    void operator() (UserMarkContainer const * container)
    {
      m2::AnyRectD const & rect = m_rectHolder(container->GetType());
      if (UserMark const * p = container->FindMarkInRect(rect, m_d))
        m_mark = p;
    }

    UserMark const * GetFindedMark() const { return m_mark; }

  private:
    BookmarkManager::TTouchRectHolder const & m_rectHolder;
    double m_d;
    UserMark const * m_mark;
  };
}

UserMark const * BookmarkManager::FindNearestUserMark(m2::AnyRectD const & rect) const
{
  return FindNearestUserMark([&rect](UserMarkType) -> m2::AnyRectD const & { return rect; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder) const
{
  BestUserMarkFinder finder(holder);
  for_each(m_categories.begin(), m_categories.end(), ref(finder));
  finder(FindUserMarksContainer(UserMarkType::API_MARK));
  finder(FindUserMarksContainer(UserMarkType::SEARCH_MARK));

  return finder.GetFindedMark();
}

bool BookmarkManager::UserMarksIsVisible(UserMarkType type) const
{
  return FindUserMarksContainer(type)->IsVisible();
}

UserMarksController & BookmarkManager::UserMarksRequestController(UserMarkType type)
{
  return FindUserMarksContainer(type)->RequestController();
}

void BookmarkManager::UserMarksReleaseController(UserMarksController & controller)
{
  FindUserMarksContainer(controller.GetType())->ReleaseController();
}

UserMarkContainer const * BookmarkManager::FindUserMarksContainer(UserMarkType type) const
{
  auto const iter = find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(), [&type](UserMarkContainer const * cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return *iter;
}

UserMarkContainer * BookmarkManager::FindUserMarksContainer(UserMarkType type)
{
  auto iter = find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(), [&type](UserMarkContainer * cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return *iter;
}

UserMarkControllerGuard::UserMarkControllerGuard(BookmarkManager & mng, UserMarkType type)
  : m_mng(mng)
  , m_controller(mng.UserMarksRequestController(type))
{
}

UserMarkControllerGuard::~UserMarkControllerGuard()
{
  m_mng.UserMarksReleaseController(m_controller);
}
