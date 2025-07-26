#include "qt/screenshoter.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/preferred_languages.hpp"

#include "indexer/scales.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <QtGui/QPixmap>

#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>

namespace qt
{
namespace
{
bool ParsePoint(std::string_view s, char const * delim, m2::PointD & pt, uint8_t & zoom)
{
  // Order in string is: lat, lon, zoom.
  strings::SimpleTokenizer iter(s, delim);
  if (!iter)
    return false;

  double lat;
  double lon;
  uint8_t zoomLevel;
  if (strings::to_double(*iter, lat) && mercator::ValidLat(lat) && ++iter && strings::to_double(*iter, lon) &&
      mercator::ValidLon(lon) && ++iter && strings::to_uint(*iter, zoomLevel) && zoomLevel >= 1 &&
      zoomLevel <= scales::GetUpperStyleScale())
  {
    pt = mercator::FromLatLon(lat, lon);
    zoom = zoomLevel;
    return true;
  }
  return false;
}

bool ParseRect(std::string_view s, char const * delim, m2::RectD & rect)
{
  // Order in string is: latLeftBottom, lonLeftBottom, latRigthTop, lonRigthTop.
  strings::SimpleTokenizer iter(s, delim);
  if (!iter)
    return false;

  double latLeftBottom;
  double lonLeftBottom;
  double latRigthTop;
  double lonRigthTop;
  if (strings::to_double(*iter, latLeftBottom) && mercator::ValidLat(latLeftBottom) && ++iter &&
      strings::to_double(*iter, lonLeftBottom) && mercator::ValidLon(lonLeftBottom) && ++iter &&
      strings::to_double(*iter, latRigthTop) && mercator::ValidLat(latRigthTop) && ++iter &&
      strings::to_double(*iter, lonRigthTop) && mercator::ValidLon(lonRigthTop))
  {
    rect =
        m2::RectD(mercator::FromLatLon(latLeftBottom, lonLeftBottom), mercator::FromLatLon(latRigthTop, lonRigthTop));
    return true;
  }
  return false;
}

bool ParsePointsStr(std::string const & pointsStr, std::list<std::pair<m2::PointD, int>> & points)
{
  strings::SimpleTokenizer tupleIter(pointsStr, ";");
  m2::PointD pt;
  uint8_t zoom;
  while (tupleIter)
  {
    if (ParsePoint(*tupleIter, ", \t", pt, zoom))
    {
      points.emplace_back(pt, zoom);
    }
    else
    {
      LOG(LWARNING, ("Failed to parse point and zoom:", *tupleIter));
      return false;
    }
    ++tupleIter;
  }
  return true;
}

bool ParseRectsStr(std::string const & rectsStr, std::list<m2::RectD> & rects)
{
  strings::SimpleTokenizer tupleIter(rectsStr, ";");
  m2::RectD rect;
  while (tupleIter)
  {
    if (ParseRect(*tupleIter, ", \t", rect))
    {
      rects.push_back(rect);
    }
    else
    {
      LOG(LWARNING, ("Failed to parse rect:", *tupleIter));
      return false;
    }
    ++tupleIter;
  }
  return true;
}
}  // namespace

Screenshoter::Screenshoter(ScreenshotParams const & screenshotParams, Framework & framework, QOpenGLWidget * widget)
  : m_screenshotParams(screenshotParams)
  , m_framework(framework)
  , m_widget(widget)
{
  m_framework.GetStorage().Subscribe(std::bind(&Screenshoter::OnCountryChanged, this, std::placeholders::_1),
                                     [](storage::CountryId const &, downloader::Progress const &) {});
}

void Screenshoter::Start()
{
  if (m_state != State::NotStarted)
    return;

  if (m_screenshotParams.m_dstPath.empty())
    m_screenshotParams.m_dstPath = base::JoinPath(m_screenshotParams.m_kmlPath, "screenshots");

  LOG(LINFO, ("\nScreenshoter parameters:"
              "\n  mode:",
              m_screenshotParams.m_mode, "\n  kml_path:", m_screenshotParams.m_kmlPath,
              "\n  points:", m_screenshotParams.m_points, "\n  rects:", m_screenshotParams.m_rects,
              "\n  dst_path:", m_screenshotParams.m_dstPath, "\n  width:", m_screenshotParams.m_width,
              "\n  height:", m_screenshotParams.m_height, "\n  dpi_scale:", m_screenshotParams.m_dpiScale));

  if (!Platform::MkDirChecked(m_screenshotParams.m_dstPath))
  {
    ChangeState(State::FileError);
    return;
  }

  if (!PrepareItemsToProcess())
    return;

  ChangeState(State::Ready);
  ProcessNextItem();
}

void Screenshoter::ProcessNextItem()
{
  CHECK_EQUAL(m_state, State::Ready, ());

  switch (m_screenshotParams.m_mode)
  {
  case ScreenshotParams::Mode::KmlFiles: return PrepareToProcessKml();
  case ScreenshotParams::Mode::Points: return ProcessNextPoint();
  case ScreenshotParams::Mode::Rects: return ProcessNextRect();
  }
  UNREACHABLE();
}

void Screenshoter::PrepareToProcessKml()
{
  if (m_framework.GetBookmarkManager().AreSymbolSizesAcquired([this] { ProcessNextKml(); }))
    ProcessNextKml();
}

void Screenshoter::ProcessNextKml()
{
  std::string const postfix = "_" + languages::GetCurrentNorm();
  std::unique_ptr<kml::FileData> kmlData;
  while (kmlData == nullptr && !m_filesToProcess.empty())
  {
    auto const filePath = m_filesToProcess.front();
    m_filesToProcess.pop_front();
    m_nextScreenshotName = base::GetNameFromFullPathWithoutExt(filePath) + postfix;

    ChangeState(State::LoadKml);
    kmlData = LoadKmlFile(filePath, KmlFileType::Text);
    if (kmlData != nullptr && kmlData->m_bookmarksData.empty() && kmlData->m_tracksData.empty())
      kmlData.reset();
  }

  if (kmlData == nullptr)
  {
    ChangeState(State::Done);
    return;
  }

  kmlData->m_categoryData.m_visible = true;

  BookmarkManager::KMLDataCollection collection;
  collection.emplace_back("", std::move(kmlData));

  auto & bookmarkManager = m_framework.GetBookmarkManager();
  auto es = bookmarkManager.GetEditSession();
  auto const idList = bookmarkManager.GetUnsortedBmGroupsIdList();
  for (auto catId : idList)
    es.DeleteBmCategory(catId, true);

  bookmarkManager.CreateCategories(std::move(collection));

  ChangeState(State::WaitPosition);
  auto const newCatId = bookmarkManager.GetUnsortedBmGroupsIdList().front();
  m_framework.ShowBookmarkCategory(newCatId, false);
  WaitPosition();
}

void Screenshoter::ProcessNextRect()
{
  if (m_rectsToProcess.empty())
  {
    ChangeState(State::Done);
    return;
  }

  std::string const postfix = "_" + languages::GetCurrentNorm();
  std::stringstream ss;
  ss << "rect_" << std::setfill('0') << std::setw(4) << m_itemsCount - m_rectsToProcess.size() << postfix;

  m_nextScreenshotName = ss.str();

  auto const rect = m_rectsToProcess.front();
  m_rectsToProcess.pop_front();

  CHECK(rect.IsValid(), ());

  ChangeState(State::WaitPosition);
  m_framework.ShowRect(rect, -1 /* maxScale */, false /* animation */, false /* useVisibleViewport */);
  WaitPosition();
}

void Screenshoter::ProcessNextPoint()
{
  if (m_pointsToProcess.empty())
  {
    ChangeState(State::Done);
    return;
  }

  std::string const postfix = "_" + languages::GetCurrentNorm();
  std::stringstream ss;
  ss << "point_" << std::setfill('0') << std::setw(4) << m_itemsCount - m_pointsToProcess.size() << postfix;

  m_nextScreenshotName = ss.str();

  auto const pointZoom = m_pointsToProcess.front();
  m_pointsToProcess.pop_front();

  ChangeState(State::WaitPosition);
  m_framework.SetViewportCenter(pointZoom.first, pointZoom.second, false /* animation */);
  WaitPosition();
}

void Screenshoter::PrepareCountries()
{
  CHECK(m_countriesToDownload.empty(), ());

  auto & storage = m_framework.GetStorage();

  int const kMinCountryZoom = 10;
  if (m_framework.GetDrawScale() >= kMinCountryZoom)
  {
    auto countryIds = m_framework.GetCountryInfoGetter().GetRegionsCountryIdByRect(m_framework.GetCurrentViewport(),
                                                                                   false /* rough */);
    for (auto const & countryId : countryIds)
    {
      if (storage.CountryStatusEx(countryId) == storage::Status::NotDownloaded)
      {
        ChangeState(State::WaitCountries);
        m_countriesToDownload.insert(countryId);
        storage.DownloadCountry(countryId, MapFileType::Map);
      }
    }
  }

  if (m_countriesToDownload.empty())
  {
    ChangeState(State::WaitGraphics);
    WaitGraphics();
  }
}

void Screenshoter::OnCountryChanged(storage::CountryId countryId)
{
  if (m_state != State::WaitCountries)
    return;

  auto const status = m_framework.GetStorage().CountryStatusEx(countryId);
  if (status == storage::Status::OnDisk)
  {
    m_countriesToDownload.erase(countryId);
    if (m_countriesToDownload.empty())
    {
      ChangeState(State::WaitGraphics);
      auto const kDelay = std::chrono::seconds(3);
      GetPlatform().RunDelayedTask(Platform::Thread::File, kDelay, [this] { WaitGraphics(); });
    }
  }
}

void Screenshoter::OnPositionReady()
{
  if (m_state != State::WaitPosition)
    return;
  PrepareCountries();
}

void Screenshoter::OnGraphicsReady()
{
  if (m_state != State::WaitGraphics)
    return;

  ChangeState(State::Ready);
  SaveScreenshot();
}

void Screenshoter::WaitPosition()
{
  m_framework.NotifyGraphicsReady([this]()
  { GetPlatform().RunTask(Platform::Thread::Gui, [this] { OnPositionReady(); }); }, false /* needInvalidate */);
}

void Screenshoter::WaitGraphics()
{
  m_framework.NotifyGraphicsReady([this]()
  { GetPlatform().RunTask(Platform::Thread::Gui, [this] { OnGraphicsReady(); }); }, true /* needInvalidate */);
}

void Screenshoter::SaveScreenshot()
{
  CHECK_EQUAL(m_state, State::Ready, ());

  if (!Platform::MkDirChecked(m_screenshotParams.m_dstPath))
  {
    ChangeState(State::FileError);
    return;
  }

  QSize screenshotSize(m_screenshotParams.m_width, m_screenshotParams.m_height);
  QImage framebuffer = m_widget->grabFramebuffer();
  ASSERT_EQUAL(framebuffer.width() % screenshotSize.width(), 0, ());
  ASSERT_EQUAL(framebuffer.height() % screenshotSize.height(), 0, ());
  QImage screenshot = framebuffer.scaled(screenshotSize);
  if (!screenshot.save(
          QString::fromStdString(base::JoinPath(m_screenshotParams.m_dstPath, m_nextScreenshotName + ".png")), "PNG",
          100))
  {
    ChangeState(State::FileError);
    return;
  }
  m_nextScreenshotName.clear();

  ProcessNextItem();
}

void Screenshoter::ChangeState(State newState, std::string const & msg)
{
  m_state = newState;
  if (m_screenshotParams.m_statusChangedFn)
  {
    std::ostringstream ss;
    ss << "[" << m_itemsCount - GetItemsToProcessCount() << "/" << m_itemsCount << "] file: " << m_nextScreenshotName
       << " state: " << DebugPrint(newState);
    if (!msg.empty())
      ss << " msg: " << msg;
    LOG(LINFO, (ss.str()));
    m_screenshotParams.m_statusChangedFn(ss.str(), newState == Screenshoter::State::Done);
  }
}

size_t Screenshoter::GetItemsToProcessCount()
{
  switch (m_screenshotParams.m_mode)
  {
  case ScreenshotParams::Mode::KmlFiles: return m_filesToProcess.size();
  case ScreenshotParams::Mode::Points: return m_pointsToProcess.size();
  case ScreenshotParams::Mode::Rects: return m_rectsToProcess.size();
  }
  UNREACHABLE();
}

bool Screenshoter::PrepareItemsToProcess()
{
  switch (m_screenshotParams.m_mode)
  {
  case ScreenshotParams::Mode::KmlFiles: return PrepareKmlFiles();
  case ScreenshotParams::Mode::Points: return PreparePoints();
  case ScreenshotParams::Mode::Rects: return PrepareRects();
  }
  UNREACHABLE();
}

bool Screenshoter::PrepareKmlFiles()
{
  if (!Platform::IsDirectory(m_screenshotParams.m_kmlPath) &&
      !Platform::IsFileExistsByFullPath(m_screenshotParams.m_kmlPath))
  {
    ChangeState(State::FileError, "Invalid kml path.");
    return false;
  }

  if (!Platform::IsDirectory(m_screenshotParams.m_kmlPath))
  {
    m_filesToProcess.push_back(m_screenshotParams.m_kmlPath);
  }
  else
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(m_screenshotParams.m_kmlPath, kKmlExtension, files);
    for (auto const & file : files)
    {
      auto const filePath = base::JoinPath(m_screenshotParams.m_kmlPath, file);
      m_filesToProcess.push_back(filePath);
    }
  }

