#include "framework.hpp"
#include "feature_processor.hpp"
#include "drawer.hpp"
#include "benchmark_provider.hpp"
#include "benchmark_engine.hpp"
#include "geourl_process.hpp"
#include "measurement_utils.hpp"
#include "dialog_settings.hpp"
#include "ge0_parser.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/feature_algo.hpp"

#include "../anim/controller.hpp"

#include "../gui/controller.hpp"

#include "../platform/settings.hpp"
#include "../platform/preferred_languages.hpp"
#include "../platform/platform.hpp"

#include "../coding/internal/file_data.hpp"
#include "../coding/zip_reader.hpp"
#include "../coding/url_encode.hpp"
#include "../coding/file_name_utils.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/math.hpp"
#include "../base/timer.hpp"
#include "../base/scope_guard.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/vector.hpp"

#include "../../api/internal/c/api-client-internals.h"
#include "../../api/src/c/api-client.h"


/// How many pixels around touch point are used to get bookmark or POI
#define TOUCH_PIXEL_RADIUS 20

#define KMZ_EXTENSION ".kmz"

#define DEFAULT_BOOKMARK_TYPE "placemark-red"

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
  if (version == feature::DataHeader::v1)
  {
    LOG(LINFO, ("Deleting old map:", file));
    RemoveMap(file);
    VERIFY ( my::DeleteFileX(GetPlatform().WritablePathForFile(file)), () );
  }
  else
  {
    if (m_lowestMapVersion > version)
      m_lowestMapVersion = version;
  }
}

void Framework::RemoveMap(string const & file)
{
  //threads::MutexGuard lock(m_modelSyn);
  m_model.RemoveMap(file);
}

void Framework::StartLocation()
{
  m_informationDisplay.locationState()->OnStartLocation();
}

void Framework::StopLocation()
{
  m_informationDisplay.locationState()->OnStopLocation();
}

void Framework::OnLocationError(location::TLocationError error)
{}

void Framework::OnLocationUpdate(location::GpsInfo const & info)
{
#ifdef FIXED_LOCATION
  location::GpsInfo rInfo(info);

  // get fixed coordinates
  m_fixedPos.GetLon(rInfo.m_longitude);
  m_fixedPos.GetLat(rInfo.m_latitude);

  // pretend like GPS position
  rInfo.m_horizontalAccuracy = 5.0;

  if (m_fixedPos.HasNorth())
  {
    // pass compass value (for devices without compass)
    location::CompassInfo compass;
    compass.m_magneticHeading = compass.m_trueHeading = 0.0;
    compass.m_timestamp = rInfo.m_timestamp;
    OnCompassUpdate(compass);
  }

#else
  location::GpsInfo const & rInfo = info;
#endif

  m_informationDisplay.locationState()->OnLocationUpdate(rInfo);
  m_balloonManager.LocationChanged(rInfo);
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

void Framework::StopLocationFollow()
{
  shared_ptr<location::State> ls = m_informationDisplay.locationState();

  ls->StopCompassFollowing();
  ls->SetLocationProcessMode(location::ELocationDoNothing);
  ls->SetIsCentered(false);
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
  pl.GetFilesByExt(pl.ResourcesDir(), DATA_FILE_EXTENSION, outMaps);
}

Framework::Framework()
  : m_animator(this),
    m_queryMaxScaleMode(false),
    m_drawPlacemark(false),

    /// @todo It's not a class state, so no need to store it in memory.
    /// Move this constants to Ruler (and don't store them at all).
    m_metresMinWidth(10),
    m_metresMaxWidth(1000000),

#if defined(OMIM_OS_DESKTOP)
    m_minRulerWidth(97),
#else
    m_minRulerWidth(60),
#endif

    m_width(0),
    m_height(0),
    m_informationDisplay(this),
    m_lowestMapVersion(numeric_limits<int>::max()),
    m_benchmarkEngine(0),
    m_bmManager(*this),
    m_balloonManager(*this)
{
  // Checking whether we should enable benchmark.
  bool isBenchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", isBenchmarkingEnabled);
  if (isBenchmarkingEnabled)
    m_benchmarkEngine = new BenchmarkEngine(this);

  // Init strings bundle.
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "^is added to the downloading queue");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading^^%");
  m_stringsBundle.SetDefaultString("country_status_download", "Download^");
  m_stringsBundle.SetDefaultString("country_status_download_failed", "Downloading^has failed");
  m_stringsBundle.SetDefaultString("try_again", "Try Again");
  m_stringsBundle.SetDefaultString("not_enough_free_space_on_sdcard", "Not enough space for downloading");

  m_stringsBundle.SetDefaultString("dropped_pin", "Dropped Pin");
  m_stringsBundle.SetDefaultString("my_places", "My Places");
  m_stringsBundle.SetDefaultString("my_position", "My Position");

  m_animController.reset(new anim::Controller());

  // Init GUI controller.
  m_guiController.reset(new gui::Controller());
  m_guiController->SetStringsBundle(&m_stringsBundle);

  // Init information display.
  m_informationDisplay.setController(m_guiController.get());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  m_informationDisplay.enableRuler(true);
  m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth, m_metresMaxWidth);

