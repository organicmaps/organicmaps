#pragma once

namespace df
{
// Now only [0-7] values are available.
enum class BatcherBucket
{
  Default = 0,
  Overlay = 1,
  UserMark = 2,
  Routing = 3,
  Traffic = 4,
  Transit = 5
};
}  // namespace df
