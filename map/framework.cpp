#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "benchmark_provider.hpp"
#include "benchmark_engine.hpp"
#include "geourl_process.hpp"
#include "measurement_utils.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"

#include "../anim/controller.hpp"

#include "../gui/controller.hpp"

#include "../platform/settings.hpp"
#include "../platform/preferred_languages.hpp"
#include "../platform/platform.hpp"

#include "../coding/internal/file_data.hpp"

#include "../yg/rendercontext.hpp"
#include "../yg/render_state.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/math.hpp"
#include "../base/timer.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/vector.hpp"

/// How many pixels around touch point are used to get bookmark or POI
#define TOUCH_PIXEL_RADIUS 15

using namespace storage;


#ifdef FIXED_LOCATION
Framework::FixedPosition::FixedPosition()
{
  m_fixedLatLon = Settings::Get("FixPosition", m_latlon);
  m_fixedDir = Settings::Get("FixDirection", m_dirFromNorth);
}
#endif

void Framework::AddMap(string const & file)
{
  LOG(LINFO, ("Loading map:", file));

  //threads::MutexGuard lock(m_modelSyn);
  int const version = m_model.AddMap(file);

  // Now we do force delete of old (April 2011) maps.
  //if (m_lowestMapVersion == -1 || (version != -1 && m_lowestMapVersion > version))
  //  m_lowestMapVersion = version;
  if (version == 0)
  {
    LOG(LINFO, ("Deleting old map:", file));
    RemoveMap(file);
    VERIFY ( my::DeleteFileX(GetPlatform().WritablePathForFile(file)), () );
  }
}

void Framework::RemoveMap(string const & datFile)
{
  //threads::MutexGuard lock(m_modelSyn);
  m_model.RemoveMap(datFile);
}

void Framework::SkipLocationCentering()
{
  m_informationDisplay.locationState()->SkipLocationCentering();
}

void Framework::OnLocationStatusChanged(location::TLocationStatus newStatus)
{
  m_informationDisplay.locationState()->OnLocationStatusChanged(newStatus);
}

void Framework::OnGpsUpdate(location::GpsInfo const & info)
{
#ifdef FIXED_LOCATION
  location::GpsInfo rInfo(info);
  m_fixedPos.GetLon(rInfo.m_longitude);
  m_fixedPos.GetLat(rInfo.m_latitude);
#else
  location::GpsInfo const & rInfo = info;
#endif

  m_informationDisplay.locationState()->OnGpsUpdate(rInfo);
}

void Framework::OnCompassUpdate(location::CompassInfo const & info)
{
#ifdef FIXED_LOCATION
  location::CompassInfo rInfo(info);
  m_fixedPos.GetNorth(rInfo.m_trueHeading);
#else
  location::CompassInfo const & rInfo = info;
#endif

  m_informationDisplay.locationState()->OnCompassUpdate(rInfo);
}

InformationDisplay & Framework::GetInformationDisplay()
{
  return m_informationDisplay;
}

CountryStatusDisplay * Framework::GetCountryStatusDisplay() const
{
  return m_informationDisplay.countryStatusDisplay().get();
}

static void GetResourcesMaps(vector<string> & outMaps)
{
  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.ResourcesDir(), "*" DATA_FILE_EXTENSION, outMaps);
}

Framework::Framework()
  : //m_hasPendingInvalidate(false),
    //m_doForceUpdate(false),
    m_animator(this),
    m_etalonSize(GetPlatform().ScaleEtalonSize()),
    m_queryMaxScaleMode(false),
    m_drawPlacemark(false),
    //m_hasPendingShowRectFixed(false),

    /// @todo Probably we can make this like static const int.
    /// It's not a class state, so no need to store it in memory.
    m_metresMinWidth(10),
    m_metresMaxWidth(1000000),
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_WINDOWS) || defined(OMIM_OS_LINUX)
    m_minRulerWidth(97),
#else
    m_minRulerWidth(60),
#endif

    m_width(0),
    m_height(0),
    m_informationDisplay(this),
    //m_lowestMapVersion(-1),
    m_benchmarkEngine(0)
{
  // Checking whether we should enable benchmark.
  bool isBenchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", isBenchmarkingEnabled);
  if (isBenchmarkingEnabled)
    m_benchmarkEngine = new BenchmarkEngine(this);

  // Init strings bundle.
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "^is added to the\ndownloading queue.");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading^(^%)");
  m_stringsBundle.SetDefaultString("country_status_download", "Download^");
  m_stringsBundle.SetDefaultString("country_status_download_failed", "Downloading^\nhas failed");
  m_stringsBundle.SetDefaultString("try_again", "Try Again");
  m_stringsBundle.SetDefaultString("not_enough_free_space_on_sdcard", "Not enough space\nfor downloading");

  m_animController.reset(new anim::Controller());

  // Init GUI controller.
  m_guiController.reset(new gui::Controller());
  m_guiController->SetStringsBundle(&m_stringsBundle);

  // Init information display.
  m_informationDisplay.setController(m_guiController.get());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  m_informationDisplay.enableCenter(true);
  m_informationDisplay.enableRuler(true);
  m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth, m_metresMaxWidth);