#ifndef OMIM_PRODUCTION
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_model.InitClassificator();

  // Get all available maps.
  vector<string> maps;
  GetResourcesMaps(maps);
#ifndef OMIM_OS_ANDROID
  // On Android, local maps are added and removed when external storage is connected/disconnected.
  GetLocalMaps(maps);
#endif

  // Remove duplicate maps if they're both present in resources and in WritableDir.
  sort(maps.begin(), maps.end());
  maps.erase(unique(maps.begin(), maps.end()), maps.end());

  // Add founded maps to index.
  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));

  // Init storage with needed callback.
  m_storage.Init(bind(&Framework::UpdateAfterDownload, this, _1));

  // To avoid possible races - init search engine once in constructor.
  (void)GetSearchEngine();

  LOG(LDEBUG, ("Storage initialized"));
}

Framework::~Framework()
{
  delete m_benchmarkEngine;
}

double Framework::GetVisualScale() const
{
  return (m_renderPolicy ? m_renderPolicy->VisualScale() : 1);
}

int Framework::GetScaleEtalonSize() const
{
  return (m_renderPolicy ? m_renderPolicy->ScaleEtalonSize() : 512);
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

string Framework::GetCountryName(storage::TIndex const & index) const
{
  string group, name;
  m_storage.GetGroupAndCountry(index, group, name);
  return (!group.empty() ? group + ", " + name : name);
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

void Framework::ShowCountry(storage::TIndex const & index)
{
  StopLocationFollow();

  ShowRectEx(GetCountryBounds(index));
}

void Framework::UpdateAfterDownload(string const & file)
{
  m2::RectD rect;
  if (m_model.UpdateMap(file, rect))
    InvalidateRect(rect, true);

  // Clear search cache of features in all viewports
  // (there are new features from downloaded file).
  GetSearchEngine()->ClearViewportsCache();
}

void Framework::AddLocalMaps()
{
  // add maps to the model
  Platform::FilesList maps;
  GetLocalMaps(maps);

  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));

  m_pSearchEngine->SupportOldFormat(m_lowestMapVersion < feature::DataHeader::v3);
}

void Framework::RemoveLocalMaps()
{
  m_model.RemoveAll();
}

void Framework::LoadBookmarks()
{
  if (!GetPlatform().IsPro())
    return;
  m_bmManager.LoadBookmarks();
}

size_t Framework::AddBookmark(size_t categoryIndex, Bookmark & bm)
{
  return m_bmManager.AddBookmark(categoryIndex, bm);
}

size_t Framework::AddCategory(string const & categoryName)
{
  return m_bmManager.CreateBmCategory(categoryName);
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
  return m_bmManager.GetBmCategory(index);
}

size_t Framework::LastEditedCategory()
{
  return m_bmManager.LastEditedBMCategory();
}

