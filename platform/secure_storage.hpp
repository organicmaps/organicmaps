#pragma once

#include <string>

namespace platform
{
class SecureStorage
{
public:
  void Save(std::string const & key, std::string const & value);
  bool Load(std::string const & key, std::string & value);
  void Remove(std::string const & key);
};
}  // namespace platform
