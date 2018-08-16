#include "map/bookmark_catalog.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <sstream>
#include <utility>

//#define STAGE_BOOKMARKS_CATALOG_SERVER
#include "private.h"

namespace
{
std::string const kCatalogFrontendServer = BOOKMARKS_CATALOG_FRONT_URL;
std::string const kCatalogDownloadServer = BOOKMARKS_CATALOG_DOWNLOAD_URL;

std::string BuildCatalogDownloadUrl(std::string const & serverId)
{
  if (kCatalogDownloadServer.empty())
    return {};
  return kCatalogDownloadServer + serverId;
}
}  // namespace

void BookmarkCatalog::RegisterByServerId(std::string const & id)
{
  if (id.empty())
    return;

  m_registeredInCatalog.insert(id);
}

void BookmarkCatalog::UnregisterByServerId(std::string const & id)
{
  if (id.empty())
    return;

  m_registeredInCatalog.erase(id);
}

bool BookmarkCatalog::IsDownloading(std::string const & id) const
{
  return m_downloadingIds.find(id) != m_downloadingIds.cend();
}

bool BookmarkCatalog::HasDownloaded(std::string const & id) const
{
  return m_registeredInCatalog.find(id) != m_registeredInCatalog.cend();
}

std::vector<std::string> BookmarkCatalog::GetDownloadingNames() const
{
  std::vector<std::string> names;
  names.reserve(m_downloadingIds.size());
  for (auto const & p : m_downloadingIds)
    names.push_back(p.second);
  return names;
}

void BookmarkCatalog::Download(std::string const & id, std::string const & name,
                               std::function<void()> && startHandler,
                               platform::RemoteFile::ResultHandler && finishHandler)
{
  if (IsDownloading(id) || HasDownloaded(id))
    return;

  m_downloadingIds.insert(std::make_pair(id, name));

  static uint32_t counter = 0;
  auto const path = my::JoinPath(GetPlatform().TmpDir(), "file" + strings::to_string(++counter));

  platform::RemoteFile remoteFile(BuildCatalogDownloadUrl(id));
  remoteFile.DownloadAsync(path, [startHandler = std::move(startHandler)](std::string const &)
  {
    if (startHandler)
      startHandler();
  }, [this, id, finishHandler = std::move(finishHandler)] (platform::RemoteFile::Result && result,
                                                           std::string const & filePath) mutable
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, id, result = std::move(result), filePath,
                                                  finishHandler = std::move(finishHandler)]() mutable
    {
      m_downloadingIds.erase(id);

      if (finishHandler)
        finishHandler(std::move(result), filePath);
    });
  });
}

std::string BookmarkCatalog::GetDownloadUrl(std::string const & serverId) const
{
  return BuildCatalogDownloadUrl(serverId);
}

std::string BookmarkCatalog::GetFrontendUrl() const
{
  return kCatalogFrontendServer + languages::GetCurrentNorm() + "/mobilefront/";
}