bool Framework::DeleteBmCategory(size_t index)
{
  return m_bmManager.DeleteBmCategory(index);
}

BookmarkAndCategory Framework::GetBookmark(m2::PointD const & pxPoint) const
{
  return GetBookmark(pxPoint, GetVisualScale());
}

BookmarkAndCategory Framework::GetBookmark(m2::PointD const & pxPoint, double visualScale) const
{
  // Get the global rect of touching area.
  int const sm = TOUCH_PIXEL_RADIUS * visualScale;
  m2::RectD rect(PtoG(m2::PointD(pxPoint.x - sm, pxPoint.y - sm)),
                 PtoG(m2::PointD(pxPoint.x + sm, pxPoint.y + sm)));

  int retBookmarkCategory = -1;
  int retBookmark = -1;
  double minD = numeric_limits<double>::max();
  bool returnBookmarkIsVisible = false;

  if (m_bmManager.AdditionalLayerIsVisible())
  {
    for (int i = 0; i < m_bmManager.AdditionalLayerNumberOfPoi(); ++i)
    {
      m2::PointD const pt = m_bmManager.AdditionalPoiLayerGetBookmark(i)->GetOrg();
      if (rect.IsPointInside(pt))
      {
        double const d = rect.Center().SquareLength(pt);
        if (d < minD)
        {
          retBookmarkCategory = static_cast<int>(additionalLayerCategory);
          retBookmark = static_cast<int>(i);
          minD = d;
        }
      }
    }
  }

  for (size_t i = 0; i < m_bmManager.GetBmCategoriesCount(); ++i)
  {
    bool const currentCategoryIsVisible = m_bmManager.GetBmCategory(i)->IsVisible();
    if (!currentCategoryIsVisible && returnBookmarkIsVisible)
      continue;

    size_t const count = m_bmManager.GetBmCategory(i)->GetBookmarksCount();
    for (size_t j = 0; j < count; ++j)
    {
      Bookmark const * bm = m_bmManager.GetBmCategory(i)->GetBookmark(j);
      m2::PointD const pt = bm->GetOrg();

      if (rect.IsPointInside(pt))
      {
        double const d = rect.Center().SquareLength(pt);
        if ((currentCategoryIsVisible && !returnBookmarkIsVisible) ||
            (d < minD))
        {
          retBookmarkCategory = static_cast<int>(i);
          retBookmark = static_cast<int>(j);
          minD = d;
          returnBookmarkIsVisible = m_bmManager.GetBmCategory(i)->IsVisible();
        }
      }
    }
  }

  return make_pair(retBookmarkCategory, retBookmark);
}

void Framework::ShowBookmark(Bookmark const & bm)
{
  StopLocationFollow();

  double scale = bm.GetScale();
  if (scale == -1.0) scale = 16.0;

  ShowRectExVisibleScale(scales::GetRectForLevel(scale, bm.GetOrg(), 1.0));
}

void Framework::ClearBookmarks()
{
  m_bmManager.ClearBookmarks();
}

