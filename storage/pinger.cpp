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

namespace pinger
{
auto constexpr kTimeoutInSeconds = 4.0;
int64_t constexpr kInvalidPing = -1;

int64_t DoPing(string const & url)
{
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
}  // namespace

namespace storage
{
// static
Pinger::Endpoints Pinger::ExcludeUnavailableAndSortEndpoints(Endpoints const & urls)
{
  using base::thread_pool::delayed::ThreadPool;

  auto const size = urls.size();
  CHECK_GREATER(size, 0, ());

  using EntryT = std::pair<int64_t, size_t>;
  std::vector<EntryT> timeUrls(size, {pinger::kInvalidPing, 0});
  {
    ThreadPool pool(size, ThreadPool::Exit::ExecPending);
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
