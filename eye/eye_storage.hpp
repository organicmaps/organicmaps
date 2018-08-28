#pragma once

#include <string>

namespace eye
{
class Storage
{
public:
  static std::string GetEyeFilePath();
  static bool Save(std::string const & filePath, std::vector<int8_t> const & src);
  static void Load(std::string const & filePath, std::vector<int8_t> & dst);
};
}  // namespace eye
