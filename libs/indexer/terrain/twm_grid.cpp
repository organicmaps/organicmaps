#include "indexer/terrain/twm_grid.hpp"

#include "indexer/terrain/terrain_utils.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

namespace terrain
{
std::vector<VersionDir> ListVersionDirs(std::string const & terrainDir)
{
  std::vector<VersionDir> dirs;
  Platform::TFilesWithType subdirs;
  Platform::GetFilesByType(terrainDir, Platform::EFileType::Directory, subdirs);
  for (auto const & [name, type] : subdirs)
    if (uint64_t version; strings::to_uint64(name, version))
      dirs.push_back({base::JoinPath(terrainDir, name), static_cast<int64_t>(version)});
  std::sort(dirs.begin(), dirs.end(),
            [](VersionDir const & a, VersionDir const & b) { return a.m_version > b.m_version; });
  dirs.push_back({terrainDir, 0});
  return dirs;
}

std::string GridBlock::GetFileName() const
{
  return GetBlockFileName(m_bottom, m_left);
}

m2::RectD GridBlock::GetRectMercator() const
{
  return {mercator::FromLatLon(m_bottom, m_left), mercator::FromLatLon(m_bottom + m_height, m_left + m_width)};
}

bool ParseBlockName(std::string_view name, int & bottom, int & left)
{
  if (name.size() != 7 || (name[0] != 'N' && name[0] != 'S') || (name[3] != 'E' && name[3] != 'W'))
    return false;
  int lat;
  int lon;
  if (!strings::to_int(name.substr(1, 2), lat) || !strings::to_int(name.substr(4, 3), lon))
    return false;
  bottom = (name[0] == 'N' ? 1 : -1) * lat;
  left = (name[3] == 'E' ? 1 : -1) * lon;
  return true;
}

bool IsValidBlock(GridBlock const & block)
{
  return block.m_width > 0 && block.m_height > 0 && block.m_left >= -180 && block.m_left + block.m_width <= 180 &&
         block.m_bottom >= -85 && block.m_bottom + block.m_height <= 85;
}
}  // namespace terrain