  m_itemsCount = m_filesToProcess.size();
  return true;
}

bool Screenshoter::PreparePoints()
{
  if (!LoadPoints(m_screenshotParams.m_points))
  {
    ChangeState(State::ParamsError, "Invalid points.");
    return false;
  }
  m_itemsCount = m_pointsToProcess.size();
  return true;
}

bool Screenshoter::PrepareRects()
{
  if (!LoadRects(m_screenshotParams.m_rects))
  {
    ChangeState(State::ParamsError, "Invalid rects.");
    return false;
  }
  m_itemsCount = m_rectsToProcess.size();
  return true;
}

bool Screenshoter::LoadRects(std::string const & rects)
{
  if (Platform::IsFileExistsByFullPath(rects))
  {
    std::ifstream fin(rects);
    std::string line;
    while (std::getline(fin, line))
    {
      if (!ParseRectsStr(line, m_rectsToProcess))
      {
        m_rectsToProcess.clear();
        return false;
      }
    }
  }
  else if (!ParseRectsStr(rects, m_rectsToProcess))
  {
    m_rectsToProcess.clear();
    return false;
  }
  return !m_rectsToProcess.empty();
}

bool Screenshoter::LoadPoints(std::string const & points)
{
  if (Platform::IsFileExistsByFullPath(points))
  {
    std::ifstream fin(points);
    std::string line;
    while (std::getline(fin, line))
    {
      if (!ParsePointsStr(line, m_pointsToProcess))
      {
        m_pointsToProcess.clear();
        return false;
      }
    }
  }
  else if (!ParsePointsStr(points, m_pointsToProcess))
  {
    m_pointsToProcess.clear();
    return false;
  }
  return !m_pointsToProcess.empty();
}

std::string DebugPrint(Screenshoter::State state)
{
  switch (state)
  {
  case Screenshoter::State::NotStarted: return "NotStarted";
  case Screenshoter::State::LoadKml: return "LoadKml";
  case Screenshoter::State::WaitPosition: return "WaitPosition";
  case Screenshoter::State::WaitCountries: return "WaitCountries";
  case Screenshoter::State::WaitGraphics: return "WaitGraphics";
  case Screenshoter::State::Ready: return "Ready";
  case Screenshoter::State::FileError: return "FileError";
  case Screenshoter::State::ParamsError: return "ParamsError";
  case Screenshoter::State::Done: return "Done";
  }
  UNREACHABLE();
}

std::string DebugPrint(ScreenshotParams::Mode mode)
{
  switch (mode)
  {
  case ScreenshotParams::Mode::KmlFiles: return "KmlFiles";
  case ScreenshotParams::Mode::Points: return "Points";
  case ScreenshotParams::Mode::Rects: return "Rects";
  }
  UNREACHABLE();
}
}  // namespace qt
