#include "qt/screenshoter.hpp"

#incldue "storage/storage.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include <QtGui/QPixmap>

Screenshoter::Screenshoter(Framework & framework, QWidget * widget)
  : m_framework(framework)
  , m_widget(widget)
{

}

void Screenshoter::ProcessBookmarks(std::string const & dir)
{
  Platform::FilesList files;
  Platform::GetFilesByExt(dir, kKmlExtension, files);

  for (auto const & file : files)
  {
    auto const filePath = base::JoinPath(dir, file);
    m_filesToProcess.push_back(filePath);
  }
}

void Screenshoter::ProcessNextKml()
{
  std::unique_ptr<kml::FileData> kmlData;
  while (kmlData == nullptr && !m_filesToProcess.empty())
  {
    auto const filePath = m_filesToProcess.front();
    m_filesToProcess.pop_front();
    kmlData = LoadKmlFile(filePath, KmlFileType::Text);

    if (kmlData != nullptr && kmlData->m_bookmarksData.empty() && kmlData->m_tracksData.empty())
      kmlData.reset();
  }

  if (kmlData == nullptr)
    return;

  m_nextScreenshotName = kmlData->m_serverId;

  BookmarkManager::KMLDataCollection collection;
  collection.emplace_back("", std::move(kmlData));

  auto & bookmarkManager = m_framework.GetBookmarkManager();
  auto es = bookmarkManager.GetEditSession();
  auto const idList = bookmarkManager.GetBmGroupsIdList();
  for (auto catId : idList)
    es.DeleteBmCategory(catId);

  bookmarkManager.CreateCategories(std::move(collection), false);

  auto const newCatId = bookmarkManager.GetBmGroupsIdList().front();
  m_framework.ShowBookmarkCategory(newCatId, false);
}

void Screenshoter::PrepareCountries()
{
  ASSERT(m_countriesToDownload.empty(), ());

  auto & storage = m_framework.GetStorage();

  int const kMinCountryZoom = 10;
  if (GetDrawScale() >= kMinCountryZoom)
  {
    auto countryIds = m_framework.GetCountryInfoGetter()->GetRegionsCountryIdByRect(GetCurrentViewport(),
                                                                                    false /* rough */);
    for (auto const & countryId : countryIds)
    {
      if (storage.CountryStatusEx(countryId) == storage::Status::ENotDownloaded)
      {
        storage.DownloadCountry(countryId, MapOptions::Map);
        m_countriesToDownload.insert(countryId);
      }
    }
  }

  if (m_countriesToDownload.empty())
    DoScreenshot();
}

void Screenshoter::OnCountryStatusChanged(storage::CountryId countryId)
{
  m_countriesToDownload.erase(countryId);
  if (m_countriesToDownload.empty())
    DoScreenshot();
}

void Screenshoter::DoScreenshot()
{
  QPixmap pixmap(m_widget->size());
  m_widget->render(&pixmap, QPoint(), QRegion(m_widget->geometry()));
  pixmap.save("/Users/daravolvenkova/Pictures/test_screen/" + m_nextScreenshotName + ".png", 0, 100);
  m_nextScreenshotName.clear();

  ProcessNextKml();
}

