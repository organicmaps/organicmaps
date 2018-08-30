#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace eye
{
class Storage
{
public:
  static std::string GetEyeFilePath();
  static bool Save(std::string const & filePath, std::vector<int8_t> const & src);
  static bool Load(std::string const & filePath, std::vector<int8_t> & dst);
};
}  // namespace eye
