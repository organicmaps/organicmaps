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

#include "../routing/route.hpp"
#include "../routing/dijkstra_router.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"

/// @todo Probably it's better to join this functionality.
//@{
#include "../indexer/feature_algo.hpp"
#include "../indexer/feature_utils.hpp"
//@}

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

namespace
{
  static const int BM_TOUCH_PIXEL_INCREASE = 20;
}

void Framework::AddMap(string const & file)
{
  LOG(LINFO, ("Loading map:", file));

  int const version = m_model.AddMap(file);
  switch (version)
  {
  case -1:
    // Error in adding map - do nothing.
    break;

  case feature::DataHeader::v1:
    // Now we do force delete of old (April 2011) maps.
    LOG(LINFO, ("Deleting old map:", file));
    RemoveMap(file);
    VERIFY ( my::DeleteFileX(GetPlatform().WritablePathForFile(file)), () );
    break;

  default:
    if (m_lowestMapVersion > version)
      m_lowestMapVersion = version;
    break;
  }
}

void Framework::RemoveMap(string const & file)
{
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

  shared_ptr<location::State> const & state = m_informationDisplay.locationState();
  state->OnLocationUpdate(rInfo);
  m_balloonManager.LocationChanged(rInfo);

  if (state->GetLocationProcessMode() != location::ELocationDoNothing)
    UpdateUserViewportChanged();
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
  : m_navigator(m_scales),
    m_animator(this),
    m_queryMaxScaleMode(false),
    m_width(0),
    m_height(0),
    m_informationDisplay(this),
    m_lowestMapVersion(numeric_limits<int>::max()),
    m_benchmarkEngine(0),
    m_bmManager(*this),
    m_balloonManager(*this),
    m_fixedSearchResults(0),
    m_locationChangedSlotID(-1)
{
  // Checking whether we should enable benchmark.
  bool isBenchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", isBenchmarkingEnabled);
  if (isBenchmarkingEnabled)
    m_benchmarkEngine = new BenchmarkEngine(this);

  m_ParsedMapApi.SetController(&m_bmManager.UserMarksGetController(UserMarkContainer::API_MARK));

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
  m_stringsBundle.SetDefaultString("routes", "Routes");

  m_animController.reset(new anim::Controller());

  // Init GUI controller.
  m_guiController.reset(new gui::Controller());
  m_guiController->SetStringsBundle(&m_stringsBundle);

  // Init information display.
  m_informationDisplay.setController(m_guiController.get());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
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

#ifndef OMIM_OS_DESKTOP
  //Init guides manager
  m_storage.GetGuideManager().RestoreFromFile();
#endif

  // Restore temporary states from persistent Settings storage
  RestoreSesame();
}

Framework::~Framework()
{
  delete m_benchmarkEngine;
}

double Framework::GetVisualScale() const
{
  return m_scales.GetVisualScale();
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
  return m_storage.CountryStatusEx(index);
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
  if (GetPlatform().HasBookmarks())
    m_bmManager.LoadBookmarks();
}

size_t Framework::AddBookmark(size_t categoryIndex, const m2::PointD & ptOrg, BookmarkData & bm)
{
  return m_bmManager.AddBookmark(categoryIndex, ptOrg, bm);
}

size_t Framework::MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex)
{
  return m_bmManager.MoveBookmark(bmIndex, curCatIndex, newCatIndex);
}

void Framework::ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm)
{
  m_bmManager.ReplaceBookmark(catIndex, bmIndex, bm);
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

bool Framework::DeleteBmCategory(size_t index)
{
  return m_bmManager.DeleteBmCategory(index);
}

void Framework::ShowBookmark(BookmarkAndCategory const & bnc)
{
  StopLocationFollow();

  // show ballon above
  Bookmark const * bmk = m_bmManager.GetBmCategory(bnc.first)->GetBookmark(bnc.second);

  double scale = bmk->GetScale();
  if (scale == -1.0)
    scale = scales::GetUpperComfortScale();

  ShowRectExVisibleScale(m_scales.GetRectForDrawScale(scale, bmk->GetOrg()));
  Bookmark * mark = GetBmCategory(bnc.first)->GetBookmark(bnc.second);
  ActivateUserMark(mark);
  m_balloonManager.OnShowMark(mark);
}

void Framework::ShowTrack(Track const & track)
{
  StopLocationFollow();
  ShowRectEx(track.GetLimitRect());
}

void Framework::ClearBookmarks()
{
  m_bmManager.ClearItems();
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

string const GenerateValidAndUniqueFilePathForKML(string const & fileName)
{
  string filePath = BookmarkCategory::RemoveInvalidSymbols(fileName);
  filePath = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath);
  return filePath;
}

}

