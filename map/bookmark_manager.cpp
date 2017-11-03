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

#include "coding/file_writer.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

#include <algorithm>

namespace
{
char const * BOOKMARK_CATEGORY = "LastBookmarkCategory";
char const * BOOKMARK_TYPE = "LastBookmarkType";

using SearchUserMarkContainer = SpecifiedUserMarkContainer<SearchMarkPoint, UserMark::Type::SEARCH>;
using ApiUserMarkContainer = SpecifiedUserMarkContainer<ApiMarkPoint, UserMark::Type::API>;
using DebugUserMarkContainer = SpecifiedUserMarkContainer<DebugMarkPoint, UserMark::Type::DEBUG_MARK>;
using RouteUserMarkContainer = SpecifiedUserMarkContainer<RouteMarkPoint, UserMark::Type::ROUTING>;
using LocalAdsMarkContainer = SpecifiedUserMarkContainer<LocalAdsMark, UserMark::Type::LOCAL_ADS>;
using StaticUserMarkContainer = SpecifiedUserMarkContainer<SearchMarkPoint, UserMark::Type::STATIC>;
}  // namespace

BookmarkManager::BookmarkManager(GetStringsBundleFn && getStringsBundleFn)
  : m_getStringsBundle(std::move(getStringsBundleFn))
{
  ASSERT(m_getStringsBundle != nullptr, ());

  m_userMarkLayers.reserve(6);
  m_userMarkLayers.emplace_back(my::make_unique<SearchUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<ApiUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<DebugUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<RouteUserMarkContainer>());
  m_userMarkLayers.emplace_back(my::make_unique<LocalAdsMarkContainer>());

  auto staticMarksContainer = my::make_unique<StaticUserMarkContainer>();
  m_selectionMark = my::make_unique<StaticMarkPoint>(staticMarksContainer.get());
  m_myPositionMark = my::make_unique<MyPositionMarkPoint>(staticMarksContainer.get());

  m_userMarkLayers.emplace_back(std::move(staticMarksContainer));
}

BookmarkManager::~BookmarkManager()
{
  m_userMarkLayers.clear();

  ClearCategories();
}

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

  std::string const dir = GetPlatform().SettingsDir();

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, BOOKMARKS_FILE_EXTENSION, files);
  for (auto const & file : files)
    LoadBookmark(dir + file);

  LoadState();
}

void BookmarkManager::LoadBookmark(std::string const & filePath)
{
  auto cat = BookmarkCategory::CreateFromKMLFile(filePath);
  if (cat != nullptr)
  {
    df::DrapeEngineLockGuard lock(m_drapeEngine);
    if (lock)
      cat->SetDrapeEngine(lock.Get());

    m_categories.emplace_back(std::move(cat));
  }
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

  auto bookmark = static_cast<Bookmark *>(cat.CreateUserMark(ptOrg));
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
    CreateBmCategory(m_getStringsBundle().GetString("my_places"));

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

size_t BookmarkManager::CreateBmCategory(std::string const & name)
{
  m_categories.emplace_back(new BookmarkCategory(name));
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
  explicit BestUserMarkFinder(BookmarkManager::TTouchRectHolder const & rectHolder)
    : m_rectHolder(rectHolder)
    , m_d(numeric_limits<double>::max())
    , m_mark(nullptr)
  {}

  void operator()(UserMarkContainer const * container)
  {
    ASSERT(container != nullptr, ());
    m2::AnyRectD const & rect = m_rectHolder(container->GetType());
    if (UserMark const * p = container->FindMarkInRect(rect, m_d))
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
};
}  // namespace

UserMark const * BookmarkManager::FindNearestUserMark(m2::AnyRectD const & rect) const
{
  return FindNearestUserMark([&rect](UserMark::Type) { return rect; });
}

UserMark const * BookmarkManager::FindNearestUserMark(TTouchRectHolder const & holder) const
{
  BestUserMarkFinder finder(holder);
  finder(FindUserMarksContainer(UserMark::Type::ROUTING));
  finder(FindUserMarksContainer(UserMark::Type::SEARCH));
  finder(FindUserMarksContainer(UserMark::Type::API));
  for (auto & cat : m_categories)
  {
    finder(cat.get());
  }

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

UserMarkNotificationGuard::UserMarkNotificationGuard(BookmarkManager & mng, UserMark::Type type)
  : m_controller(mng.GetUserMarksController(type))
{}

UserMarkNotificationGuard::~UserMarkNotificationGuard()
{
  m_controller.NotifyChanges();
}
