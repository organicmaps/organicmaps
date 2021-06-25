#include "storage/pinger.hpp"

#include "platform/http_client.hpp"
#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread_pool_delayed.hpp"

#include <chrono>
#include <map>


using namespace std;
using namespace std::chrono;

namespace
{
auto constexpr kTimeoutInSeconds = 4.0;

int32_t DoPing(string const & url)
{
  if (url.empty())
  {
    ASSERT(false, ("Metaserver returned an empty url."));
    return -1;
  }

  platform::HttpClient request(url);
  request.SetHttpMethod("HEAD");
  request.SetTimeout(kTimeoutInSeconds);
  auto const begin = high_resolution_clock::now();
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    return duration_cast<milliseconds>(high_resolution_clock::now() - begin).count();
  }
  else
  {
    LOG(LWARNING, ("Request to server", url, "failed with code =", request.ErrorCode(), "; redirection =", request.WasRedirected()));
  }

  return -1;
}
}  // namespace

namespace storage
{
// static
Pinger::Endpoints Pinger::ExcludeUnavailableEndpoints(Endpoints const & urls)
{
  using base::thread_pool::delayed::ThreadPool;

  auto const size = urls.size();
  CHECK_GREATER(size, 0, ());

  map<int32_t, size_t> timeUrls;
  {
    ThreadPool t(size, ThreadPool::Exit::ExecPending);
    for (size_t i = 0; i < size; ++i)
    {
      t.Push([&urls, &timeUrls, i]
      {
        auto const pingTime = DoPing(urls[i]);
        if (pingTime > 0)
          timeUrls[pingTime] = i;
      });
    }
  }

  Endpoints readyUrls;
  for (auto const & [_, index] : timeUrls)
    readyUrls.push_back(urls[index]);

  return readyUrls;
}
}  // namespace storage
