#include "bookmark_manager.hpp"
#include "framework.hpp"

#include "../platform/platform.hpp"
#include "../platform/settings.hpp"

#include "../indexer/scales.hpp"

#include "../geometry/transformations.hpp"

#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/vector.hpp"


BookmarkManager::BookmarkManager(Framework & f)
  : m_framework(f), m_bmScreen(0), m_lastScale(1.0)
{
  m_additionalPoiLayer = new BookmarkCategory("");
}

BookmarkManager::~BookmarkManager()
{
  delete m_additionalPoiLayer;
  ClearItems();
  DeleteScreen();
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

void BookmarkManager::DrawCategory(BookmarkCategory const * cat, shared_ptr<PaintEvent> const & e) const
{
  Navigator const & navigator = m_framework.GetNavigator();
  InformationDisplay & informationDisplay = m_framework.GetInformationDisplay();
  ScreenBase const & screen = navigator.Screen();

  Drawer * pDrawer = e->drawer();
  graphics::Screen * pScreen = pDrawer->screen();

  LazyMatrixCalc matrix(screen, m_lastScale);

  // Draw tracks.
  for (size_t i = 0; i < cat->GetTracksCount(); ++i)
  {
    Track const * track = cat->GetTrack(i);
    if (track->HasDisplayList())
      track->Draw(pScreen, matrix.GetFinalG2P());
  }

  // Draw bookmarks.
  m2::AnyRectD const & glbRect = screen.GlobalRect();
  for (size_t j = 0; j < cat->GetBookmarksCount(); ++j)
  {
    Bookmark const * bm = cat->GetBookmark(j);
    m2::PointD const & org = bm->GetOrg();

    if (glbRect.IsPointInside(org))
      informationDisplay.drawPlacemark(pDrawer, bm->GetType().c_str(), navigator.GtoP(org));
  }
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
  BookmarkCategory * cat = BookmarkCategory::CreateFromKMLFile(filePath);
  if (cat)
    m_categories.push_back(cat);
}

size_t BookmarkManager::AddBookmark(size_t categoryIndex, Bookmark & bm)
{
  bm.SetTimeStamp(time(0));
  bm.SetScale(m_framework.GetDrawScale());

  BookmarkCategory * pCat = m_categories[categoryIndex];
  pCat->AddBookmark(bm);
  pCat->SetVisible(true);
  pCat->SaveToKMLFile();

  m_lastCategoryUrl = pCat->GetFileName();
  m_lastType = bm.GetType();
  SaveState();

  return (pCat->GetBookmarksCount() - 1);
}

void BookmarkManager::ReplaceBookmark(size_t catIndex, size_t bmIndex, Bookmark const & bm)
{
  BookmarkCategory * pCat = m_categories[catIndex];
  pCat->ReplaceBookmark(bmIndex, bm);
  pCat->SaveToKMLFile();

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
    m_categories.push_back(new BookmarkCategory(m_framework.GetStringsBundle().GetString("my_places")));

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
  m_categories.push_back(new BookmarkCategory(name));
  return (m_categories.size()-1);
}

void BookmarkManager::DrawItems(shared_ptr<PaintEvent> const & e) const
{
  ScreenBase const & screen = m_framework.GetNavigator().Screen();
  m2::RectD const limitRect = screen.ClipRect();

  LazyMatrixCalc matrix(screen, m_lastScale);

  // Update track's display lists.
  for (size_t i = 0; i < m_categories.size(); ++i)
  {
    BookmarkCategory const * cat = m_categories[i];
    if (cat->IsVisible())
    {
      for (size_t j = 0; j < cat->GetTracksCount(); ++j)
      {
        Track const * track = cat->GetTrack(j);
        if (limitRect.IsIntersect(track->GetLimitRect()))
        {
          if (!track->HasDisplayList() || matrix.IsScaleChanged())
            track->CreateDisplayList(m_bmScreen, matrix.GetScaleG2P());
        }
        else
          track->DeleteDisplayList();
      }
    }
  }

  graphics::Screen * pScreen = e->drawer()->screen();
  pScreen->beginFrame();

  for (size_t i = 0; i < m_categories.size(); ++i)
  {
    if (m_categories[i]->IsVisible())
      DrawCategory(m_categories[i], e);
  }

  if (m_additionalPoiLayer->IsVisible())
    DrawCategory(m_additionalPoiLayer, e);

  pScreen->endFrame();
}

void BookmarkManager::DeleteBmCategory(CategoryIter i)
{
  BookmarkCategory * cat = *i;

  FileWriter::DeleteFileX(cat->GetFileName());

  delete cat;

  m_categories.erase(i);
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

void BookmarkManager::AdditionalPoiLayerSetInvisible()
{
  m_additionalPoiLayer->SetVisible(false);
}

void BookmarkManager::AdditionalPoiLayerSetVisible()
{
  m_additionalPoiLayer->SetVisible(true);
}

void BookmarkManager::AdditionalPoiLayerAddPoi(Bookmark const & bm)
{
  m_additionalPoiLayer->AddBookmark(bm);
}

Bookmark const * BookmarkManager::AdditionalPoiLayerGetBookmark(size_t index) const
{
  return m_additionalPoiLayer->GetBookmark(index);
}

Bookmark * BookmarkManager::AdditionalPoiLayerGetBookmark(size_t index)
{
  return m_additionalPoiLayer->GetBookmark(index);
}

void BookmarkManager::AdditionalPoiLayerClear()
{
  m_additionalPoiLayer->ClearBookmarks();
}

bool BookmarkManager::IsAdditionalLayerPoi(const BookmarkAndCategory & bm) const
{
  return (bm.first == additionalLayerCategory && bm.second >= 0);
}

void BookmarkManager::SetScreen(graphics::Screen * screen)
{
  DeleteScreen();
  m_bmScreen = screen;
}

void BookmarkManager::DeleteScreen()
{
  if (m_bmScreen)
  {
    // Delete display lists for all tracks
    for (size_t i = 0; i < m_categories.size(); ++i)
      for (size_t j = 0; j < m_categories[i]->GetTracksCount(); ++j)
        m_categories[i]->GetTrack(j)->DeleteDisplayList();

    delete m_bmScreen;
    m_bmScreen = 0;
  }
}
