#include "platform/servers_list.hpp"
#include "platform/http_request.hpp"
#include "platform/settings.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/assert.hpp"

#include "3party/jansson/myjansson.hpp"

namespace downloader
{

// Returns false if can't parse urls. Note that it also clears outUrls.
bool ParseServerList(string const & jsonStr, vector<string> & outUrls)
{
  outUrls.clear();
  try
  {
    base::Json root(jsonStr.c_str());
    for (size_t i = 0; i < json_array_size(root.get()); ++i)
    {
      char const * url = json_string_value(json_array_get(root.get(), i));
      if (url)
        outUrls.push_back(url);
    }
  }
  catch (base::Json::Exception const & ex)
  {
    LOG(LERROR, ("Can't parse server list json:", ex.Msg(), jsonStr));
  }
  return !outUrls.empty();
}

void GetServerListFromRequest(HttpRequest const & request, vector<string> & urls)
{
  if (request.GetStatus() == HttpRequest::Status::Completed && ParseServerList(request.GetData(), urls))
    return;

  VERIFY(ParseServerList(GetPlatform().DefaultUrlsJSON(), urls), ());
  LOG(LWARNING, ("Can't get servers list from request, using default servers:", urls));
}

} // namespace downloader