namespace
{

/// @return extension with a dot in lower case
string const GetFileExt(string const & filePath)
{
  string ext = my::GetFileExtension(filePath);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

string const GetFileName(string const & filePath)
{
  string ret = filePath;
  my::GetNameFromFullPath(ret);
  return ret;
}

string const GenerateValidAndUniqueFilePathForKLM(string const & fileName)
{
  string filePath = BookmarkCategory::RemoveInvalidSymbols(fileName);
  filePath = BookmarkCategory::GenerateUniqueFileName(GetPlatform().WritableDir(), filePath);
  return filePath;
}

}

bool Framework::AddBookmarksFile(string const & filePath)
{
  string const fileExt = GetFileExt(filePath);
  string fileSavePath;
  if (fileExt == BOOKMARKS_FILE_EXTENSION)
  {
    fileSavePath = GenerateValidAndUniqueFilePathForKLM(GetFileName(filePath));
    if (!my::CopyFileX(filePath, fileSavePath))
      return false;
  }
  else if (fileExt == KMZ_EXTENSION)
  {
    try
    {
      ZipFileReader::FileListT files;
      ZipFileReader::FilesList(filePath, files);
      string kmlFileName;
      for (size_t i = 0; i < files.size(); ++i)
      {
        if (GetFileExt(files[i].first) == BOOKMARKS_FILE_EXTENSION)
        {
          kmlFileName = files[i].first;
          break;
        }
      }
      if (kmlFileName.empty())
        return false;

      fileSavePath = GenerateValidAndUniqueFilePathForKLM(kmlFileName);
      ZipFileReader::UnzipFile(filePath, kmlFileName, fileSavePath);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Error unzipping file", filePath, e.Msg()));
      return false;
    }
  }
  else
  {
    LOG(LWARNING, ("Unknown file type", filePath));
    return false;
  }

  // Update freshly added bookmarks
  m_bmManager.LoadBookmark(fileSavePath);

  return true;
}

void Framework::AdditionalPoiLayerSetInvisible()
{
  m_bmManager.AdditionalPoiLayerSetInvisible();
}

void Framework::AdditionalPoiLayerSetVisible()
{
  m_bmManager.AdditionalPoiLayerSetVisible();
}

void Framework::AdditionalPoiLayerAddPoi(Bookmark const & bm)
{
  m_bmManager.AdditionalPoiLayerAddPoi(bm);
}

Bookmark const * Framework::AdditionalPoiLayerGetBookmark(size_t index) const
{
  return m_bmManager.AdditionalPoiLayerGetBookmark(index);
}

Bookmark * Framework::AdditionalPoiLayerGetBookmark(size_t index)
{
  return m_bmManager.AdditionalPoiLayerGetBookmark(index);
}

void Framework::AdditionalPoiLayerDeleteBookmark(int index)
{
  m_bmManager.AdditionalPoiLayerDeleteBookmark(index);
}

void Framework::AdditionalPoiLayerClear()
{
  m_bmManager.AdditionalPoiLayerClear();
}

bool Framework::IsAdditionalLayerPoi(const BookmarkAndCategory & bm) const
{
  return m_bmManager.IsAdditionalLayerPoi(bm);
}

bool Framework::AdditionalLayerIsVisible()
{
  return m_bmManager.AdditionalLayerIsVisible();
}

size_t Framework::AdditionalLayerNumberOfPoi()
{
  return m_bmManager.AdditionalLayerNumberOfPoi();
}

void Framework::GetLocalMaps(vector<string> & outMaps) const
{
  Platform & pl = GetPlatform();
  pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, outMaps);
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

    m_navigator.OnSize(viewPort.minX(), viewPort.minY(), viewPort.SizeX(), viewPort.SizeY());

    m_balloonManager.ScreenSizeChanged(w, h);
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
  fwork::FeatureProcessor doDraw(clipRect, screen, e, scaleLevel);

  try
  {
    // limit scaleLevel to be not more than upperScale
    int const upperScale = scales::GetUpperScale();
    int const scale = min((m_queryMaxScaleMode ? upperScale : scaleLevel), upperScale);

    if (isTiling)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, scale);
    else
      m_model.ForEachFeature(selectRect, doDraw, scale);
  }
  catch (redraw_operation_cancelled const &)
  {}

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

  Drawer * pDrawer = e->drawer();
  graphics::Screen * pScreen = pDrawer->screen();

  pScreen->beginFrame();

  bool const isEmptyModel = m_renderPolicy->IsEmptyModel();

  if (isEmptyModel)
    m_informationDisplay.setEmptyCountryIndex(m_renderPolicy->GetCountryIndex());

  m_informationDisplay.enableCountryStatusDisplay(isEmptyModel);
  m_informationDisplay.enableCompassArrow(m_navigator.Screen().GetAngle() != 0);
  m_informationDisplay.setCompassArrowAngle(m_navigator.Screen().GetAngle());

  m_informationDisplay.setScreen(m_navigator.Screen());

  m_informationDisplay.setDebugInfo(0, GetDrawScale());

  m_informationDisplay.enableRuler(true);

  m_informationDisplay.doDraw(pDrawer);

  if (m_drawPlacemark)
    m_informationDisplay.drawPlacemark(pDrawer, DEFAULT_BOOKMARK_TYPE, m_navigator.GtoP(m_placemark));

  m_bmManager.DrawBookmarks(e);
  DrawMapApiPoints(e);

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

