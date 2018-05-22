#include "map/bookmark_catalog.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/string_utils.hpp"

#include <sstream>
#include <utility>

#define STAGE_BOOKMARKS_CATALOG_SERVER
#include "private.h"

namespace
{
std::string const kCatalogFrontendServer = BOOKMARKS_CATALOG_FRONT_URL;
std::string const kCatalogDownloadServer = BOOKMARKS_CATALOG_DOWNLOAD_URL;

std::string BuildCatalogDownloadUrl(std::string const & serverId)
{
  if (kCatalogDownloadServer.empty())
    return {};

  std::ostringstream ss;
  ss << kCatalogDownloadServer << "/static/" << serverId << "/" << serverId;
  return ss.str();
}
}  // namespace

BookmarkCatalog::BookmarkCatalog(std::string const & catalogDir)
{
  Platform::FilesList files;
  Platform::GetFilesRecursively(catalogDir, files);
  for (auto const & f : files)
    m_downloadedIds.insert(my::GetNameFromFullPathWithoutExt(f));
}

void BookmarkCatalog::RegisterDownloadedId(std::string const & id)
{
  if (id.empty())
    return;

  m_downloadedIds.insert(id);
  m_downloadingIds.erase(id);
}

void BookmarkCatalog::UnregisterDownloadedId(std::string const & id)
{
  if (id.empty())
    return;

  m_downloadedIds.erase(id);
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
  if (m_downloadingIds.find(id) != m_downloadingIds.cend() ||
      m_downloadedIds.find(id) != m_downloadedIds.cend())
  {
    return;
  }

  m_downloadingIds.insert(std::make_pair(id, name));

  if (startHandler)
    startHandler();

  static uint32_t counter = 0;
  auto const path = my::JoinPath(GetPlatform().TmpDir(), "file" + strings::to_string(++counter));

  auto const url = BuildCatalogDownloadUrl(id);
  if (url.empty())
  {
    if (finishHandler)
    {
      finishHandler(platform::RemoteFile::Result(url, platform::RemoteFile::Status::NetworkError,
                                                 "Empty server URL."), {});
    }
    return;
  }

  platform::RemoteFile remoteFile(url);
  remoteFile.DownloadAsync(path, [finishHandler = std::move(finishHandler)]
    (platform::RemoteFile::Result && result, std::string const & filePath)
  {
    if (finishHandler)
      finishHandler(std::move(result), filePath);
  });
}

std::string BookmarkCatalog::GetCatalogDownloadUrl(std::string const & serverId) const
{
  return BuildCatalogDownloadUrl(serverId);
}

std::string BookmarkCatalog::GetCatalogFrontendUrl() const
{
  return kCatalogFrontendServer;
}
