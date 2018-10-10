#include "map/bookmark_catalog.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/serdes_json.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/visitor.hpp"

#include <cstring>
#include <sstream>
#include <utility>

#include "private.h"

namespace
{
std::string const kCatalogFrontendServer = BOOKMARKS_CATALOG_FRONT_URL;
std::string const kCatalogDownloadServer = BOOKMARKS_CATALOG_DOWNLOAD_URL;
std::string const kCatalogEditorServer = BOOKMARKS_CATALOG_EDITOR_URL;

std::string BuildCatalogDownloadUrl(std::string const & serverId)
{
  if (kCatalogDownloadServer.empty())
    return {};
  return kCatalogDownloadServer + serverId;
}

std::string BuildTagsUrl(std::string const & language)
{
  if (kCatalogEditorServer.empty())
    return {};
  return kCatalogEditorServer + "editor/tags/?lang=" + language;
}

struct SubtagData
{
  std::string m_name;
  std::string m_color;
  std::string m_translation;
  DECLARE_VISITOR(visitor(m_name, "name"),
                  visitor(m_color, "color"),
                  visitor(m_translation, "translation"))
};

struct TagData
{
  std::string m_name;
  std::vector<SubtagData> m_subtags;
  DECLARE_VISITOR(visitor(m_name, "name"),
                  visitor(m_subtags, "subtags"))
};

struct TagsResponseData
{
  std::vector<TagData> m_tags;

  DECLARE_VISITOR(visitor(m_tags))
};

BookmarkCatalog::Tag::Color ExtractColor(std::string const & c)
{
  BookmarkCatalog::Tag::Color result;

  auto const components = c.size() / 2;
  if (components < result.size())
    return {};

  for (size_t i = 0; i < result.size(); i++)
    result[i] = static_cast<float>(std::stoi(c.substr(i * 2, 2), nullptr, 16)) / 255;
  return result;
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
  auto const path = base::JoinPath(GetPlatform().TmpDir(), "file" + strings::to_string(++counter));

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

void BookmarkCatalog::RequestTagGroups(std::string const & language,
                                       BookmarkCatalog::TagGroupsCallback && callback) const
{
  auto const tagsUrl = BuildTagsUrl(language);
  if (tagsUrl.empty())
  {
    if (callback)
      callback(false /* success */, {});
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [tagsUrl, callback = std::move(callback)]()
  {
    platform::HttpClient request(tagsUrl);
    request.SetRawHeader("Accept", "application/json");
    request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
    if (request.RunHttpRequest())
    {
      auto const resultCode = request.ErrorCode();
      if (resultCode >= 200 && resultCode < 300)  // Ok.
      {
        TagsResponseData tagsResponseData;
        try
        {
          coding::DeserializerJson des(request.ServerResponse());
          des(tagsResponseData);
        }
        catch (coding::DeserializerJson::Exception const & ex)
        {
          LOG(LWARNING, ("Tags request deserialization error:", ex.Msg()));
          if (callback)
            callback(false /* success */, {});
          return;
        }

        TagGroups result;
        result.reserve(tagsResponseData.m_tags.size());
        for (auto const & t : tagsResponseData.m_tags)
        {
          TagGroup group;
          group.m_name = t.m_name;
          group.m_tags.reserve(t.m_subtags.size());
          for (auto const & st : t.m_subtags)
          {
            Tag tag;
            tag.m_id = st.m_name;
            tag.m_name = st.m_translation;
            tag.m_color = ExtractColor(st.m_color);
            group.m_tags.push_back(std::move(tag));
          }
          result.push_back(std::move(group));
        }
        if (callback)
          callback(true /* success */, result);
        return;
      }
      else
      {
        LOG(LWARNING, ("Tags request error. Code =", resultCode,
                       "Response =", request.ServerResponse()));
      }
    }
    if (callback)
      callback(false /* success */, {});
  });
}