bool Framework::CheckMinVisibleScale(m2::RectD & rect) const
{
  int const worldS = scales::GetUpperWorldScale();
  if (scales::GetScaleLevel(rect) > worldS)
  {
    m2::PointD const c = rect.Center();
    if (!IsCountryLoaded(c))
    {
      rect = scales::GetRectForLevel(worldS, c, 1.0);
      return true;
    }
  }
  return false;
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

  double const halfSize = GetScaleEtalonSize() / 2.0;
  m2::RectD etalonRect(-halfSize, -halfSize, halfSize, halfSize);

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

void Framework::ClearAllCaches()
{
  m_model.ClearCaches();
  GetSearchEngine()->ClearAllCaches();
}

void Framework::MemoryWarning()
{
  ClearAllCaches();

  LOG(LINFO, ("MemoryWarning"));
}

void Framework::EnterBackground()
{
  // Do not clear caches for Android. This function is called when main activity is paused,
  // but at the same time search activity (for example) is enabled.
#ifndef OMIM_OS_ANDROID
  ClearAllCaches();
#endif

  dlg_settings::EnterBackground(my::Timer::LocalTime() - m_StartForegroundTime);
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
  m_dragCompassProcessMode = locationState->GetCompassProcessMode();
  m_dragLocationProcessMode = locationState->GetLocationProcessMode();
}

void Framework::DoDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.DoDrag(pt, ElapsedSeconds());

  shared_ptr<location::State> locationState = m_informationDisplay.locationState();

  locationState->SetIsCentered(false);
  locationState->StopCompassFollowing();
  locationState->SetLocationProcessMode(location::ELocationDoNothing);

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

  if (m_dragLocationProcessMode != location::ELocationDoNothing)
  {
    // reset GPS centering mode if we have dragged far from current location
    ScreenBase const & s = m_navigator.Screen();
    if (GetPixelCenter().Length(s.GtoP(locationState->Position())) >= s.GetMinPixelRectSize() / 5.0)
      locationState->SetLocationProcessMode(location::ELocationDoNothing);
    else
    {
      locationState->SetLocationProcessMode(m_dragLocationProcessMode);
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
  shared_ptr<location::State> locationState = m_informationDisplay.locationState();
  m2::PointD const pt = (locationState->GetLocationProcessMode() == location::ELocationDoNothing) ?
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
  && (locationState->GetLocationProcessMode() == location::ELocationCenterOnly))
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
  && (locationState->GetCompassProcessMode() == location::ECompassFollow))
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
      m_pSearchEngine.reset(new search::Engine(
                              &m_model.GetIndex(),
                              pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                              pl.GetReader(PACKED_POLYGONS_FILE),
                              pl.GetReader(COUNTRIES_FILE),
                              languages::CurrentLanguage()));

      m_pSearchEngine->SupportOldFormat(m_lowestMapVersion < feature::DataHeader::v3);
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Can't load needed resources for search::Engine: ", e.Msg()));
    }
  }

  return m_pSearchEngine.get();
}

storage::TIndex Framework::GetCountryIndex(m2::PointD const & pt) const
{
  return m_storage.FindIndexByFile(GetSearchEngine()->GetCountryFile(pt));
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
  if (params.IsValidPosition())
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
  StopLocationFollow();

  m2::RectD const rect = res.GetFeatureRect();

  ShowRectExVisibleScale(rect);

  search::AddressInfo info;
  info.MakeFrom(res);
  m_balloonManager.ShowAddress(res.GetFeatureCenter(), info);
}

