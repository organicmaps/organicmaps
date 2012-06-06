#include "servers_list.hpp"
#include "http_request.hpp"
#include "settings.hpp"
#include "platform.hpp"

#include "../base/logging.hpp"
#include "../base/assert.hpp"

#include "../3party/jansson/myjansson.hpp"


#define SETTINGS_SERVERS_KEY "LastBaseUrls"


namespace downloader
{

bool ParseServerList(string const & jsonStr, vector<string> & outUrls)
{
  outUrls.clear();
  try
  {
    my::Json root(jsonStr.c_str());
    for (size_t i = 0; i < json_array_size(root); ++i)
    {
      char const * url = json_string_value(json_array_get(root, i));
      if (url)
        outUrls.push_back(url);
    }
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Can't parse server list json", e.what(), jsonStr));
  }
  return !outUrls.empty();
}

void GetServerListFromRequest(HttpRequest const & request, vector<string> & urls)
{
  if (request.Status() == HttpRequest::ECompleted &&
      ParseServerList(request.Data(), urls))
  {
    Settings::Set(SETTINGS_SERVERS_KEY, request.Data());
  }
  else
  {
    string serverList;
    if (!Settings::Get(SETTINGS_SERVERS_KEY, serverList))
      serverList = GetPlatform().DefaultUrlsJSON();
    VERIFY ( ParseServerList(serverList, urls), () );
  }
}

}
