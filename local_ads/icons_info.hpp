#pragma once

#include "base/macros.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace local_ads
{
class IconsInfo
{
public:
  static IconsInfo & Instance();

  void SetSourceFile(std::string const & fileName);
  std::string GetIcon(uint16_t index) const;

private:
  IconsInfo() = default;
  ~IconsInfo() {}

  std::string m_fileName;
  std::map<uint16_t, std::string> m_icons;
  mutable std::mutex m_mutex;

  DISALLOW_COPY_AND_MOVE(IconsInfo);
};
}  // namespace local_ads
