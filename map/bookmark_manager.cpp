#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/user_mark.hpp"

#include "graphics/depth_constants.hpp"

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
  , m_lastScale(1.0)
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

namespace
{

class LazyMatrixCalc
{
  ScreenBase const & m_screen;
  double & m_lastScale;

  typedef ScreenBase::MatrixT MatrixT;
  MatrixT m_matrix;
  double m_resScale;
  bool m_scaleChanged;

  void CalcScaleG2P()
  {
    // Check whether viewport scale changed since the last drawing.
    m_scaleChanged = false;
    double const d = m_lastScale / m_screen.GetScale();
    if (d >= 2.0 || d <= 0.5)
    {
      m_scaleChanged = true;
      m_lastScale = m_screen.GetScale();
    }

    // GtoP matrix for scaling only (with change Y-axis direction).
    m_resScale = 1.0 / m_lastScale;
    m_matrix = math::Scale(math::Identity<double, 3>(), m_resScale, -m_resScale);
  }

public:
  LazyMatrixCalc(ScreenBase const & screen, double & lastScale)
    : m_screen(screen), m_lastScale(lastScale), m_resScale(0.0)
  {
  }

  bool IsScaleChanged()
  {
    if (m_resScale == 0.0)
      CalcScaleG2P();
    return m_scaleChanged;
  }

  MatrixT const & GetScaleG2P()
  {
    if (m_resScale == 0.0)
      CalcScaleG2P();
    return m_matrix;
  }

  MatrixT const & GetFinalG2P()
  {
    if (m_resScale == 0.0)
    {
      // Final GtoP matrix for drawing track's display lists.
      m_resScale = m_lastScale / m_screen.GetScale();
      m_matrix =
          math::Shift(
            math::Scale(
              math::Rotate(math::Identity<double, 3>(), m_screen.GetAngle()),
            m_resScale, m_resScale),
          m_screen.GtoP(m2::PointD(0.0, 0.0)));
    }
    return m_matrix;
  }
};

}

void BookmarkManager::PrepareToShutdown()
{
  m_routeRenderer->PrepareToShutdown();
}

void BookmarkManager::DrawCategory(BookmarkCategory const * cat, PaintOverlayEvent const & e) const
{
///@TODO UVR
//#ifndef USE_DRAPE
//  /// TODO cutomize draw in UserMarkContainer for user Draw method
//  ASSERT(cat, ());
//  if (!cat->IsVisible())
//    return;
//  Navigator const & navigator = m_framework.GetNavigator();
//  ScreenBase const & screen = navigator.Screen();

//  Drawer * pDrawer = e.GetDrawer();
//  graphics::Screen * pScreen = pDrawer->screen();

//  LazyMatrixCalc matrix(screen, m_lastScale);

//  // Draw tracks.
//  for (size_t i = 0; i < cat->GetTracksCount(); ++i)
//  {
//    Track const * track = cat->GetTrack(i);
//    if (track->HasDisplayList())
//      track->Draw(pScreen, matrix.GetFinalG2P());
//  }

//  cat->Draw(e, m_cache);
//#endif // USE_DRAPE
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
  m2::PointD ptOrg = bm->GetOrg();

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

void BookmarkManager::DrawItems(Drawer * drawer) const
{
  //@TODO UVR
  //ASSERT(m_cache != NULL, ());
  //ASSERT(m_framework.GetLocationState(), ());

  //ScreenBase const & screen = m_framework.GetNavigator().Screen();
  //m2::RectD const limitRect = screen.ClipRect();

  //LazyMatrixCalc matrix(screen, m_lastScale);

  //double const drawScale = m_framework.GetDrawScale();
  //double const visualScale = m_framework.GetVisualScale();
  //location::RouteMatchingInfo const & matchingInfo = m_framework.GetLocationState()->GetRouteMatchingInfo();

  //auto trackUpdateFn = [&](Track const * track)
  //{
  //  ASSERT(track, ());
  //  if (limitRect.IsIntersect(track->GetLimitRect()))
  //    track->CreateDisplayList(m_bmScreen, matrix.GetScaleG2P(), matrix.IsScaleChanged(), drawScale, visualScale, matchingInfo);
  //  else
  //    track->CleanUp();
  //};

  //auto dlUpdateFn = [&trackUpdateFn] (BookmarkCategory const * cat)
  //{
  //  bool const isVisible = cat->IsVisible();
  //  for (size_t j = 0; j < cat->GetTracksCount(); ++j)
  //  {
  //    Track const * track = cat->GetTrack(j);
  //    ASSERT(track, ());
  //    if (isVisible)
  //      trackUpdateFn(track);
  //    else
  //      track->CleanUp();
  //  }
  //};

  // Update track's display lists.
  //for (size_t i = 0; i < m_categories.size(); ++i)
  //{
  //  BookmarkCategory const * cat = m_categories[i];
  //  ASSERT(cat, ());
  //  dlUpdateFn(cat);
  //}

  //graphics::Screen * pScreen = GPUDrawer::GetScreen(drawer);
  //pScreen->beginFrame();

  //PaintOverlayEvent event(drawer, screen);
  //for_each(m_userMarkLayers.begin(), m_userMarkLayers.end(), bind(&UserMarkContainer::Draw, _1, event, m_cache));
  //for_each(m_categories.begin(), m_categories.end(), bind(&BookmarkManager::DrawCategory, this, _1, event));
  //m_routeRenderer->Render(pScreen, screen);
  //m_selection.Draw(event, m_cache);

  //pScreen->endFrame();
#endif // USE_DRAPE
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

///@TODO UVR
//void BookmarkManager::ActivateMark(UserMark const * mark, bool needAnim)
//{
//  m_selection.ActivateMark(mark, needAnim);
//}

//bool BookmarkManager::UserMarkHasActive() const
//{
//  return m_selection.IsActive();
//}

//bool BookmarkManager::IsUserMarkActive(UserMark const * mark) const
//{
//  if (mark == nullptr)
//    return false;

//  return (m_selection.m_container == mark->GetContainer() &&
//          m_selection.m_ptOrg.EqualDxDy(mark->GetOrg(), 1.0E-4));
//}

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
  return FindNearestUserMark([&rect](UserMarkContainer::Type) -> m2::AnyRectD const & { return rect; });
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

void BookmarkManager::SetRouteTrack(RouteTrack & track)
{
  m_routeTrack.reset();
  m_routeTrack.reset(track.CreatePersistent());
}

void BookmarkManager::ResetRouteTrack()
{
  m_routeTrack.reset();
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
