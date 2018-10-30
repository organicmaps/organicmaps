#pragma once

#include "map/notifications/notification_queue.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <vector>

namespace notifications
{
class QueueSerdes
{
public:
  DECLARE_EXCEPTION(UnknownVersion, RootException);

  static void Serialize(Queue const & queue, std::vector<int8_t> & result);
  static void Deserialize(std::vector<int8_t> const & bytes, Queue & result);
};
}  // namespace notifications