#ifndef OMIM_PRODUCTION
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_model.InitClassificator();

  // To avoid possible races - init search engine once in constructor.
  (void)GetSearchEngine();

  // Get all available maps.
  vector<string> maps;
  GetResourcesMaps(maps);
#ifndef OMIM_OS_ANDROID
  // On Android, local maps are added and removed when
  // external storage is connected/disconnected.
  GetLocalMaps(maps);
#endif

  // Remove duplicate maps if they're both present in resources and in WritableDir
  sort(maps.begin(), maps.end());
  maps.erase(unique(maps.begin(), maps.end()), maps.end());

  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));

  // Init storage with needed callback.
  m_storage.Init(bind(&Framework::UpdateAfterDownload, this, _1));

  LOG(LDEBUG, ("Storage initialized"));
}

Framework::~Framework()
{
  delete m_benchmarkEngine;
  ClearBookmarks();
}

void Framework::DeleteCountry(TIndex const & index)
{
  if (!m_storage.DeleteFromDownloader(index))
  {
    string const & file = m_storage.CountryByIndex(index).GetFile().m_fileName;
    if (m_model.DeleteMap(file + DATA_FILE_EXTENSION))
      InvalidateRect(GetCountryBounds(file), true);
  }

  m_storage.NotifyStatusChanged(index);
}

TStatus Framework::GetCountryStatus(TIndex const & index) const
{
  using namespace storage;

  TStatus res = m_storage.CountryStatus(index);

  if (res == EUnknown)
  {
    Country const & c = m_storage.CountryByIndex(index);
    LocalAndRemoteSizeT const size = c.Size();

    if (size.first == 0)
      return ENotDownloaded;

    if (size.second == 0)
      return EUnknown;

    res = EOnDisk;
    if (size.first != size.second)
    {
      /// @todo Do better version check, not just size comparison.

      // Additional check for .ready file.
      // Use EOnDisk status if it's good, or EOnDiskOutOfDate otherwise.
      Platform const & pl = GetPlatform();
      string const fName = pl.WritablePathForFile(c.GetFile().GetFileWithExt() + READY_FILE_EXTENSION);

      uint64_t sz = 0;
      if (!pl.GetFileSizeByFullPath(fName, sz) || sz != size.second)
        res = EOnDiskOutOfDate;
    }
  }

  return res;
}

m2::RectD Framework::GetCountryBounds(string const & file) const
{
  m2::RectD const r = GetSearchEngine()->GetCountryBounds(file);
  ASSERT ( r.IsValid(), () );
  return r;
}

m2::RectD Framework::GetCountryBounds(TIndex const & index) const
{
  return GetCountryBounds(m_storage.CountryByIndex(index).GetFile().m_fileName);
}

void Framework::UpdateAfterDownload(string const & file)
{
  m2::RectD rect;
  if (m_model.UpdateMap(file, rect))
    InvalidateRect(rect, true);
}

void Framework::AddLocalMaps()
{
  // add maps to the model
  Platform::FilesList maps;
  GetLocalMaps(maps);

  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));
}

void Framework::RemoveLocalMaps()
{
  m_model.RemoveAll();
}

void Framework::LoadBookmarks()
{
  ClearBookmarks();
  string const dir = GetPlatform().WritableDir();
  Platform::FilesList files;
  Platform::GetFilesInDir(dir, "*.kml", files);
  for (size_t i = 0; i < files.size(); ++i)
  {
    BookmarkCategory * cat = BookmarkCategory::CreateFromKMLFile(dir + files[i]);
    if (cat)
    {
      LOG(LINFO, ("Loaded bookmarks category", cat->GetName(), "with", cat->GetBookmarksCount(), "bookmarks"));
      m_bookmarks.push_back(cat);
    }
  }
}

