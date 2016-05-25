#include "editor/user_stats.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"

#include "3party/Alohalytics/src/http_client.h"
#include "3party/pugixml/src/pugixml.hpp"

using TRequest = alohalytics::HTTPClientPlatformWrapper;


namespace
{
string const kUserStatsUrl = "http://py.osmz.ru/mmwatch/user?format=xml";
auto constexpr kUninitialized = -1;
}  // namespace

namespace editor
{
UserStats::UserStats(string const & userName)
  : m_userName(userName), m_changesCount(kUninitialized), m_rank(kUninitialized)
{
  m_updateStatus = Update();
}

bool UserStats::IsChangesCountInitialized() const
{
  return m_changesCount != kUninitialized;
}

bool UserStats::IsRankInitialized() const
{
  return m_rank != kUninitialized;
}

bool UserStats::Update()
{
  auto const url = kUserStatsUrl + "&name=" + UrlEncode(m_userName);
  TRequest request(url);

  if (!request.RunHTTPRequest())
  {
    LOG(LWARNING, ("Network error while connecting to", url));
    return false;
  }

  if (request.error_code() != 200)
  {
    LOG(LWARNING, ("Server returned", request.error_code(), "for url", url));
    return false;
  }

  auto const response = request.server_response();

  pugi::xml_document document;
  if (!document.load_buffer(response.data(), response.size()))
  {
    LOG(LWARNING, ("Cannot parse server response:", response));
    return false;
  }

  m_changesCount = document.select_node("mmwatch/edits/@value").attribute().as_int(kUninitialized);
  m_rank = document.select_node("mmwatch/rank/@value").attribute().as_int(kUninitialized);

  return true;
}
}  // namespace editor