bool Framework::AddBookmarksFile(string const & filePath)
{
  string const fileExt = GetFileExt(filePath);
  string fileSavePath;
  if (fileExt == BOOKMARKS_FILE_EXTENSION)
  {
    fileSavePath = GenerateValidAndUniqueFilePathForKML(GetFileName(filePath));
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

      fileSavePath = GenerateValidAndUniqueFilePathForKML(kmlFileName);
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

void Framework::GetLocalMaps(vector<string> & outMaps) const
{
  Platform & pl = GetPlatform();
  pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, outMaps);

#ifdef OMIM_OS_ANDROID
  // On Android World and WorldCoasts can be stored in alternative /Android/obb/ path.
  char const * arrCheck[] = { WORLD_FILE_NAME DATA_FILE_EXTENSION,
                              WORLD_COASTS_FILE_NAME DATA_FILE_EXTENSION };

  for (size_t i = 0; i < ARRAY_SIZE(arrCheck); ++i)
    if (find(outMaps.begin(), outMaps.end(), arrCheck[i]) == outMaps.end())
      outMaps.push_back(arrCheck[i]);
#endif
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


void Framework::OnSize(int w, int h)
{
  if (w < 2) w = 2;
  if (h < 2) h = 2;

  m_navigator.OnSize(0, 0, w, h);

  if (m_renderPolicy)
  {
    m_informationDisplay.setDisplayRect(m2::RectI(0, 0, w, h));
    m_renderPolicy->OnSize(w, h);
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
  return m_scales.GetDrawTileScale(m_navigator.Screen());
}

RenderPolicy::TRenderFn Framework::DrawModelFn()
{
  bool const isTiling = m_renderPolicy->IsTiling();
  return bind(&Framework::DrawModel, this, _1, _2, _3, _4, isTiling);
}

void Framework::DrawModel(shared_ptr<PaintEvent> const & e,
                          ScreenBase const & screen,
                          m2::RectD const & renderRect,
                          int baseScale, bool isTilingQuery)
{
  m2::RectD selectRect;
  m2::RectD clipRect;

  double const inflationSize = m_scales.GetClipRectInflation();
  screen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), clipRect);
  screen.PtoG(m2::RectD(renderRect), selectRect);

  int drawScale = m_scales.GetDrawTileScale(baseScale);
  fwork::FeatureProcessor doDraw(clipRect, screen, e, drawScale);
  if (m_queryMaxScaleMode)
    drawScale = scales::GetUpperScale();

  try
  {
    int const upperScale = scales::GetUpperScale();
    if (isTilingQuery && drawScale <= upperScale)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, drawScale);
    else
      m_model.ForEachFeature(selectRect, doDraw, min(upperScale, drawScale));
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
    m_informationDisplay.setEmptyCountryIndex(GetCountryIndex(GetViewportCenter()));

  m_informationDisplay.enableCountryStatusDisplay(isEmptyModel);
  bool isCompassEnabled = ang::AngleIn2PI(m_navigator.Screen().GetAngle()) > my::DegToRad(3.0);
  m_informationDisplay.enableCompassArrow(isCompassEnabled ||
                                          (m_informationDisplay.isCompassArrowEnabled() && m_navigator.InAction()));

  m_informationDisplay.setCompassArrowAngle(m_navigator.Screen().GetAngle());

  m_informationDisplay.setScreen(m_navigator.Screen());

  int drawScale = GetDrawScale();
  m_informationDisplay.setDebugInfo(0, drawScale);

  m_informationDisplay.enableRuler(drawScale > 4);
#ifdef DEBUG
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_informationDisplay.doDraw(pDrawer);
  pScreen->endFrame();

  m_bmManager.DrawItems(e);

  m_guiController->UpdateElements();
  m_guiController->DrawFrame(pScreen);
}

/// Function for calling from platform dependent-paint function.
void Framework::DoPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
  {
    m_renderPolicy->DrawFrame(e, m_navigator.Screen());

    // don't render additional elements if guiController wasn't initialized
    if (m_guiController->GetCacheScreen() != NULL)
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

shared_ptr<MoveScreenTask> Framework::SetViewportCenterAnimated(m2::PointD const & endPt)
{
  anim::Controller::Guard guard(GetAnimController());
  ScreenBase const & s = m_navigator.Screen();
  m2::PointD const & startPt = s.GetOrg();
  double const speed = m_navigator.ComputeMoveSpeed(startPt, endPt);

  return m_animator.MoveScreen(startPt, endPt, speed);
}

m2::AnyRectD Framework::ToRotated(m2::RectD const & rect) const
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(),
                      m_navigator.Screen().GetAngle(),
                      m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
}

void Framework::CheckMinGlobalRect(m2::RectD & rect) const
{
  m2::RectD const minRect = m_scales.GetRectForDrawScale(scales::GetUpperStyleScale(), rect.Center());
  if (minRect.IsRectInside(rect))
    rect = minRect;
}

void Framework::CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale/* = -1*/) const
{
  CheckMinGlobalRect(rect);

  m2::PointD const c = rect.Center();
  int const worldS = scales::GetUpperWorldScale();

  int scale = m_scales.GetDrawTileScale(rect);
  if (scale > worldS && !IsCountryLoaded(c))
  {
    // country is not loaded - limit on world scale
    rect = m_scales.GetRectForDrawScale(worldS, c);
    scale = worldS;
  }

  if (maxScale != -1 && scale > maxScale)
  {
    // limit on passed maximal scale
    rect = m_scales.GetRectForDrawScale(maxScale, c);
  }
}

