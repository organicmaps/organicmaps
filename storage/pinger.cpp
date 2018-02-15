#include "storage/pinger.hpp"

#include "platform/http_client.hpp"
#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/worker_thread.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <sstream>
#include <utility>

using namespace std;

namespace
{
auto constexpr kTimeoutInSeconds = 5.0;

void DoPing(string const & url, size_t index, vector<string> & readyUrls)
{
  if (url.empty())
  {
    ASSERT(false, ("Metaserver returned an empty url."));
    return;
  }

  platform::HttpClient request(url);
  request.SetHttpMethod("HEAD");
  request.SetTimeout(kTimeoutInSeconds);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    readyUrls[index] = url;
  }
  else
  {
    ostringstream ost;
    ost << "Request to server " << url << " failed. Code = " << request.ErrorCode()
        << ", redirection = " << request.WasRedirected();
    LOG(LINFO, (ost.str()));
  }
}

void SendStatistics(size_t serversLeft)
{
  alohalytics::Stats::Instance().LogEvent(
      "Downloader_ServerList_check",
      {{"lang", languages::GetCurrentNorm()}, {"servers", to_string(serversLeft)}});
}
}  // namespace

namespace storage
{
// static
void Pinger::Ping(vector<string> const & urls, Pinger::Pong const & pong)
{
  auto const size = urls.size();
  CHECK_GREATER(size, 0, ());

  vector<string> readyUrls(size);
  {
    base::WorkerThread t(size);
    for (size_t i = 0; i < size; ++i)
      t.Push([ url = urls[i], &readyUrls, i ] { DoPing(url, i, readyUrls); });

    t.Shutdown(base::WorkerThread::Exit::ExecPending);
  }

  my::EraseIf(readyUrls, [](auto const & url) { return url.empty(); });
  SendStatistics(readyUrls.size());
  pong(move(readyUrls));
}
}  // namespace storage