void Framework::AddBookmark(string const & category, Bookmark const & bm)
{
  // @TODO not optimal for 1st release
  // Existing bookmark can be moved from one category to another,
  // or simply replaced in the same category,
  // so we scan all categories for the same bookmark
  double const squareDistance = my::sq(1.0 * MercatorBounds::degreeInMetres);
  for (size_t i = 0; i < m_bookmarks.size(); ++i)
  {
    m2::PointD const org = bm.GetOrg();
    BookmarkCategory * cat = m_bookmarks[i];
    int index = cat->GetBookmark(org, squareDistance);
    if (index >= 0)
    {
      // found bookmark to replace
      if (category == cat->GetName())
      {
        cat->ReplaceBookmark(static_cast<size_t>(index), bm);
        // Autosave added bookmark
        (void)cat->SaveToKMLFileAtPath(GetPlatform().WritableDir());
        return;
      }
      else
      {
        // Bookmark was moved from one category to another
        cat->DeleteBookmark(static_cast<size_t>(index));
      }
    }
  }

  BookmarkCategory * cat = GetBmCategory(category);
  ASSERT(cat, ("category should autocreate if not exists"));
  cat->AddBookmark(bm);
  // Autosave added bookmark
  (void)cat->SaveToKMLFileAtPath(GetPlatform().WritableDir());
}

namespace
{
  class EqualCategoryName
  {
    string const & m_name;
  public:
    EqualCategoryName(string const & name) : m_name(name) {}
    bool operator() (BookmarkCategory const * cat) const
    {
      return (cat->GetName() == m_name);
    }
  };
}

BookmarkCategory * Framework::GetBmCategory(size_t index) const
{
  return (index < m_bookmarks.size() ? m_bookmarks[index] : 0);
}

BookmarkCategory * Framework::GetBmCategory(string const & name)
{
  vector<BookmarkCategory *>::iterator i =
      find_if(m_bookmarks.begin(), m_bookmarks.end(), EqualCategoryName(name));

  if (i != m_bookmarks.end())
    return (*i);

  // Automatically create not existing category
  BookmarkCategory * cat = new BookmarkCategory(name);
  m_bookmarks.push_back(cat);
  return cat;
}

bool Framework::DeleteBmCategory(size_t index)
{
  if (index < m_bookmarks.size())
  {
    // Delete category file
    BookmarkCategory * cat = m_bookmarks[index];
    FileWriter::DeleteFileX(cat->GetFileName());
    delete cat;
    m_bookmarks.erase(m_bookmarks.begin() + index);
    return true;
  }
  else return false;
}

BookmarkAndCategory Framework::GetBookmark(m2::PointD pt) const
{
  // @TODO Refactor. Why bookmarks can't be retrieved? Change pixel point to global point.
  if (m_renderPolicy == 0)
    return MakeEmptyBookmarkAndCategory();
  return GetBookmark(pt, m_renderPolicy->VisualScale());
}

BookmarkAndCategory Framework::GetBookmark(m2::PointD pxPoint, double visualScale) const
{
  // Get the global rect of touching area.
  int const sm = TOUCH_PIXEL_RADIUS * visualScale;
  m2::RectD rect(PtoG(m2::PointD(pxPoint.x - sm, pxPoint.y - sm)),
                 PtoG(m2::PointD(pxPoint.x + sm, pxPoint.y + sm)));

  int retBookmark = -1;
  string retBookmarkCategory;
  double minD = numeric_limits<double>::max();

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
  {
    size_t const count = m_bookmarks[i]->GetBookmarksCount();
    for (size_t j = 0; j < count; ++j)
    {
      Bookmark const * bm = m_bookmarks[i]->GetBookmark(j);
      m2::PointD const pt = bm->GetOrg();

      if (rect.IsPointInside(pt))
      {
        double const d = rect.Center().SquareLength(pt);
        if (d < minD)
        {
          retBookmark = static_cast<int>(j);
          retBookmarkCategory = m_bookmarks[i]->GetName();
          minD = d;
        }
      }
    }
  }

  return make_pair(retBookmarkCategory, retBookmark);
}

void Framework::ClearBookmarks()
{
  for_each(m_bookmarks.begin(), m_bookmarks.end(), DeleteFunctor());
  m_bookmarks.clear();
}

void Framework::GetLocalMaps(vector<string> & outMaps) const
{
  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.WritableDir(), "*" DATA_FILE_EXTENSION, outMaps);
}

void Framework::PrepareToShutdown()
{
  SetRenderPolicy(0);
}

void Framework::SetMaxWorldRect()
{
  m_navigator.SetFromRect(m2::AnyRectD(m_model.GetWorldRect()));
}

bool Framework::NeedRedraw() const
{
  // Checking this here allows to avoid many dummy "IsInitialized" flags in client code.
  return (m_renderPolicy && m_renderPolicy->NeedRedraw());
}

void Framework::SetNeedRedraw(bool flag)
{
  m_renderPolicy->GetWindowHandle()->setNeedRedraw(flag);
  //if (!flag)
  //  m_doForceUpdate = false;
}

void Framework::Invalidate(bool doForceUpdate)
{
  InvalidateRect(MercatorBounds::FullRect(), doForceUpdate);
}

