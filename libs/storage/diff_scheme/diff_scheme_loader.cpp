#include "storage/diff_scheme/diff_scheme_loader.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <glaze/json.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "private.h"

namespace storage::diffs_json
{
struct LocalMap
{
  std::string name;
  uint64_t version = 0;
};

struct LocalMapsRequest
{
  std::vector<LocalMap> mwms;
  uint64_t max_version = 0;
};

struct DiffResponse
{
  std::string name;
  int64_t size = -1;
};

struct DiffListResponse
{
  std::vector<DiffResponse> mwms;
};
}  // namespace storage::diffs_json

namespace
{
using namespace storage::diffs;

auto const kTimeoutInSeconds = 5.0;

std::string SerializeCheckerData(LocalMapsInfo const & info)
{
  storage::diffs_json::LocalMapsRequest request;
  request.mwms.reserve(info.m_localMaps.size());
  for (auto const & nameAndVersion : info.m_localMaps)
    request.mwms.push_back({.name = nameAndVersion.first, .version = nameAndVersion.second});
  request.max_version = info.m_currentDataVersion;

  std::string buffer;
  if (auto const error = glz::write_json(request, buffer); error)
    LOG(LERROR, ("Diff request serialization failed:", glz::format_error(error)));
  return buffer;
}

NameDiffInfoMap DeserializeResponse(std::string const & response, LocalMapsInfo::NameVersionMap const & nameVersionMap)
{
  if (response.empty())
  {
    LOG(LERROR, ("Diff response shouldn't be empty."));
    return {};
  }

  storage::diffs_json::DiffListResponse diffList;
  glz::opts constexpr opts{.error_on_unknown_keys = false};
  if (auto const error = glz::read<opts>(diffList, response); error)
    return {};

  if (diffList.mwms.empty() || diffList.mwms.size() != nameVersionMap.size())
  {
    LOG(LERROR, ("Diff list size in response must be equal to mwm list size in request."));
    return {};
  }

  NameDiffInfoMap diffs;

  for (auto const & diffJson : diffList.mwms)
  {
    // Invalid size. The diff is not available.
    if (diffJson.size < 0)
      continue;

    if (nameVersionMap.find(diffJson.name) == nameVersionMap.end())
    {
      LOG(LERROR, ("Incorrect country name in response:", diffJson.name));
      return {};
    }

    DiffInfo info(diffJson.size, nameVersionMap.at(diffJson.name));
    diffs.emplace(diffJson.name, std::move(info));
  }

  return diffs;
}

NameDiffInfoMap Load(LocalMapsInfo const & info)
{
  if (info.m_localMaps.empty() || DIFF_LIST_URL[0] == 0)
    return {};

  platform::HttpClient request(DIFF_LIST_URL);
  std::string const body = SerializeCheckerData(info);
  ASSERT(!body.empty(), ());
  request.SetBodyData(body, "application/json");
  request.SetTimeout(kTimeoutInSeconds);
  NameDiffInfoMap diffs;
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    diffs = DeserializeResponse(request.ServerResponse(), info.m_localMaps);
  }
  else
  {
    std::ostringstream ost;
    ost << "Request to diffs server failed. Code = " << request.ErrorCode()
        << ", redirection = " << request.WasRedirected();
    LOG(LINFO, (ost.str()));
  }

  return diffs;
}
}  // namespace

namespace storage
{
namespace diffs
{
// static
void Loader::Load(LocalMapsInfo && info, DiffsReceivedCallback && callback)
{
  GetPlatform().RunTask(Platform::Thread::Network, [info = std::move(info), callback = std::move(callback)]()
  {
    auto result = ::Load(info);
    GetPlatform().RunTask(Platform::Thread::Gui, [result = std::move(result), callback = std::move(callback)]() mutable
    { callback(std::move(result)); });
  });
}
}  // namespace diffs
}  // namespace storage
