#include "storage/diff_scheme/diff_scheme_checker.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"

#include <memory>
#include <utility>

#include "3party/jansson/myjansson.hpp"

#include "private.h"

using namespace std;

namespace
{
using namespace diff_scheme;

char const kMaxVersionKey[] = "max_version";
char const kMwmsKey[] = "mwms";
char const kNameKey[] = "name";
char const kSizeKey[] = "size";
char const kVersionKey[] = "version";

auto const kTimeoutInSeconds = 5.0;

string SerializeCheckerData(Checker::LocalMapsInfo const & info)
{
  auto mwmsArrayNode = my::NewJSONArray();
  for (auto const & nameAndVersion : info.m_localMaps)
  {
    auto node = my::NewJSONObject();
    ToJSONObject(*node, kNameKey, nameAndVersion.first);
    ToJSONObject(*node, kVersionKey, nameAndVersion.second);
    json_array_append_new(mwmsArrayNode.get(), node.release());
  }

  auto const root = my::NewJSONObject();
  json_object_set_new(root.get(), kMwmsKey, mwmsArrayNode.release());
  ToJSONObject(*root, kMaxVersionKey, info.m_currentDataVersion);
  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(root.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return buffer.get();
}

NameFileInfoMap DeserializeResponse(string const & response, Checker::NameVersionMap const & nameVersionMap)
{
  if (response.empty())
  {
    LOG(LERROR, ("Diff response shouldn't be empty."));
    return NameFileInfoMap{};
  }

  my::Json const json(response.c_str());
  if (json.get() == nullptr)
    return NameFileInfoMap{};

  auto const root = json_object_get(json.get(), kMwmsKey);
  if (root == nullptr || !json_is_array(root))
    return NameFileInfoMap{};

  auto const size = json_array_size(root);
  if (size == 0 || size != nameVersionMap.size())
  {
    LOG(LERROR, ("Diff list size in response must be equal to mwm list size in request."));
    return NameFileInfoMap{};
  }

  NameFileInfoMap diffs;

  for (size_t i = 0; i < size; ++i)
  {
    auto const node = json_array_get(root, i);

    if (!node)
    {
      LOG(LERROR, ("Incorrect server response."));
      return NameFileInfoMap{};
    }

    string name;
    FromJSONObject(node, kNameKey, name);
    int64_t size;
    FromJSONObject(node, kSizeKey, size);
    // Invalid size. The diff is not available.
    if (size < 0)
      continue;

    if (nameVersionMap.find(name) == nameVersionMap.end())
    {
      LOG(LERROR, ("Incorrect country name in response:", name));
      return NameFileInfoMap{};
    }

    FileInfo info(size, nameVersionMap.at(name));
    diffs.emplace(move(name), move(info));
  }

  return diffs;
}
}  // namespace

namespace diff_scheme
{
// static
void Checker::Check(LocalMapsInfo const & info, Callback const & fn)
{
  // TODO(Vlad): Log falling back to old scheme.
  if (info.m_localMaps.empty())
  {
    fn(NameFileInfoMap{});
    return;
  }

  threads::SimpleThread thread([info, fn] {
    platform::HttpClient request(DIFF_LIST_URL);
    string const body = SerializeCheckerData(info);
    ASSERT(!body.empty(), ());
    request.SetBodyData(body, "application/json");
    request.SetTimeout(kTimeoutInSeconds);
    NameFileInfoMap diffs;
    if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
      diffs = DeserializeResponse(request.ServerResponse(), info.m_localMaps);

    GetPlatform().RunOnGuiThread([fn, diffs] {
      fn(diffs);
    });
  });
  thread.detach();
}
}  // namespace diff_scheme