void Framework::InvalidateRect(m2::RectD const & rect, bool doForceUpdate)
{
  if (m_renderPolicy)
  {
    ASSERT ( rect.IsValid(), () );
    m_renderPolicy->SetForceUpdate(doForceUpdate);
    m_renderPolicy->SetInvalidRect(m2::AnyRectD(rect));
    m_renderPolicy->GetWindowHandle()->invalidate();
  }
  /*
  else
  {
    m_hasPendingInvalidate = true;
    m_doForceUpdate = doForceUpdate;
    m_invalidRect = m2::AnyRectD(rect);
  }
  */
}

void Framework::SaveState()
{
  m_navigator.SaveState();
}

bool Framework::LoadState()
{
  return m_navigator.LoadState();
}
//@}

/// Resize event from window.
void Framework::OnSize(int w, int h)
{
  if (w < 2) w = 2;
  if (h < 2) h = 2;

  if (m_renderPolicy)
  {
    m_informationDisplay.setDisplayRect(m2::RectI(m2::PointI(0, 0), m2::PointU(w, h)));

    m2::RectI const & viewPort = m_renderPolicy->OnSize(w, h);

    m_navigator.OnSize(
          viewPort.minX(),
          viewPort.minY(),
          viewPort.SizeX(),
          viewPort.SizeY());
  }

  m_width = w;
  m_height = h;
}

bool Framework::SetUpdatesEnabled(bool doEnable)
{
  if (m_renderPolicy)
    return m_renderPolicy->GetWindowHandle()->setUpdatesEnabled(doEnable);
  else
    return false;
}

int Framework::GetDrawScale() const
{
  if (m_renderPolicy)
    return m_renderPolicy->GetDrawScale(m_navigator.Screen());
  else
    return 0;
}

/*
double Framework::GetCurrentScale() const
{
  m2::PointD textureCenter(m_navigator.Screen().PixelRect().Center());
  m2::RectD glbRect;

  unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
  m_navigator.Screen().PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                                      textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                            glbRect);
  return scales::GetScaleLevelD(glbRect);
}
*/

RenderPolicy::TRenderFn Framework::DrawModelFn()
{
  return bind(&Framework::DrawModel, this, _1, _2, _3, _4, _5, _6);
}

/// Actual rendering function.
void Framework::DrawModel(shared_ptr<PaintEvent> const & e,
                          ScreenBase const & screen,
                          m2::RectD const & selectRect,
                          m2::RectD const & clipRect,
                          int scaleLevel,
                          bool isTiling)
{
  fwork::DrawProcessor doDraw(clipRect, screen, e, scaleLevel);

  try
  {
    int const scale = (m_queryMaxScaleMode ? scales::GetUpperScale() : scaleLevel);

    //threads::MutexGuard lock(m_modelSyn);
    if (isTiling)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, scale);
    else
      m_model.ForEachFeature(selectRect, doDraw, scale);
  }
  catch (redraw_operation_cancelled const &)
  {
    shared_ptr<yg::gl::RenderState> pState = e->drawer()->screen()->renderState();
    if (pState)
    {
      pState->m_isEmptyModelCurrent = false;
      pState->m_isEmptyModelActual = false;
    }
  }

  e->setIsEmptyDrawing(doDraw.IsEmptyDrawing());

  if (m_navigator.Update(ElapsedSeconds()))
    Invalidate();
}

bool Framework::IsCountryLoaded(m2::PointD const & pt) const
{
  // Correct, but slow version (check country polygon).
  string const fName = GetSearchEngine()->GetCountryFile(pt);
  if (fName.empty())
    return true;

  return m_model.IsLoaded(fName);
}

void Framework::BeginPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->BeginFrame(e, m_navigator.Screen());
}

void Framework::EndPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->EndFrame(e, m_navigator.Screen());
}

void Framework::DrawAdditionalInfo(shared_ptr<PaintEvent> const & e)
{
  // m_informationDisplay is set and drawn after the m_renderPolicy
  ASSERT ( m_renderPolicy, () );

  DrawerYG * pDrawer = e->drawer();
  yg::gl::Screen * pScreen = pDrawer->screen().get();

  pScreen->beginFrame();

  bool const isEmptyModel = m_renderPolicy->IsEmptyModel();

  if (isEmptyModel)
    m_informationDisplay.setEmptyCountryName(m_renderPolicy->GetCountryName());

  m_informationDisplay.enableCountryStatusDisplay(isEmptyModel);
  m_informationDisplay.enableCompassArrow(m_navigator.Screen().GetAngle() != 0);
  m_informationDisplay.setCompassArrowAngle(m_navigator.Screen().GetAngle());

  m_informationDisplay.setScreen(m_navigator.Screen());

  m_informationDisplay.setDebugInfo(0/*m_renderQueue.renderState().m_duration*/, GetDrawScale());

  m2::PointD const center = m_navigator.Screen().GlobalRect().GlobalCenter();
  m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x),
                                            MercatorBounds::YToLat(center.y)));

  m_informationDisplay.enableRuler(true);

  m_informationDisplay.doDraw(pDrawer);

  if (m_drawPlacemark)
    m_informationDisplay.drawPlacemark(pDrawer, "placemark-red", m_navigator.GtoP(m_placemark));

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
    if (m_bookmarks[i]->IsVisible())
    {
      size_t const count = m_bookmarks[i]->GetBookmarksCount();
      for (size_t j = 0; j < count; ++j)
      {
        Bookmark const * bm = m_bookmarks[i]->GetBookmark(j);
        m_informationDisplay.drawPlacemark(pDrawer, bm->GetType().c_str(), m_navigator.GtoP(bm->GetOrg()));
      }
    }

  pScreen->endFrame();

  m_guiController->UpdateElements();
  m_guiController->DrawFrame(pScreen);
}

