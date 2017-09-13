#pragma once

#include "storage/diff_scheme/diff_types.hpp"

namespace storage
{
namespace diffs
{
class Checker final
{
public:
  static NameFileInfoMap Check(LocalMapsInfo const & info);
};
}  // namespace diffs
}  // namespace storage