bool Framework::GetDistanceAndAzimut(m2::PointD const & point,
                                     double lat, double lon, double north,
                                     string & distance, double & azimut)
{
#ifdef FIXED_LOCATION
  m_fixedPos.GetLat(lat);
  m_fixedPos.GetLon(lon);
  m_fixedPos.GetNorth(north);
#endif

  double const d = ms::DistanceOnEarth(lat, lon,
                                       MercatorBounds::YToLat(point.y),
                                       MercatorBounds::XToLon(point.x));

  // Distance may be less than 1.0
  (void) MeasurementUtils::FormatDistance(d, distance);

  if (north >= 0.0)
  {
    // We calculate azimut even when distance is very short (d ~ 0),
    // because return value has 2 states (near me or far from me).

    azimut = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                     MercatorBounds::LatToY(lat)),
                          point) + north;

    double const pi2 = 2.0*math::pi;
    if (azimut < 0.0)
      azimut += pi2;
    else if (azimut > pi2)
      azimut -= pi2;
  }

  // This constant and return value is using for arrow/flag choice.
  return (d < 25000.0);
}

void Framework::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  m_guiController->ResetRenderParams();
  m_renderPolicy.reset();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy)
  {
    gui::Controller::RenderParams rp(m_renderPolicy->Density(),
                                     bind(&WindowHandle::invalidate,
                                          renderPolicy->GetWindowHandle().get()),
                                     m_renderPolicy->GetGlyphCache(),
                                     m_renderPolicy->GetCacheScreen().get());

    m_guiController->SetRenderParams(rp);

    m_renderPolicy->SetCountryIndexFn(bind(&Framework::GetCountryIndex, this, _1));

    m_renderPolicy->SetAnimController(m_animController.get());

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());
    m_navigator.SetMinScreenParams(static_cast<unsigned>(m_minRulerWidth * m_renderPolicy->VisualScale()),
                                   m_metresMinWidth);

    m_informationDisplay.setVisualScale(m_renderPolicy->VisualScale());

    m_balloonManager.RenderPolicyCreated(m_renderPolicy->Density());

    if (m_width != 0 && m_height != 0)
      OnSize(m_width, m_height);

    // Do full invalidate instead of any "pending" stuff.
    Invalidate();

    m_renderPolicy->SetRenderFn(DrawModelFn());

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
  m_informationDisplay.ruler()->setIsDirtyLayout(true);
  Invalidate();
}

string Framework::GetCountryCode(m2::PointD const & pt) const
{
  return GetSearchEngine()->GetCountryCode(pt);
}

gui::Controller * Framework::GetGuiController() const
{
  return m_guiController.get();
}

anim::Controller * Framework::GetAnimController() const
{
  return m_animController.get();
}

void Framework::AddBookmarkAndSetViewport(Bookmark & bm, m2::RectD const & viewPort)
{
  size_t const catIndex = LastEditedCategory();
  m_bmManager.AddBookmark(catIndex, bm);
  ShowRectExVisibleScale(viewPort);
}

bool Framework::SetViewportByURL(string const & url, url_scheme::ApiPoint & balloonPoint)
{
  if (strings::StartsWith(url, "geo"))
  {
    using namespace url_scheme;

    Info info;
    ParseGeoURL(url, info);

    if (info.IsValid())
    {
      StopLocationFollow();
      balloonPoint.m_name = m_stringsBundle.GetString("dropped_pin");
      balloonPoint.m_lat = info.m_lat;
      balloonPoint.m_lon = info.m_lon;
      SetViewPortSync(info.GetViewport());
      return true;
    }
  }
  else if (strings::StartsWith(url, "ge0"))
  {
    StopLocationFollow();
    url_scheme::Ge0Parser parser;
    double zoomLevel;
    if (parser.Parse(url, balloonPoint, zoomLevel))
    {
      if (balloonPoint.m_name.empty())
        balloonPoint.m_name = m_stringsBundle.GetString("dropped_pin");

      m2::PointD const center(MercatorBounds::LonToX(balloonPoint.m_lon), MercatorBounds::LatToY(balloonPoint.m_lat));
      SetViewPortSync(scales::GetRectForLevel(zoomLevel, center, 1));
      return true;
    }
  }
  else if (strings::StartsWith(url, "mapswithme://") || strings::StartsWith(url, "mwm://"))
  {
    if (m_ParsedMapApi.SetUriAndParse(url))
    {
      StopLocationFollow();
      // Can do better consider nav bar size
      SetViewPortSync(MercatorBounds::FromLatLonRect(m_ParsedMapApi.GetLatLonRect()));

      if (!m_ParsedMapApi.GetPoints().empty())
      {
        balloonPoint = m_ParsedMapApi.GetPoints().front();
        return true;
      }
    }
  }
  return false;
}