/// Function for calling from platform dependent-paint function.
void Framework::DoPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
  {
    m_renderPolicy->DrawFrame(e, m_navigator.Screen());

    DrawAdditionalInfo(e);
  }
}

m2::PointD Framework::GetViewportCenter() const
{
  return m_navigator.Screen().GlobalRect().GlobalCenter();
}

void Framework::SetViewportCenter(m2::PointD const & pt)
{
  m_navigator.CenterViewport(pt);
  Invalidate();
}

static int const theMetersFactor = 6;

m2::AnyRectD Framework::ToRotated(m2::RectD const & rect) const
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(),
                      m_navigator.Screen().GetAngle(),
                      m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
}

void Framework::CheckMinGlobalRect(m2::AnyRectD & rect) const
{
  m2::RectD const minRect = MercatorBounds::RectByCenterXYAndSizeInMeters(
                                rect.GlobalCenter(), theMetersFactor * m_metresMinWidth);

  m2::AnyRectD const minAnyRect = ToRotated(minRect);

  /// @todo It would be better here to check only AnyRect ortho-sizes with minimal values.
  if (minAnyRect.IsRectInside(rect))
    rect = minAnyRect;
}

void Framework::CheckMinVisibleScale(m2::RectD & rect) const
{
  int const worldS = scales::GetUpperWorldScale();
  if (scales::GetScaleLevel(rect) > worldS)
  {
    m2::PointD const c = rect.Center();
    if (!IsCountryLoaded(c))
      rect = scales::GetRectForLevel(worldS, c, 1.0);
  }
}

void Framework::ShowRect(m2::RectD const & r)
{
  m2::AnyRectD rect(r);
  CheckMinGlobalRect(rect);

  m_navigator.SetFromRect(rect);
  Invalidate();
}

void Framework::ShowRectEx(m2::RectD const & rect)
{
  ShowRectFixed(ToRotated(rect));
}

void Framework::ShowRectExVisibleScale(m2::RectD rect)
{
  CheckMinVisibleScale(rect);
  ShowRectEx(rect);
}

void Framework::ShowRectFixed(m2::AnyRectD const & r)
{
  m2::AnyRectD rect(r);
  CheckMinGlobalRect(rect);

  /*
  if (!m_renderPolicy)
  {
    m_pendingFixedRect = rect;
    m_hasPendingShowRectFixed = true;
    return;
  }
  */

  //size_t const sz = m_renderPolicy->ScaleEtalonSize();

  /// @todo Get stored value instead of m_renderPolicy call because of invalid render policy here.
  m2::RectD etalonRect(0, 0, m_etalonSize, m_etalonSize);
  etalonRect.Offset(-m_etalonSize / 2, -m_etalonSize);

  m2::PointD const pxCenter = m_navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  m_navigator.SetFromRects(rect, etalonRect);

  Invalidate();
}

void Framework::DrawPlacemark(m2::PointD const & pt)
{
  m_drawPlacemark = true;
  m_placemark = pt;
}

void Framework::DisablePlacemark()
{
  m_drawPlacemark = false;
}

void Framework::MemoryWarning()
{
  // clearing caches on memory warning.
  m_model.ClearCaches();

  GetSearchEngine()->ClearCaches();

  LOG(LINFO, ("MemoryWarning"));
}

#define MIN_FOREGROUND_TIME_TO_SHOW_FACEBOOK_DIALOG 60 * 60
#define FOREGROUND_TIME_SETTINGS "ForegroundTime"
#define SHOW_FACEBOOK_SETTINGS "ShouldShowFacebookDialog"

void Framework::EnterBackground()
{
  // clearing caches on entering background.
  m_model.ClearCaches();

  double val = 0;
  (void)Settings::Get(FOREGROUND_TIME_SETTINGS, val);
  Settings::Set(FOREGROUND_TIME_SETTINGS, my::Timer::LocalTime() - m_StartForegroundTime + val);
}

