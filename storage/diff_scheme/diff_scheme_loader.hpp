#pragma once

#include "storage/diff_scheme/diff_types.hpp"

#include <functional>

namespace storage
{
namespace diffs
{
using DiffsReceivedCallback = std::function<void(diffs::NameDiffInfoMap && diffs)>;

class Loader final
{
public:
  static void Load(LocalMapsInfo && info, DiffsReceivedCallback && callback);
};
}  // namespace diffs
}  // namespace storage