void Framework::ShowRect(double lat, double lon, double zoom)
{
  m2::PointD center(MercatorBounds::FromLatLon(lat, lon));
  ShowRectEx(m_scales.GetRectForDrawScale(zoom, center));
}

void Framework::ShowRect(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  m_navigator.SetFromRect(m2::AnyRectD(rect));
  Invalidate();
}

void Framework::ShowRectEx(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  ShowRectFixed(rect);
}

void Framework::ShowRectExVisibleScale(m2::RectD rect, int maxScale/* = -1*/)
{
  CheckMinMaxVisibleScale(rect, maxScale);

  ShowRectFixed(rect);
}

void Framework::ShowRectFixed(m2::RectD const & rect)
{
  ShowRectFixedAR(ToRotated(rect));
}

void Framework::ShowRectFixedAR(m2::AnyRectD const & rect)
{
  double const halfSize = m_scales.GetTileSize() / 2.0;
  m2::RectD etalonRect(-halfSize, -halfSize, halfSize, halfSize);

  m2::PointD const pxCenter = m_navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  m_navigator.SetFromRects(rect, etalonRect);

  Invalidate();
}

void Framework::UpdateUserViewportChanged()
{
  if (IsISActive())
  {
    (void)GetCurrentPosition(m_lastSearch.m_lat, m_lastSearch.m_lon);
    m_lastSearch.m_callback = bind(&Framework::OnSearchResultsCallback, this, _1);
    m_lastSearch.SetSearchMode(search::SearchParams::IN_VIEWPORT);
    m_lastSearch.SetForceSearch(false);

    (void)GetSearchEngine()->Search(m_lastSearch, GetCurrentViewport(), true);
  }
}

void Framework::OnSearchResultsCallback(search::Results const & results)
{
  if (!results.IsEndMarker() && results.GetCount() > 0)
  {
    // Got here from search thread. Need to switch into GUI thread to modify search mark container.
    // Do copy the results structure to pass into GUI thread.
    GetPlatform().RunOnGuiThread(bind(&Framework::OnSearchResultsCallbackUI, this, results));
  }
}

void Framework::OnSearchResultsCallbackUI(search::Results const & results)
{
  if (IsISActive())
  {
    FillSearchResultsMarks(results);

    Invalidate();
  }
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
  {
    m_renderPolicy->StopDrag();
    UpdateUserViewportChanged();
  }
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

    UpdateUserViewportChanged();
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

  m_animController->AddTask(m_navigator.ScaleToPointAnim(pt, e.ScaleFactor(), 0.25));

  Invalidate();
  UpdateUserViewportChanged();
}

void Framework::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