void Framework::EnterForeground()
{
  m_StartForegroundTime = my::Timer::LocalTime();
}

/*
/// @TODO refactor to accept point and min visible length
void Framework::CenterAndScaleViewport()
{
  m2::PointD const pt = m_locationState.Position();
  m_navigator.CenterViewport(pt);

  m2::RectD const minRect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, m_metresMinWidth);
  double const xMinSize = theMetersFactor * max(m_locationState.ErrorRadius(), minRect.SizeX());
  double const yMinSize = theMetersFactor * max(m_locationState.ErrorRadius(), minRect.SizeY());

  bool needToScale = false;

  m2::RectD clipRect = GetCurrentViewport();
  if (clipRect.SizeX() < clipRect.SizeY())
    needToScale = clipRect.SizeX() > xMinSize * 3;
  else
    needToScale = clipRect.SizeY() > yMinSize * 3;

  if (needToScale)
  {
    double const k = max(xMinSize / clipRect.SizeX(),
                         yMinSize / clipRect.SizeY());

    clipRect.Scale(k);
    m_navigator.SetFromRect(m2::AnyRectD(clipRect));
  }

  Invalidate();
}
*/

/// Show all model by it's world rect.
void Framework::ShowAll()
{
  SetMaxWorldRect();
  Invalidate();
}

/// @name Drag implementation.
//@{
m2::PointD Framework::GetPixelCenter() const
{
  return m_navigator.Screen().PixelRect().Center();
}

void Framework::StartDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.StartDrag(pt, ElapsedSeconds());

  if (m_renderPolicy)
    m_renderPolicy->StartDrag();

  shared_ptr<location::State> locationState = m_informationDisplay.locationState();
  m_dragCompassProcessMode = locationState->CompassProcessMode();
}

void Framework::DoDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.DoDrag(pt, ElapsedSeconds());

  m_informationDisplay.locationState()->SetIsCentered(false);
  m_informationDisplay.locationState()->StopCompassFollowing();

  if (m_renderPolicy)
    m_renderPolicy->DoDrag();
}

void Framework::StopDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_navigator.StopDrag(pt, ElapsedSeconds(), true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  shared_ptr<location::State> locationState = m_informationDisplay.locationState();

  if (locationState->LocationProcessMode() != location::ELocationDoNothing)
  {
    // reset GPS centering mode if we have dragged far from current location
    ScreenBase const & s = m_navigator.Screen();
    if (GetPixelCenter().Length(s.GtoP(locationState->Position())) >= s.GetMinPixelRectSize() / 2.0)
      locationState->SetLocationProcessMode(location::ELocationDoNothing);
    else
    {
      if (m_dragCompassProcessMode == location::ECompassFollow)
        locationState->AnimateToPositionAndEnqueueFollowing();
      else
        locationState->AnimateToPosition();
    }
  }

  if (m_renderPolicy)
    m_renderPolicy->StopDrag();
}

void Framework::StartRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StartRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->StartRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::DoRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.DoRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->DoRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::StopRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StopRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->StopRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::Move(double azDir, double factor)
{
  m_navigator.Move(azDir, factor);

  Invalidate();
}
//@}

/// @name Scaling.
//@{
void Framework::ScaleToPoint(ScaleToPointEvent const & e)
{
  m2::PointD const pt = (m_informationDisplay.locationState()->LocationProcessMode() == location::ELocationDoNothing) ?
        m_navigator.ShiftPoint(e.Pt()) : GetPixelCenter();

  m_navigator.ScaleToPoint(pt, e.ScaleFactor(), ElapsedSeconds());

  Invalidate();
}

void Framework::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

void Framework::Scale(double scale)
{
  m_navigator.Scale(scale);

  Invalidate();
}

void Framework::CalcScalePoints(ScaleEvent const & e, m2::PointD & pt1, m2::PointD & pt2) const
{
  pt1 = m_navigator.ShiftPoint(e.Pt1());
  pt2 = m_navigator.ShiftPoint(e.Pt2());

  shared_ptr<location::State> locationState = m_informationDisplay.locationState();

  if (locationState->HasPosition()
  && (locationState->LocationProcessMode() == location::ELocationCenterOnly))
  {
    m2::PointD const ptC = (pt1 + pt2) / 2;
    m2::PointD const ptDiff = GetPixelCenter() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif
}

void Framework::StartScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.StartScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StartScale();
}

void Framework::DoScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.DoScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->DoScale();

  shared_ptr<location::State> locationState = m_informationDisplay.locationState();

  if (m_navigator.IsRotatingDuringScale()
  && (locationState->CompassProcessMode() == location::ECompassFollow))
    locationState->StopCompassFollowing();
}

void Framework::StopScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.StopScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StopScale();
}
//@}