void Framework::SetViewPortSync(m2::RectD rect)
{
  // This is tricky way to syncronize work and rendring threads.
  // Quick buben-fix to show correct rect in API when country is not downloaded
  if (CheckMinVisibleScale(rect))
    rect.Inflate(0.25, 0.25);
  m2::AnyRectD aRect(rect);
  CheckMinGlobalRect(aRect);
  m_animator.ChangeViewport(aRect, aRect, 0.0);
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_navigator.Screen().ClipRect();
}

bool Framework::IsBenchmarking() const
{
  return m_benchmarkEngine != 0;
}

namespace
{

typedef shared_ptr<graphics::OverlayElement> OEPointerT;

OEPointerT GetClosestToPivot(list<OEPointerT> const & l, m2::PointD const & pxPoint)
{
  double dist = numeric_limits<double>::max();
  OEPointerT res;

  for (list<OEPointerT>::const_iterator it = l.begin(); it != l.end(); ++it)
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

}

bool Framework::GetVisiblePOI(m2::PointD const & pxPoint, m2::PointD & pxPivot,
                              search::AddressInfo & info) const
{
  graphics::OverlayElement::UserInfo ui;

  {
    // It seems like we don't need to lock frame here.
    // Overlay locking and storing items as shared_ptr is enough here.
    //m_renderPolicy->FrameLock();

    m2::PointD const pt = m_navigator.ShiftPoint(pxPoint);
    double const halfSize = TOUCH_PIXEL_RADIUS * GetVisualScale();

    list<OEPointerT> candidates;
    m2::RectD const rect(pt.x - halfSize, pt.y - halfSize,
                         pt.x + halfSize, pt.y + halfSize);

    graphics::Overlay * frameOverlay = m_renderPolicy->FrameOverlay();
    frameOverlay->lock();
    frameOverlay->selectOverlayElements(rect, candidates);
    frameOverlay->unlock();

    OEPointerT elem = GetClosestToPivot(candidates, pt);

    if (elem)
      ui = elem->userInfo();

    //m_renderPolicy->FrameUnlock();
  }

  if (ui.IsValid())
  {
    Index::FeaturesLoaderGuard guard(m_model.GetIndex(), ui.m_mwmID);

    FeatureType ft;
    guard.GetFeature(ui.m_offset, ft);

    // @TODO experiment with other pivots
    ASSERT_NOT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
    m2::PointD const center = feature::GetCenter(ft);

    GetAddressInfo(ft, center, info);

    pxPivot = GtoP(center);
    return true;
  }

  return false;
}

Framework::BookmarkOrPoi Framework::GetBookmarkOrPoi(m2::PointD const & pxPoint, m2::PointD & pxPivot,
                                                     search::AddressInfo & info, BookmarkAndCategory & bmCat)
{
  bmCat = GetBookmark(pxPoint);
  if (IsValid(bmCat))
    return Framework::BOOKMARK;

  if (GetVisiblePOI(pxPoint, pxPivot, info))
  {
    // We need almost the exact position of the bookmark, parameter 0.1 resolves the error in 2 pixels
    bmCat = GetBookmark(pxPivot, 0.1);
    if (IsValid(bmCat))
      return Framework::BOOKMARK;
    else
      return Framework::POI;
  }
  return Framework::NOTHING_FOUND;
}

Animator & Framework::GetAnimator()
{
  return m_animator;
}

Navigator & Framework::GetNavigator()
{
  return m_navigator;
}

shared_ptr<location::State> const & Framework::GetLocationState() const
{
  return m_informationDisplay.locationState();
}

StringsBundle const & Framework::GetStringsBundle()
{
  return m_stringsBundle;
}

string Framework::CodeGe0url(Bookmark const * bmk, bool const addName)
{
  double lat = MercatorBounds::YToLat(bmk->GetOrg().y);
  double lon = MercatorBounds::XToLon(bmk->GetOrg().x);
  return CodeGe0url(lat, lon, bmk->GetScale(), addName ? bmk->GetName() : "");
}

string Framework::CodeGe0url(double const lat, double const lon, double const zoomLevel, string const & name)
{
  size_t const resultSize = MapsWithMe_GetMaxBufferSize(name.size());

  string res(resultSize, 0);
  int const len = MapsWithMe_GenShortShowMapUrl(lat, lon, zoomLevel, name.c_str(), &res[0], res.size());

  ASSERT_LESS_OR_EQUAL(len, res.size(), ());
  res.resize(len);

  return res;
}

void Framework::DrawMapApiPoints(shared_ptr<PaintEvent> const & e)
{
  Navigator & navigator = GetNavigator();
  InformationDisplay & informationDisplay  = GetInformationDisplay();
  // get viewport limit rect
  m2::AnyRectD const & glbRect = navigator.Screen().GlobalRect();
  Drawer * pDrawer = e->drawer();

  vector<url_scheme::ApiPoint> const & v = GetMapApiPoints();

  for (size_t i = 0; i < v.size(); ++i)
  {
    m2::PointD const & org = m2::PointD(MercatorBounds::LonToX(v[i].m_lon),
                                        MercatorBounds::LatToY(v[i].m_lat));
    if (glbRect.IsPointInside(org))
      //ToDo Use Custom Pins
      //super magic hack!!! Only purple! Only hardcore
      informationDisplay.drawPlacemark(pDrawer, "api_pin", navigator.GtoP(org));
  }
}

void Framework::MapApiSetUriAndParse(string const & url)
{
  m_ParsedMapApi.SetUriAndParse(url);
}

//Dummy method. TODO create method that will run all layers without copy/past
bool Framework::GetMapApiPoint(m2::PointD const & pxPoint, url_scheme::ApiPoint & point)
{
  int const sm = TOUCH_PIXEL_RADIUS * GetVisualScale();
  m2::RectD const rect(PtoG(m2::PointD(pxPoint.x - sm, pxPoint.y - sm)),
                       PtoG(m2::PointD(pxPoint.x + sm, pxPoint.y + sm)));
  double minD = numeric_limits<double>::max();
  bool result = false;

  vector <url_scheme::ApiPoint> const & vect = m_ParsedMapApi.GetPoints();

  for (size_t i = 0; i < vect.size();++i)
  {
    m2::PointD const pt = m2::PointD(m2::PointD(MercatorBounds::LonToX(vect[i].m_lon),
                                                MercatorBounds::LatToY(vect[i].m_lat)));
    if (rect.IsPointInside(pt))
    {
      double const d = rect.Center().SquareLength(pt);
      if (d < minD)
      {
        point = vect[i];
        minD = d;
        result = true;
      }
    }
  }
  return result;
}

string Framework::GenerateApiBackUrl(url_scheme::ApiPoint const & point)
{
  string res = m_ParsedMapApi.GetGlobalBackUrl();
  if (!res.empty())
  {
    res += "pin?ll=" + strings::to_string(point.m_lat) + "," + strings::to_string(point.m_lon);
    if (!point.m_name.empty())
      res += "&n=" + UrlEncode(point.m_name);
    if (!point.m_id.empty())
      res += "&id=" + UrlEncode(point.m_id);
  }
  return res;
}
