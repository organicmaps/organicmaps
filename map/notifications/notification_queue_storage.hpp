#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace notifications
{
class QueueStorage
{
public:
  static std::string GetFilePath();
  static std::string GetNotificationsDir();
  static bool Save(std::vector<int8_t> const & src);
  static bool Load(std::vector<int8_t> & dst);
};
}  // namespace notifications