search::Engine * Framework::GetSearchEngine() const
{
  if (!m_pSearchEngine)
  {
    Platform & pl = GetPlatform();

    try
    {
      m_pSearchEngine.reset(new search::Engine(&m_model.GetIndex(),
                               pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                               pl.GetReader(PACKED_POLYGONS_FILE),
                               pl.GetReader(COUNTRIES_FILE),
                               languages::CurrentLanguage()));
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Can't load needed resources for search::Engine: ", e.Msg()));
    }
  }

  return m_pSearchEngine.get();
}

string Framework::GetCountryName(m2::PointD const & pt) const
{
  return GetSearchEngine()->GetCountryName(pt);
}

string Framework::GetCountryName(string const & id) const
{
  return GetSearchEngine()->GetCountryName(id);
}

void Framework::PrepareSearch(bool hasPt, double lat, double lon)
{
  GetSearchEngine()->PrepareSearch(GetCurrentViewport(), hasPt, lat, lon);
}

bool Framework::Search(search::SearchParams const & params)
{
#ifdef FIXED_LOCATION
  search::SearchParams rParams(params);
  if (params.m_validPos)
  {
    m_fixedPos.GetLat(rParams.m_lat);
    m_fixedPos.GetLon(rParams.m_lon);
  }
#else
  search::SearchParams const & rParams = params;
#endif

  return GetSearchEngine()->Search(rParams, GetCurrentViewport());
}

bool Framework::GetCurrentPosition(double & lat, double & lon) const
{
  shared_ptr<location::State> locationState = m_informationDisplay.locationState();

  if (locationState->HasPosition())
  {
    m2::PointD const pos = locationState->Position();
    lat = MercatorBounds::YToLat(pos.y);
    lon = MercatorBounds::XToLon(pos.x);
    return true;
  }
  else return false;
}

void Framework::ShowSearchResult(search::Result const & res)
{
  m2::RectD const rect = res.GetFeatureRect();

  ShowRectExVisibleScale(rect);

  DrawPlacemark(rect.Center());
}

void Framework::GetDistanceAndAzimut(search::Result const & res,
                                     double lat, double lon, double north,
                                     string & distance, double & azimut)
{
  #ifdef FIXED_LOCATION
    m_fixedPos.GetLat(lat);
    m_fixedPos.GetLon(lon);
    m_fixedPos.GetNorth(north);
  #endif

  m2::PointD const center = res.GetFeatureCenter();

  double const d = ms::DistanceOnEarth(lat, lon,
                                       MercatorBounds::YToLat(center.y),
                                       MercatorBounds::XToLon(center.x));

  CHECK ( MeasurementUtils::FormatDistance(d, distance), () );

  // Do not show direction arrow for features that are too far than 25 km.
  if (north >= 0.0 && d < 25000.0)
  {
    azimut = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                     MercatorBounds::LatToY(lat)),
                          center) + north;

    double const pi2 = 2.0*math::pi;
    if (azimut < 0.0)
      azimut += pi2;
    else if (azimut > pi2)
      azimut -= pi2;
  }
}

void Framework::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  if (renderPolicy)
  {
    m_informationDisplay.setVisualScale(renderPolicy->VisualScale());

    m_navigator.SetMinScreenParams(static_cast<unsigned>(m_minRulerWidth * renderPolicy->VisualScale()),
                                   m_metresMinWidth);

    yg::gl::RenderContext::initParams();
  }

  m_guiController->ResetRenderParams();
  m_renderPolicy.reset();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy)
  {
    m_etalonSize = m_renderPolicy->ScaleEtalonSize();

    gui::Controller::RenderParams rp(m_renderPolicy->VisualScale(),
                                     bind(&WindowHandle::invalidate,
                                          renderPolicy->GetWindowHandle().get()),
                                     m_renderPolicy->GetGlyphCache(),
                                     m_renderPolicy->GetDrawer()->screen().get());

    m_guiController->SetRenderParams(rp);

    string (Framework::*pFn)(m2::PointD const &) const = &Framework::GetCountryName;
    m_renderPolicy->SetCountryNameFn(bind(pFn, this, _1));

    m_renderPolicy->SetRenderFn(DrawModelFn());

    m_renderPolicy->SetAnimController(m_animController.get());

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

    if (m_width != 0 && m_height != 0)
      OnSize(m_width, m_height);

    // Do full invalidate instead of any "pending" stuff.
    Invalidate();

    if (m_benchmarkEngine)
      m_benchmarkEngine->Start();
  }
}

RenderPolicy * Framework::GetRenderPolicy() const
{
  return m_renderPolicy.get();
}

void Framework::SetupMeasurementSystem()
{
  m_informationDisplay.setupRuler();
  Invalidate();
}

