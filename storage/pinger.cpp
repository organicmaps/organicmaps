#include "storage/pinger.hpp"

#include "platform/http_client.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/thread_pool_delayed.hpp"

#include <chrono>


namespace pinger
{
auto constexpr kTimeoutInSeconds = 4.0;
int64_t constexpr kInvalidPing = -1;

int64_t DoPing(std::string const & url)
{
  using namespace std::chrono;

  if (url.empty())
  {
    ASSERT(false, ("Metaserver returned an empty url."));
    return kInvalidPing;
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

  return kInvalidPing;
}
} // namespace pinger

namespace storage
{
// static
Pinger::Endpoints Pinger::ExcludeUnavailableAndSortEndpoints(Endpoints const & urls)
{
  auto const size = urls.size();
  CHECK_GREATER(size, 0, ());

  using EntryT = std::pair<int64_t, size_t>;
  std::vector<EntryT> timeUrls(size, {pinger::kInvalidPing, 0});
  {
    base::DelayedThreadPool pool(size, base::DelayedThreadPool::Exit::ExecPending);
    for (size_t i = 0; i < size; ++i)
    {
      pool.Push([&urls, &timeUrls, i]
      {
        timeUrls[i] = { pinger::DoPing(urls[i]), i };
      });
    }
  }

  std::sort(timeUrls.begin(), timeUrls.end(), [](EntryT const & e1, EntryT const & e2)
  {
    return e1.first < e2.first;
  });

  Endpoints readyUrls;
  for (auto const & [ping, index] : timeUrls)
  {
    if (ping >= 0)
      readyUrls.push_back(urls[index]);
  }

  return readyUrls;
}
}  // namespace storage
