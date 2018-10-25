#pragma once

#include "kml/types.hpp"

#include "platform/remote_file.hpp"
#include "platform/safe_callback.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class BookmarkCatalog
{
public:
  struct Tag
  {
    using Color = std::array<float, 3>;

    std::string m_id;
    Color m_color;
    std::string m_name;
  };

  struct TagGroup
  {
    std::string m_name;
    std::vector<Tag> m_tags;
  };

  using TagGroups = std::vector<TagGroup>;
  using TagGroupsCallback = platform::SafeCallback<void(bool success, TagGroups const &)>;

  void RegisterByServerId(std::string const & id);
  void UnregisterByServerId(std::string const & id);
  void Download(std::string const & id, std::string const & name,
                std::string const & accessToken,
                std::function<void()> && startHandler,
                platform::RemoteFile::ResultHandler && finishHandler);

  bool IsDownloading(std::string const & id) const;
  bool HasDownloaded(std::string const & id) const;
  size_t GetDownloadingCount() const { return m_downloadingIds.size(); }
  std::vector<std::string> GetDownloadingNames() const;

  std::string GetDownloadUrl(std::string const & serverId) const;
  std::string GetFrontendUrl() const;

  void RequestTagGroups(std::string const & language, TagGroupsCallback && callback) const;

  enum class UploadResult
  {
    Success,
    NetworkError,
    ServerError,
    AuthError,
    MalformedDataError,
    AccessError,
    InvalidCall
  };
  using UploadSuccessCallback = platform::SafeCallback<void(UploadResult result,
                                                            std::shared_ptr<kml::FileData> fileData,
                                                            bool originalFileExists,
                                                            bool originalFileUnmodified)>;
  using UploadErrorCallback = platform::SafeCallback<void(UploadResult result,
                                                          std::string const & description)>;

  struct UploadData
  {
    std::string m_serverId;
    kml::AccessRules m_accessRules = kml::AccessRules::DirectLink;
    std::string m_userName;
    std::string m_userId;
  };

  void Upload(UploadData uploadData, std::string const & accessToken,
              std::shared_ptr<kml::FileData> fileData, std::string const & pathToKmb,
              UploadSuccessCallback && uploadSuccessCallback,
              UploadErrorCallback && uploadErrorCallback);

private:
  std::map<std::string, std::string> m_downloadingIds;
  std::set<std::string> m_registeredInCatalog;
};
