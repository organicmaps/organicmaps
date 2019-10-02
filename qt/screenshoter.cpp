#include "qt/screenshoter.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "storage/storage.hpp"

#include "platform/preferred_languages.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <QtGui/QPixmap>

#include <chrono>
#include <functional>
#include <sstream>

namespace qt
{
Screenshoter::Screenshoter(ScreenshotParams const & screenshotParams, Framework & framework, QWidget * widget)
  : m_screenshotParams(screenshotParams)
  , m_framework(framework)
  , m_widget(widget)
{
  m_framework.GetStorage().Subscribe(std::bind(&Screenshoter::OnCountryChanged, this, std::placeholders::_1),
                                     [](storage::CountryId const &, storage::MapFilesDownloader::Progress const &) {});
}

void Screenshoter::Start()
{
  if (m_state != State::NotStarted)
    return;

  if (m_screenshotParams.m_dstPath.empty())
    m_screenshotParams.m_dstPath = base::JoinPath(m_screenshotParams.m_kmlPath, "screenshots");

  LOG(LINFO, ("\nScreenshoter parameters:"
    "\n  kml_path:", m_screenshotParams.m_kmlPath,
    "\n  dst_path:", m_screenshotParams.m_dstPath,
    "\n  width:", m_screenshotParams.m_width,
    "\n  height:", m_screenshotParams.m_height,
    "\n  dpi_scale:", m_screenshotParams.m_dpiScale));

  if (!Platform::IsDirectory(m_screenshotParams.m_kmlPath) || !Platform::MkDirChecked(m_screenshotParams.m_dstPath))
  {
    ChangeState(State::FileError);
    return;
  }

  Platform::FilesList files;
  Platform::GetFilesByExt(m_screenshotParams.m_kmlPath, kKmlExtension, files);

  for (auto const & file : files)
  {
    auto const filePath = base::JoinPath(m_screenshotParams.m_kmlPath, file);
    m_filesToProcess.push_back(filePath);
  }
  m_filesCount = m_filesToProcess.size();

  ChangeState(State::Ready);
  ProcessNextKml();
}

void Screenshoter::ProcessNextKml()
{
  CHECK_EQUAL(m_state, State::Ready, ());

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
  auto const idList = bookmarkManager.GetBmGroupsIdList();
  for (auto catId : idList)
    es.DeleteBmCategory(catId);

  bookmarkManager.CreateCategories(std::move(collection), false);

  ChangeState(State::WaitPosition);
  auto const newCatId = bookmarkManager.GetBmGroupsIdList().front();

  m_dataRect = bookmarkManager.GetCategoryRect(newCatId);
  ExpandBookmarksRectForPreview(m_dataRect);
  CHECK(m_dataRect.IsValid(), ());

  if (!CheckViewport())
    m_framework.ShowBookmarkCategory(newCatId, false);
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
      if (storage.CountryStatusEx(countryId) == storage::Status::ENotDownloaded)
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
  if (status == storage::Status::EOnDisk)
  {
    m_countriesToDownload.erase(countryId);
    if (m_countriesToDownload.empty())
    {
      ChangeState(State::WaitGraphics);
      auto const kDelay = std::chrono::seconds(3);
      GetPlatform().RunDelayedTask(Platform::Thread::File, kDelay, [this]{ WaitGraphics(); });
    }
  }
}

bool Screenshoter::CheckViewport()
{
  if (m_state != State::WaitPosition)
    return false;

  auto const currentViewport = m_framework.GetCurrentViewport();

  double const kEpsRect = 1e-2;
  double const kEpsCenter = 1e-3;
  double const minCheckedZoomLevel = 5;
  if (m_framework.GetDrawScale() < minCheckedZoomLevel ||
      (m_dataRect.Center().EqualDxDy(currentViewport.Center(), kEpsCenter) &&
        (fabs(m_dataRect.SizeX() - currentViewport.SizeX()) < kEpsRect ||
         fabs(m_dataRect.SizeY() - currentViewport.SizeY()) < kEpsRect)))
  {
    PrepareCountries();
    return true;
  }
  return false;
}

void Screenshoter::OnGraphicsReady()
{
  if (m_state != State::WaitGraphics)
    return;

  ChangeState(State::Ready);
  SaveScreenshot();
}

void Screenshoter::WaitGraphics()
{
  m_framework.NotifyGraphicsReady([this]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this] { OnGraphicsReady(); });
  });
}

void Screenshoter::SaveScreenshot()
{
  CHECK_EQUAL(m_state, State::Ready, ());

  if (!Platform::MkDirChecked(m_screenshotParams.m_dstPath))
  {
    ChangeState(State::FileError);
    return;
  }

  QPixmap pixmap(QSize(m_screenshotParams.m_width, m_screenshotParams.m_height));
  m_widget->render(&pixmap, QPoint(), QRegion(m_widget->geometry()));
  if (!pixmap.save(QString::fromStdString(base::JoinPath(m_screenshotParams.m_dstPath, m_nextScreenshotName + ".png")),
                   nullptr, 100))
  {
    ChangeState(State::FileError);
    return;
  }
  m_nextScreenshotName.clear();

  ProcessNextKml();
}

void Screenshoter::ChangeState(State newState)
{
  m_state = newState;
  if (m_screenshotParams.m_statusChangedFn)
  {
    std::ostringstream ss;
    ss << "[" << m_filesCount - m_filesToProcess.size() << "/" << m_filesCount << "] file: "
       << m_nextScreenshotName << " state: " << DebugPrint(newState);
    LOG(LINFO, (ss.str()));
    m_screenshotParams.m_statusChangedFn(ss.str(), newState == Screenshoter::State::Done);
  }
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
  case Screenshoter::State::Done: return "Done";
  }
  UNREACHABLE();
}
}  // namespace qt
