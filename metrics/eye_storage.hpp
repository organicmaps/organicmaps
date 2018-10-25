#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace eye
{
class Storage
{
public:
  static std::string GetEyeDir();
  static std::string GetInfoFilePath();
  static std::string GetPoiEventsFilePath();
  static bool SaveInfo(std::vector<int8_t> const & src);
  static bool LoadInfo(std::vector<int8_t> & dst);
  static bool SaveMapObjects(std::vector<int8_t> const & src);
  static bool LoadMapObjects(std::vector<int8_t> & dst);
  static bool AppendMapObjectEvent(std::vector<int8_t> const & src);
  static void Migrate();
};
}  // namespace eye
