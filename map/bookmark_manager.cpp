#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/routing_mark.hpp"
#include "map/user_mark.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "indexer/scales.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

BookmarkManager::BookmarkManager(Framework & f)
  : m_framework(f)
{
  m_userMarkLayers.reserve(4);
  m_userMarkLayers.emplace_back(new SearchUserMarkContainer(0.0 /* activePinDepth */, m_framework));
  m_userMarkLayers.emplace_back(new ApiUserMarkContainer(0.0 /* activePinDepth */, m_framework));
  m_userMarkLayers.emplace_back(new DebugUserMarkContainer(0.0 /* debugDepth */, m_framework));
  m_userMarkLayers.emplace_back(new RouteUserMarkContainer(0.0 /* activePinDepth */, m_framework));
  UserMarkContainer::InitStaticMarks(FindUserMarksContainer(UserMarkType::SEARCH_MARK));
}

BookmarkManager::~BookmarkManager()
{
  m_userMarkLayers.clear();

  ClearCategories();
}

namespace
{
char const * BOOKMARK_CATEGORY = "LastBookmarkCategory";
char const * BOOKMARK_TYPE = "LastBookmarkType";
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

  string const dir = GetPlatform().SettingsDir();

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);
  for (size_t i = 0; i < files.size(); ++i)
    LoadBookmark(dir + files[i]);

  LoadState();
}

void BookmarkManager::LoadBookmark(string const & filePath)
{
  unique_ptr<BookmarkCategory> cat(BookmarkCategory::CreateFromKMLFile(filePath, m_framework));
  if (cat)
    m_categories.emplace_back(std::move(cat));
}

void BookmarkManager::InitBookmarks()
{
  for (auto & cat : m_categories)
  {
    BookmarkCategory::Guard guard(*cat);
  }
}

size_t BookmarkManager::AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(m_framework.GetDrawScale());

  BookmarkCategory & cat = *m_categories[categoryIndex];

  BookmarkCategory::Guard guard(cat);
  Bookmark * bookmark = static_cast<Bookmark *>(guard.m_controller.CreateUserMark(ptOrg));
  bookmark->SetData(bm);
  bookmark->SetCreationAnimationShown(false);
  guard.m_controller.SetIsVisible(true);
  cat.SaveToKMLFile();

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

  {
    BookmarkCategory * cat = m_framework.GetBmCategory(curCatIndex);
    BookmarkCategory::Guard guard(*cat);
    Bookmark const * bm = static_cast<Bookmark const *>(guard.m_controller.GetUserMark(bmIndex));
    data = bm->GetData();
    ptOrg = bm->GetPivot();

    guard.m_controller.DeleteUserMark(bmIndex);
    cat->SaveToKMLFile();
  }

  return AddBookmark(newCatIndex, ptOrg, data);
}

void BookmarkManager::ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm)
{
  BookmarkCategory & cat = *m_categories[catIndex];
  BookmarkCategory::Guard guard(cat);
  static_cast<Bookmark *>(guard.m_controller.GetUserMarkForEdit(bmIndex))->SetData(bm);
  cat.SaveToKMLFile();

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
    CreateBmCategory(m_framework.GetStringsBundle().GetString("my_places"));

  return 0;
}

string BookmarkManager::LastEditedBMType() const
{
  return (m_lastType.empty() ? BookmarkCategory::GetDefaultType() : m_lastType);
}

BookmarkCategory * BookmarkManager::GetBmCategory(size_t index) const
{
  return (index < m_categories.size() ? m_categories[index].get() : 0);
}

size_t BookmarkManager::CreateBmCategory(string const & name)
{
  m_categories.emplace_back(new BookmarkCategory(name, m_framework));
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
      {
        double const kEps = 1e-5;
        if (m_mark == nullptr || !p->GetPivot().EqualDxDy(m_mark->GetPivot(), kEps))
          m_mark = p;
      }
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
  finder(FindUserMarksContainer(UserMarkType::ROUTING_MARK));
  finder(FindUserMarksContainer(UserMarkType::SEARCH_MARK));
  finder(FindUserMarksContainer(UserMarkType::API_MARK));
  for (auto & cat : m_categories)
  {
    finder(cat.get());
  }

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
  auto const iter = find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(),
                            [&type](unique_ptr<UserMarkContainer> const & cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return iter->get();
}

UserMarkContainer * BookmarkManager::FindUserMarksContainer(UserMarkType type)
{
  auto iter = find_if(m_userMarkLayers.begin(), m_userMarkLayers.end(),
                      [&type](unique_ptr<UserMarkContainer> const & cont)
  {
    return cont->GetType() == type;
  });
  ASSERT(iter != m_userMarkLayers.end(), ());
  return iter->get();
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