void Framework::Scale(double scale)
{
  m_animController->AddTask(m_navigator.ScaleAnim(scale));

  Invalidate();
  UpdateUserViewportChanged();
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
  {
    m_renderPolicy->StopScale();
    UpdateUserViewportChanged();
  }
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

///////////////////////////// ROUTING /////////////////////////////////////////////////

namespace
{
char const * ROUTER_ALL = "all";
char const * ROUTER_HELICOPTER = "helicopter";
char const * ROUTER_OSRM = "osrm";
char const * ROUTER_MAPSME = "routeme";
}

void Framework::SetRouteStart(m2::PointD const & mercatorStart)
{
  m_routingEngine.SetStartingPoint(mercatorStart);
  m_routingEngine.Calculate(ROUTER_ALL, bind(&Framework::OnRouteCalculated, this, _1));
}

void Framework::SetRouteEnd(m2::PointD const & mercatorEnd)
{
  m_routingEngine.SetFinalPoint(mercatorEnd);
  m_routingEngine.Calculate(ROUTER_ALL, bind(&Framework::OnRouteCalculated, this, _1));
}

bool Framework::IsRoutingEnabled() const
{
  return m_routingEngine.IsRoutingEnabled();
}

void Framework::OnRouteCalculated(routing::Route const & route)
{
  if (!route.IsValid())
  {
    LOG(LWARNING, ("Route calculation has failed.", route.GetName()));
    return;
  }

  // Get/create temporary category for route track
  BookmarkManager & bm = GetBookmarkManager();
  string const categoryName = m_stringsBundle.GetString("routes");
  BookmarkCategory * cat = 0;
  for (size_t i = 0; i < bm.GetBmCategoriesCount(); ++i)
  {
    if (bm.GetBmCategory(i)->GetName() == categoryName)
    {
      cat = bm.GetBmCategory(i);
      break;
    }
  }
  if (!cat)
    cat = bm.GetBmCategory(bm.CreateBmCategory(categoryName));

  Track track(route.GetPoly());
  track.SetName(route.GetName());
  string const & source = route.GetRouterId();

  graphics::Color routeColor = graphics::Color::Black();
  if (source == ROUTER_HELICOPTER)
    routeColor = graphics::Color::Red();
  else if (source == ROUTER_OSRM)
    routeColor = graphics::Color::Blue();
  else if (source == ROUTER_MAPSME)
    routeColor = graphics::Color::Green();

  track.SetColor(routeColor);
  cat->AddTrack(track);
}

void Framework::DeleteRoutes()
{
  /// @todo
}

routing::IRouter * Framework::CreateRouter()
{
  return new routing::DijkstraRouter(&m_model.GetIndex());
}

void Framework::RestoreSesame()
{
  bool enable = false;
  if (Settings::Get(ROUTER_HELICOPTER, enable) && enable)
    m_routingEngine.AddRouter(ROUTER_HELICOPTER);
  if (Settings::Get(ROUTER_OSRM, enable) && enable)
    m_routingEngine.AddRouter(ROUTER_OSRM);
  if (Settings::Get(ROUTER_MAPSME, enable) && enable)
    m_routingEngine.AddRouter(CreateRouter());
}

/// Activates hidden features via search queries
bool Framework::SesameOpen(search::SearchParams const & params)
{
  // Quick check
  string const & q = params.m_query;
  if (!q.empty() && q[0] != '?')
    return false;

  char const * searchResult = 0;
  if (params.m_query == "?routing on")
  {
    m_routingEngine.AddRouter(ROUTER_HELICOPTER);
    m_routingEngine.AddRouter(ROUTER_OSRM);
    m_routingEngine.AddRouter(CreateRouter());

    // Enable all other engines here
    Settings::Set(ROUTER_HELICOPTER, true);
    Settings::Set(ROUTER_OSRM, true);
    Settings::Set(ROUTER_MAPSME, true);
    searchResult = "All routing engines activated";
  }
  else if (params.m_query == "?routing off")
  {
    m_routingEngine.RemoveRouter(ROUTER_HELICOPTER);
    m_routingEngine.RemoveRouter(ROUTER_OSRM);
    m_routingEngine.RemoveRouter(ROUTER_MAPSME);

    // Disable all other engines here
    Settings::Set(ROUTER_HELICOPTER, false);
    Settings::Set(ROUTER_OSRM, false);
    Settings::Set(ROUTER_MAPSME, false);
    searchResult = "All routing engines disabled";
  }
  else if (params.m_query == "?heli on")
  {
    m_routingEngine.AddRouter(ROUTER_HELICOPTER);
    Settings::Set(ROUTER_HELICOPTER, true);
    searchResult = "Helicopter routing activated";
  }
  else if (params.m_query == "?heli off")
  {
    m_routingEngine.RemoveRouter(ROUTER_HELICOPTER);
    Settings::Set(ROUTER_HELICOPTER, false);
    searchResult = "Helicopter routing disabled";
  }
  else if (params.m_query == "?online on" || params.m_query == "?osrm on")
  {
    m_routingEngine.AddRouter(ROUTER_OSRM);
    Settings::Set(ROUTER_OSRM, true);
    searchResult = "OSRM routing activated";
  }
  else if (params.m_query == "?online off" || params.m_query == "?osrm off")
  {
    m_routingEngine.RemoveRouter(ROUTER_OSRM);
    Settings::Set(ROUTER_OSRM, false);
    searchResult = "OSRM routing disabled";
  }
  else if (params.m_query == "?routeme on")
  {
    m_routingEngine.AddRouter(CreateRouter());
    Settings::Set(ROUTER_MAPSME, true);
    searchResult = "maps.me routing activated";
  }
  else if (params.m_query == "?routeme off")
  {
    m_routingEngine.RemoveRouter(ROUTER_MAPSME);
    Settings::Set(ROUTER_MAPSME, false);
    searchResult = "maps.me routing disabled";
  }

  if (searchResult)
  {
    search::Results results;
    results.AddResult(search::Result(searchResult, ""));
    params.m_callback(results);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////

bool Framework::Search(search::SearchParams const & params)
{
  // Activate hidden features
  if (SesameOpen(params))
    return true;

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
  UserMarkContainer::Type const type = UserMarkContainer::SEARCH_MARK;
  m_bmManager.UserMarksSetVisible(type, true);
  m_bmManager.UserMarksClear(type);
  m_bmManager.UserMarksSetDrawable(type, false);

  m_lastSearch.Clear();
  m_fixedSearchResults = 0;

  int scale;
  m2::PointD center;

  using namespace search;
  using namespace feature;

  switch (res.GetResultType())
  {
    case Result::RESULT_FEATURE:
    {
      FeatureID const id = res.GetFeatureID();
      Index::FeaturesLoaderGuard guard(m_model.GetIndex(), id.m_mwm);

      FeatureType ft;
      guard.GetFeature(id.m_offset, ft);

      scale = GetFeatureViewportScale(TypesHolder(ft));
      center = GetCenter(ft, scale);
      break;
    }

    case Result::RESULT_LATLON:
      scale = scales::GetUpperComfortScale();
      center = res.GetFeatureCenter();
      break;

    default:
      return;
  }

  ShowRectExVisibleScale(m_scales.GetRectForDrawScale(scale, center));
  StopLocationFollow();

  search::AddressInfo info;
  info.MakeFrom(res);

  SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(m_bmManager.UserMarksAddMark(type, center));
  mark->SetInfo(info);

  m_balloonManager.OnShowMark(mark);
}

size_t Framework::ShowAllSearchResults()
{
  using namespace search;

  Results results;
  GetSearchEngine()->GetResults(results);

  size_t count = results.GetCount();
  switch (count)
  {
  case 1:
    {
      Result const & r = results.GetResult(0);
      if (!r.IsSuggest())
        ShowSearchResult(r);
      else
        count = 0;
      // do not put break here
    }
  case 0:
    return count;
  }

  m_fixedSearchResults = 0;
  FillSearchResultsMarks(results);
  m_fixedSearchResults = count;

  // Setup viewport according to results.
  m2::AnyRectD viewport = m_navigator.Screen().GlobalRect();
  m2::PointD const center = viewport.Center();

  double minDistance = numeric_limits<double>::max();
  int minInd = -1;
  for (size_t i = 0; i < count; ++i)
  {
    Result const & r = results.GetResult(i);
    if (r.HasPoint())
    {
      double const dist = center.SquareLength(r.GetFeatureCenter());
      if (dist < minDistance)
      {
        minDistance = dist;
        minInd = static_cast<int>(i);
      }
    }
  }

  if (minInd != -1)
  {
    m2::PointD const pt = results.GetResult(minInd).GetFeatureCenter();
    if (!viewport.IsPointInside(pt))
    {
      viewport.SetSizesToIncludePoint(pt);

      ShowRectFixedAR(viewport);
      StopLocationFollow();
    }
    else
      minInd = -1;
  }

  if (minInd == -1)
    Invalidate();

  return count;
}

void Framework::FillSearchResultsMarks(search::Results const & results)
{
  UserMarkContainer::Type const type = UserMarkContainer::SEARCH_MARK;
  m_bmManager.UserMarksSetVisible(type, true);
  m_bmManager.UserMarksSetDrawable(type, true);
  m_bmManager.UserMarksClear(type, m_fixedSearchResults);

  size_t const count = results.GetCount();
  for (size_t i = 0; i < count; ++i)
  {
    using namespace search;

    Result const & r = results.GetResult(i);
    if (r.HasPoint())
    {
      AddressInfo info;
      info.MakeFrom(r);

      m2::PointD const pt = r.GetFeatureCenter();
      SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(m_bmManager.UserMarksAddMark(type, pt));
      mark->SetInfo(info);
    }
  }
}

void Framework::CancelInteractiveSearch()
{
  m_lastSearch.Clear();
  m_bmManager.UserMarksClear(UserMarkContainer::SEARCH_MARK);

  m_fixedSearchResults = 0;

  Invalidate();
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

    azimut = ang::AngleTo(MercatorBounds::FromLatLon(lat, lon), point) + north;

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
  m_bmManager.ResetScreen();
  m_guiController->ResetRenderParams();
  m_renderPolicy.reset();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy)
  {
    m_renderPolicy->SetAnimController(m_animController.get());

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

    m_renderPolicy->SetRenderFn(DrawModelFn());

    m_scales.SetParams(m_renderPolicy->VisualScale(), m_renderPolicy->TileSize());

    if (m_benchmarkEngine)
      m_benchmarkEngine->Start();
  }
}

void Framework::InitGuiSubsystem()
{
  if (m_renderPolicy)
  {
    gui::Controller::RenderParams rp(m_renderPolicy->Density(),
                                     bind(&WindowHandle::invalidate,
                                          m_renderPolicy->GetWindowHandle().get()),
                                     m_renderPolicy->GetGlyphCache(),
                                     m_renderPolicy->GetCacheScreen().get());

    m_guiController->SetRenderParams(rp);
    m_informationDisplay.setVisualScale(m_renderPolicy->VisualScale());
    m_balloonManager.RenderPolicyCreated(m_renderPolicy->Density());

    if (m_width != 0 && m_height != 0)
      OnSize(m_width, m_height);

    // init Bookmark manager
    //@{
    graphics::Screen::Params pr;
    pr.m_resourceManager = m_renderPolicy->GetResourceManager();
    pr.m_threadSlot = m_renderPolicy->GetResourceManager()->guiThreadSlot();
    pr.m_renderContext = m_renderPolicy->GetRenderContext();

    pr.m_storageType = graphics::EMediumStorage;
    pr.m_textureType = graphics::ESmallTexture;

    m_bmManager.SetScreen(m_renderPolicy->GetCacheScreen().get());
    //@}

    // Do full invalidate instead of any "pending" stuff.
    Invalidate();
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

bool Framework::ShowMapForURL(string const & url)
{
  m2::PointD point;
  m2::RectD rect;
  string name;
  UserMark const * apiMark = 0;

  enum ResultT { FAILED, NEED_CLICK, NO_NEED_CLICK };
  ResultT result = FAILED;

  // always hide current balloon here
  m_balloonManager.Hide();

  using namespace url_scheme;
  using namespace strings;

  if (StartsWith(url, "geo"))
  {
    Info info;
    ParseGeoURL(url, info);
    if (info.IsValid())
    {
      point = MercatorBounds::FromLatLon(info.m_lat, info.m_lon);
      rect = m_scales.GetRectForDrawScale(info.m_zoom, point);
      result = NEED_CLICK;
    }
  }
  else if (StartsWith(url, "ge0"))
  {
    Ge0Parser parser;
    double zoom;
    ApiPoint pt;

    if (parser.Parse(url, pt, zoom))
    {
      point = MercatorBounds::FromLatLon(pt.m_lat, pt.m_lon);
      rect = m_scales.GetRectForDrawScale(zoom, point);
      name = pt.m_name;
      result = NEED_CLICK;
    }
  }
  else if (StartsWith(url, "mapswithme://") || StartsWith(url, "mwm://"))
  {
    m_bmManager.UserMarksClear(UserMarkContainer::API_MARK);

    if (m_ParsedMapApi.SetUriAndParse(url))
    {
      if (!m_ParsedMapApi.GetViewportRect(m_scales, rect))
        rect = ScalesProcessor::GetWorldRect();

      if ((apiMark = m_ParsedMapApi.GetSinglePoint()))
        result = NEED_CLICK;
      else
        result = NO_NEED_CLICK;
    }
  }

  if (result != FAILED)
  {
    // set viewport and stop follow mode if any
    StopLocationFollow();
    ShowRectExVisibleScale(rect);

    if (result != NO_NEED_CLICK)
    {
      if (apiMark)
      {
        LOG(LINFO, ("Show API mark:", static_cast<ApiMarkPoint const *>(apiMark)->GetName()));

        m_balloonManager.OnShowMark(apiMark);
      }
      else
      {
        PoiMarkPoint * mark = GetAddressMark(point);
        if (!name.empty())
          mark->SetName(name);
        m_balloonManager.OnShowMark(mark);
      }
    }
    else
    {
      m_balloonManager.RemovePin();
      m_balloonManager.Dismiss();
    }

    return true;
  }
  else
    return false;
}

void Framework::SetViewPortASync(m2::RectD const & r)
{
  m2::AnyRectD const aRect(r);
  m_animator.ChangeViewport(aRect, aRect, 0.0);
}

void Framework::UpdateSelectedMyPosition(m2::PointD const & pt)
{
  MyPositionMarkPoint * myPositionMark = UserMarkContainer::UserMarkForMyPostion();
  myPositionMark->SetPtOrg(pt);
  ActivateUserMark(myPositionMark, false);
}

void Framework::DisconnectMyPositionUpdate()
{
  if (m_locationChangedSlotID != -1)
  {
    GetLocationState()->RemovePositionChangedListener(m_locationChangedSlotID);
    m_locationChangedSlotID = -1;
  }
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

void Framework::ActivateUserMark(UserMark const * mark, bool needAnim)
{
  if (mark != UserMarkContainer::UserMarkForMyPostion())
    DisconnectMyPositionUpdate();
  m_bmManager.ActivateMark(mark, needAnim);
}

bool Framework::HasActiveUserMark() const
{
  return m_bmManager.UserMarkHasActive();
}

namespace
{
  class MainTouchRectHolder : public BookmarkManager::TouchRectHolder
  {
  public:
    MainTouchRectHolder(m2::AnyRectD const & defaultRect, m2::AnyRectD const & bmRect)
      : m_defRect(defaultRect)
      , m_bmRect(bmRect)
    {
    }

    m2::AnyRectD const & GetTouchArea(UserMarkContainer::Type type) const
    {
      switch (type)
      {
      case UserMarkContainer::BOOKMARK_MARK:
        return m_bmRect;
      default:
        return m_defRect;
      }
    }

  private:
    m2::AnyRectD const & m_defRect;
    m2::AnyRectD const & m_bmRect;
  };
}

UserMark const * Framework::GetUserMark(m2::PointD const & pxPoint, bool isLongPress)
{
  DisconnectMyPositionUpdate();
  m2::AnyRectD rect;
  m_navigator.GetTouchRect(pxPoint, TOUCH_PIXEL_RADIUS * GetVisualScale(), rect);

  shared_ptr<location::State> const & locationState = GetLocationState();
  if (locationState->HasPosition())
  {
    m2::PointD const & glPivot = locationState->Position();
    if (rect.IsPointInside(glPivot))
    {
      search::AddressInfo info;
      info.m_name = m_stringsBundle.GetString("my_position");
      MyPositionMarkPoint * myPostition = UserMarkContainer::UserMarkForMyPostion();
      m_locationChangedSlotID = locationState->AddPositionChangedListener(bind(&Framework::UpdateSelectedMyPosition, this, _1));
      myPostition->SetPtOrg(glPivot);
      myPostition->SetInfo(info);
      return myPostition;
    }
  }

  m2::AnyRectD bmSearchRect;
  double const pxWidth  =  TOUCH_PIXEL_RADIUS * GetVisualScale();
  double const pxHeight = (TOUCH_PIXEL_RADIUS + BM_TOUCH_PIXEL_INCREASE) * GetVisualScale();
  m_navigator.GetTouchRect(pxPoint + m2::PointD(0, BM_TOUCH_PIXEL_INCREASE), pxWidth, pxHeight, bmSearchRect);
  MainTouchRectHolder holder(rect, bmSearchRect);
  UserMark const * mark = m_bmManager.FindNearestUserMark(holder);

  if (mark == NULL)
  {
    bool needMark = false;
    m2::PointD pxPivot;
    search::AddressInfo info;
    if (GetVisiblePOI(pxPoint, pxPivot, info))
      needMark = true;
    else if (isLongPress)
    {
      GetAddressInfoForPixelPoint(pxPoint, info);
      pxPivot = pxPoint;
      needMark = true;
    }

    if (needMark)
    {
      PoiMarkPoint * poiMark = UserMarkContainer::UserMarkForPoi();
      poiMark->SetPtOrg(m_navigator.PtoG(pxPivot));
      poiMark->SetInfo(info);
      mark = poiMark;
    }
  }

  return mark;
}

PoiMarkPoint * Framework::GetAddressMark(m2::PointD const & globalPoint) const
{
  search::AddressInfo info;
  GetAddressInfoForGlobalPoint(globalPoint, info);
  PoiMarkPoint * mark = UserMarkContainer::UserMarkForPoi();
  mark->SetPtOrg(globalPoint);
  mark->SetInfo(info);
  return mark;
}

BookmarkAndCategory Framework::FindBookmark(UserMark const * mark) const
{
  BookmarkAndCategory empty = MakeEmptyBookmarkAndCategory();
  BookmarkAndCategory result = empty;
  for (size_t i = 0; i < GetBmCategoriesCount(); ++i)
  {
    if (mark->GetContainer() == GetBmCategory(i))
    {
      result.first = i;
      break;
    }
  }

  ASSERT(result.first != empty.first, ());
  BookmarkCategory const * cat = GetBmCategory(result.first);
  for (size_t i = 0; i < cat->GetBookmarksCount(); ++i)
  {
    if (mark == cat->GetBookmark(i))
    {
      result.second = i;
      break;
    }
  }

  ASSERT(result != empty, ());
  return result;
}

StringsBundle const & Framework::GetStringsBundle()
{
  return m_stringsBundle;
}

string Framework::CodeGe0url(Bookmark const * bmk, bool addName)
{
  double lat = MercatorBounds::YToLat(bmk->GetOrg().y);
  double lon = MercatorBounds::XToLon(bmk->GetOrg().x);
  return CodeGe0url(lat, lon, bmk->GetScale(), addName ? bmk->GetName() : "");
}

string Framework::CodeGe0url(double lat, double lon, double zoomLevel, string const & name)
{
  size_t const resultSize = MapsWithMe_GetMaxBufferSize(name.size());

  string res(resultSize, 0);
  int const len = MapsWithMe_GenShortShowMapUrl(lat, lon, zoomLevel, name.c_str(), &res[0], res.size());

  ASSERT_LESS_OR_EQUAL(len, res.size(), ());
  res.resize(len);

  return res;
}

string Framework::GenerateApiBackUrl(ApiMarkPoint const & point)
{
  string res = m_ParsedMapApi.GetGlobalBackUrl();
  if (!res.empty())
  {
    double lat, lon;
    point.GetLatLon(lat, lon);
    res += "pin?ll=" + strings::to_string(lat) + "," + strings::to_string(lon);
    if (!point.GetName().empty())
      res += "&n=" + UrlEncode(point.GetName());
    if (!point.GetID().empty())
      res += "&id=" + UrlEncode(point.GetID());
  }
  return res;
}

bool Framework::IsDataVersionUpdated()
{
  int64_t storedVersion;
  if (Settings::Get("DataVersion", storedVersion))
  {
    return storedVersion < m_storage.GetCurrentDataVersion();
  }
  // no key in the settings, assume new version
  return true;
}

void Framework::UpdateSavedDataVersion()
{
  Settings::Set("DataVersion", m_storage.GetCurrentDataVersion());
}

bool Framework::GetGuideInfo(storage::TIndex const & index, guides::GuideInfo & info) const
{
  return m_storage.GetGuideManager().GetGuideInfo(m_storage.CountryFileName(index), info);
}
