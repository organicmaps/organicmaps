#pragma once

#include "partners_api/utm.hpp"

#include "map/catalog_headers_provider.hpp"

#include "kml/types.hpp"

#include "platform/http_client.hpp"
#include "platform/remote_file.hpp"
#include "platform/safe_callback.hpp"

#include <array>
#include <functional>
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

  struct CustomProperty
  {
    std::string m_key;
    std::string m_name;
    bool m_isRequired = true;

    struct Option
    {
      std::string m_value;
      std::string m_name;
    };
    std::vector<Option> m_options;
  };

  using TagGroups = std::vector<TagGroup>;
  using TagGroupsCallback = platform::SafeCallback<void(bool success, TagGroups const &,
                                                        uint32_t maxTagsCount)>;
  using CustomProperties = std::vector<CustomProperty>;
  using CustomPropertiesCallback = platform::SafeCallback<void(bool success, CustomProperties const &)>;
  using InvalidTokenHandler = std::function<void()>;

  using HeadersProvider = std::function<platform::HttpClient::Headers()>;

  void RegisterByServerId(std::string const & id);
  void UnregisterByServerId(std::string const & id);

  enum class DownloadResult
  {
    Success,
    NetworkError,
    ServerError,
    AuthError,
    DiskError,
    NeedPayment
  };
  using DownloadStartCallback = std::function<void()>;
  using DownloadFinishCallback = std::function<void(DownloadResult result,
                                                    std::string const & description,
                                                    std::string const & filePath)>;
  void Download(std::string const & id, std::string const & accessToken,
                DownloadStartCallback && startHandler,
                DownloadFinishCallback && finishHandler);

  bool IsDownloading(std::string const & id) const;
  bool HasDownloaded(std::string const & id) const;
  std::set<std::string> const & GetDownloadedIds() const;
  size_t GetDownloadingCount() const { return m_downloadingIds.size(); }

  std::string GetDownloadUrl(std::string const & serverId) const;
  std::string GetWebEditorUrl(std::string const & serverId, std::string const & language) const;
  std::string GetFrontendUrl(UTM utm) const;

  void RequestTagGroups(std::string const & language, TagGroupsCallback && callback) const;

  void RequestCustomProperties(std::string const & language,
                               CustomPropertiesCallback && callback) const;

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

  using PingCallback = platform::SafeCallback<void(bool isSuccessful)>;
  void Ping(PingCallback && callback) const;

  using BookmarksToDeleteCallback = platform::SafeCallback<void(std::vector<std::string> const & serverIds)>;
  void RequestBookmarksToDelete(std::string const & accessToken, std::string const & userId,
                                std::vector<std::string> const & serverIds,
                                BookmarksToDeleteCallback && callback) const;

  // Handler can be called from non-UI thread.
  void SetInvalidTokenHandler(InvalidTokenHandler && onInvalidToken);

  void SetHeadersProvider(HeadersProvider const & provider);
  platform::HttpClient::Headers GetHeaders() const;

private:
  std::set<std::string> m_downloadingIds;
  std::set<std::string> m_registeredInCatalog;
  InvalidTokenHandler m_onInvalidToken;
  HeadersProvider m_headersProvider;
};
