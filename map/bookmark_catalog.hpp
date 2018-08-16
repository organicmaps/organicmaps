#pragma once

#include "platform/remote_file.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

class BookmarkCatalog
{
public:
  void RegisterByServerId(std::string const & id);
  void UnregisterByServerId(std::string const & id);
  void Download(std::string const & id, std::string const & name,
                std::function<void()> && startHandler,
                platform::RemoteFile::ResultHandler && finishHandler);

  bool IsDownloading(std::string const & id) const;
  bool HasDownloaded(std::string const & id) const;
  size_t GetDownloadingCount() const { return m_downloadingIds.size(); }
  std::vector<std::string> GetDownloadingNames() const;

  std::string GetDownloadUrl(std::string const & serverId) const;
  std::string GetFrontendUrl() const;

private:
  std::map<std::string, std::string> m_downloadingIds;
  std::set<std::string> m_registeredInCatalog;
};