/*
// 0 - old April version which we should delete
#define MAXIMUM_VERSION_TO_DELETE 0

bool Framework::NeedToDeleteOldMaps() const
{
  return m_lowestMapVersion == MAXIMUM_VERSION_TO_DELETE;
}

void Framework::DeleteOldMaps()
{
  Platform & p = GetPlatform();
  vector<string> maps;
  p.GetFilesInDir(p.WritableDir(), "*" DATA_FILE_EXTENSION, maps);
  for (vector<string>::iterator it = maps.begin(); it != maps.end(); ++it)
  {
    feature::DataHeader header;
    LoadMapHeader(p.GetReader(*it), header);
    if (header.GetVersion() <= MAXIMUM_VERSION_TO_DELETE)
    {
      LOG(LINFO, ("Deleting old map", *it));
      RemoveMap(*it);
      FileWriter::DeleteFileX(p.WritablePathForFile(*it));
      InvalidateRect(header.GetBounds());
    }
  }
  m_lowestMapVersion = MAXIMUM_VERSION_TO_DELETE + 1;
}
*/

string Framework::GetCountryCodeByPosition(double lat, double lon) const
{
  return GetSearchEngine()->GetCountryCode(m2::PointD(
                           MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat)));
}

gui::Controller * Framework::GetGuiController() const
{
  return m_guiController.get();
}

anim::Controller * Framework::GetAnimController() const
{
  return m_animController.get();
}

bool Framework::SetViewportByURL(string const & url)
{
  using namespace url_scheme;

  Info info;
  ParseURL(url, info);

  if (info.IsValid())
  {
    ShowRectEx(info.GetViewport());
    Invalidate();
    return true;
  }

  return false;
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_navigator.Screen().ClipRect();
}

bool Framework::IsBenchmarking() const
{
  return m_benchmarkEngine != 0;
}

bool Framework::ShouldShowFacebookDialog() const
{
  double val = 0;
  bool flag = true;
  (void)Settings::Get(FOREGROUND_TIME_SETTINGS, val);
  (void)Settings::Get(SHOW_FACEBOOK_SETTINGS, flag);
  return (flag && (val >= MIN_FOREGROUND_TIME_TO_SHOW_FACEBOOK_DIALOG));
}

void Framework::SaveFacebookDialogResult(int result)
{
  switch (result)
  {
  case 0: case 2:
    Settings::Set(SHOW_FACEBOOK_SETTINGS, false);
    break;
  case 1:
    Settings::Set(FOREGROUND_TIME_SETTINGS, 0);
    break;
  default:
    LOG(LINFO, ("Unknown Facebook dialog result!"));
    break;
  }
}

shared_ptr<yg::OverlayElement> const GetClosestToPivot(list<shared_ptr<yg::OverlayElement> > const & l,
                                                       m2::PointD const & pxPoint)
{
  double dist = numeric_limits<double>::max();
  shared_ptr<yg::OverlayElement> res;

  for (list<shared_ptr<yg::OverlayElement> >::const_iterator it = l.begin();
       it != l.end();
       ++it)
  {
    double const curDist = pxPoint.SquareLength((*it)->pivot());
    if (curDist < dist)
    {
      dist = curDist;
      res = *it;
    }
  }

  return res;
}

bool Framework::GetVisiblePOI(m2::PointD const & pxPoint, m2::PointD & pxPivot, AddressInfo & info) const
{
  if (!m_renderPolicy)
  {
    LOG(LINFO, ("GetVisiblePOI called without valid renderPolicy!"));
    return false;
  }

  m2::PointD const pt = m_navigator.ShiftPoint(pxPoint);
  double const halfSize = TOUCH_PIXEL_RADIUS * m_renderPolicy->VisualScale();

  typedef yg::OverlayElement ElementT;

  list<shared_ptr<ElementT> > candidates;
  m2::RectD rect(pt.x - halfSize, pt.y - halfSize,
                 pt.x + halfSize, pt.y + halfSize);
  m_renderPolicy->GetOverlay()->selectOverlayElements(rect, candidates);

  shared_ptr<ElementT> res = GetClosestToPivot(candidates, pt);
  if (res)
  {
    ElementT::UserInfo const & ui = res->userInfo();
    if (ui.IsValid())
    {
      Index::FeaturesLoaderGuard guard(m_model.GetIndex(), ui.m_mwmID);

      FeatureType ft;
      guard.GetFeature(ui.m_offset, ft);

      // center point of element in global coordinates
      m2::PointD const center = m_navigator.Screen().PtoG(res->point(yg::EPosCenter));

      GetAddressInfo(ft, center, info);

      pxPivot = GtoP(center);
      return true;
    }
  }

  return false;
}

Animator & Framework::GetAnimator()
{
  return m_animator;
}

Navigator & Framework::GetNavigator()
{
  return m_navigator;
}
