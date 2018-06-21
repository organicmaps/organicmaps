#pragma once

#include "ugc/types.hpp"

namespace ugc
{
namespace migration
{
enum class Result
{
  UpToDate,
  Failure,
  Success
};

Result Migrate(UpdateIndexes & source);
}  // namespace migration
}  // namespace ugc
